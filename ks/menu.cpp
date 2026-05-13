#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <sstream>

using namespace std;

// ================================================================
// 词法分析基础数据
// ================================================================

// 关键字表 (KT)
const map<string, int> KT = {
    {"int", 1}, {"float", 2}, {"double", 3}, {"char", 4}, {"bool", 5},
    {"if", 6}, {"else", 7}, {"while", 8}, {"for", 9}, {"return", 10},
    {"void", 11}, {"struct", 12}
};

// 界符表 (BT)
const map<string, int> BT = {
    {"+", 1}, {"-", 2}, {"*", 3}, {"/", 4}, {"=", 5}, {"==", 6},
    {"<", 7}, {">", 8}, {"(", 9}, {")", 10}, {"{", 11}, {"}", 12},
    {";", 13}, {",", 14}, {"[", 15}, {"]", 16}, {"!=", 17}, {"<=", 18},
    {">=", 19}, {"&&", 20}, {"||", 21}
};

// 标识符表 (IT) — 全局共享
vector<string> IT;

// ================================================================
// Token 结构
// ================================================================

struct Token {
    char type;   // 'K'=关键字, 'B'=界符, 'I'=标识符, 'C'=常数
    int  id;     // 在对应表中的序号
    string raw;  // 原始文本

    string toString() const {
        return string("(") + type + ", " + to_string(id) + ")";
    }
};

// ================================================================
// 符号表各子结构
// ================================================================

struct Addr {
    string table;
    int    index;
    Addr(string t = "", int i = -1) : table(move(t)), index(i) {}
    string toString() const { return table + "[" + to_string(index) + "]"; }
};

struct TypeNode {
    string TVAL;   // 类型标记："itp","rtp","ctp","dtp","a"(数组),"d"(结构体)
    int    TPOINT; // 指向 AINFL 或 RINFL 的索引，基本类型时为 -1
    int    LEN;    // 字节长度
    TypeNode(string tv = "", int tp = -1, int len = 0)
        : TVAL(move(tv)), TPOINT(tp), LEN(len) {}
};

struct ArrayInfo {
    int    SIZE; // 元素个数
    string CTP;  // 元素类型（"itp" 或 "TYPEL:N"）
    int    CLEN; // 单个元素字节长度
    ArrayInfo(int s = 0, string c = "", int cl = 0)
        : SIZE(s), CTP(move(c)), CLEN(cl) {}
};

struct RecordInfo {
    string ID;  // 成员名
    int    OFF; // 在结构体内的偏移量
    string TP;  // 成员类型
    RecordInfo(string id = "", int off = 0, string tp = "")
        : ID(move(id)), OFF(off), TP(move(tp)) {}
};

struct ValueInfo {
    string NAME;
    int    LEVEL;
    int    OFFSET;
    ValueInfo(string n = "", int l = 0, int off = 0)
        : NAME(move(n)), LEVEL(l), OFFSET(off) {}
};

struct Symbol {
    string NAME;
    string TYPE;
    string CAT;  // 'p'=函数, 'v'=变量, 't'=类型别名
    Addr   ADDR;
    Symbol(string n = "", string t = "", string c = "", Addr a = Addr())
        : NAME(move(n)), TYPE(move(t)), CAT(move(c)), ADDR(move(a)) {}
};

struct ParamNode {
    string NAME;
    string TYPE;
    string KIND;   // "vf" = 值形参
    int    LEVEL;
    int    OFFSET;
    ParamNode(string n = "", string t = "", string k = "vf", int l = 0, int off = 0)
        : NAME(move(n)), TYPE(move(t)), KIND(move(k)), LEVEL(l), OFFSET(off) {}
};

struct ProcInfo {
    string NAME;
    int    LEVEL;
    int    TOTAL_LEN;   // 局部变量总长度
    int    PARAM_COUNT;
    int    ENTRY_INDEX; // 在 PARAML 中的起始索引
    ProcInfo(string n = "", int l = 0, int tl = 0, int pc = 0, int entry = -1)
        : NAME(move(n)), LEVEL(l), TOTAL_LEN(tl), PARAM_COUNT(pc), ENTRY_INDEX(entry) {}
};

// ================================================================
// 词法分析器（独立函数）
// ================================================================

int getOrInsertIdent(const string &s) {
    auto it = find(IT.begin(), IT.end(), s);
    if (it != IT.end()) return (int)distance(IT.begin(), it);
    IT.push_back(s);
    return (int)IT.size() - 1;
}

vector<Token> scanner(const string &code) {
    vector<Token> tokens;
    int i = 0, len = (int)code.length();

    while (i < len) {
        // 跳过空白
        if (isspace((unsigned char)code[i])) { i++; continue; }

        // 行注释
        if (i + 1 < len && code[i] == '/' && code[i+1] == '/') {
            while (i < len && code[i] != '\n') i++;
            continue;
        }
        // 块注释
        if (i + 1 < len && code[i] == '/' && code[i+1] == '*') {
            i += 2;
            while (i + 1 < len && !(code[i] == '*' && code[i+1] == '/')) i++;
            i += 2;
            continue;
        }

        // 标识符 / 关键字
        if (isalpha((unsigned char)code[i]) || code[i] == '_') {
            string s;
            while (i < len && (isalnum((unsigned char)code[i]) || code[i] == '_'))
                s += code[i++];
            if (KT.count(s))
                tokens.push_back({'K', KT.at(s), s});
            else
                tokens.push_back({'I', getOrInsertIdent(s), s});
            continue;
        }

        // 整数 / 浮点常数
        if (isdigit((unsigned char)code[i])) {
            string s;
            while (i < len && isdigit((unsigned char)code[i])) s += code[i++];
            if (i < len && code[i] == '.') {
                s += code[i++];
                while (i < len && isdigit((unsigned char)code[i])) s += code[i++];
            }
            tokens.push_back({'C', (int)stod(s), s});
            continue;
        }

        // 界符（优先匹配双字符）
        {
            string two = (i + 1 < len) ? string{code[i], code[i+1]} : "";
            string one(1, code[i]);
            if (!two.empty() && BT.count(two)) {
                tokens.push_back({'B', BT.at(two), two});
                i += 2;
            } else if (BT.count(one)) {
                tokens.push_back({'B', BT.at(one), one});
                i++;
            } else {
                // 未识别字符跳过（可改为抛异常）
                cerr << "[词法警告] 未识别字符: '" << code[i] << "' 已跳过\n";
                i++;
            }
        }
    }
    return tokens;
}

// ================================================================
// 符号表系统
// ================================================================

class SymbolTableSystem {
public:
    // 各子表
    vector<TypeNode>   TYPEL;
    vector<ArrayInfo>  AINFL;
    vector<RecordInfo> RINFL;
    vector<ValueInfo>  VALL;
    vector<Symbol>     SYNBL;
    vector<ProcInfo>   PFINFL;
    vector<ParamNode>  PARAML;

private:
    vector<Token> tokens; // 保存词法流（供 printAll 使用）
    int pos = 0;

    // 基本类型名 → 内部标记
    map<string, string> typeMap = {
        {"int","itp"}, {"float","rtp"}, {"char","ctp"},
        {"double","dtp"}, {"void","void"}
    };

    int currentLevel = 1;
    vector<int> offsetStack = {0};

    // ---------- 辅助：token 读取 ----------

    bool atEnd() const { return pos >= (int)tokens.size(); }

    Token peek(int ahead = 0) const {
        int idx = pos + ahead;
        if (idx >= (int)tokens.size())
            return {'?', -1, ""};
        return tokens[idx];
    }

    Token consume() {
        if (atEnd()) throw runtime_error("意外的 Token 结束");
        return tokens[pos++];
    }

    bool match(const string &raw) {
        if (!atEnd() && peek().raw == raw) { pos++; return true; }
        return false;
    }

    void expect(const string &raw) {
        if (!match(raw))
            throw runtime_error("期望 '" + raw + "' 但得到 '" + peek().raw + "'");
    }

    // ---------- 类型工具 ----------

    int getTypeLength(const string &t) const {
        if (t == "itp" || t == "bool") return 4;
        if (t == "dtp")                return 8;
        if (t == "rtp")                return 4;   // float 通常 4 字节
        if (t == "ctp")                return 1;
        if (t == "void")               return 0;
        if (t.rfind("TYPEL:", 0) == 0) {
            int idx = stoi(t.substr(6));
            if (idx >= 0 && idx < (int)TYPEL.size())
                return TYPEL[idx].LEN;
        }
        return 0;
    }

    // 将基本类型或已解析类型包装成多维数组，返回最终类型字符串
    string buildArray(const string &base, const vector<int> &dims) {
        if (dims.empty()) return base;
        string cur = base;
        for (int j = (int)dims.size() - 1; j >= 0; j--) {
            int clen = getTypeLength(cur);
            AINFL.push_back({dims[j], cur, clen});
            TYPEL.push_back({"a", (int)AINFL.size()-1, dims[j] * clen});
            cur = "TYPEL:" + to_string((int)TYPEL.size()-1);
        }
        return cur;
    }

    // ---------- 语法 / 语义处理 ----------

    // 解析数组维度列表，返回各维大小
    vector<int> parseDims() {
        vector<int> dims;
        while (!atEnd() && peek().raw == "[") {
            pos++; // skip '['
            Token sz = consume();
            if (sz.type != 'C')
                throw runtime_error("数组维度必须为整型常量，得到: " + sz.raw);
            dims.push_back(sz.id);
            expect("]");
        }
        return dims;
    }

    // 解析 struct 定义
    void parseStruct() {
        Token nameTok = consume();
        string sName = nameTok.raw;
        expect("{");

        vector<pair<string, string>> members; // (成员名, 类型字符串)

        while (!atEnd() && peek().raw != "}") {
            // 成员类型
            Token mTypeTok = consume();
            if (!typeMap.count(mTypeTok.raw))
                throw runtime_error("struct 成员类型未知: " + mTypeTok.raw);
            string mBase = typeMap.at(mTypeTok.raw);

            // 支持同行多个成员：int x, y;
            do {
                if (peek().raw == ",") consume();
                Token mNameTok = consume();
                vector<int> dims = parseDims();
                members.push_back({mNameTok.raw, buildArray(mBase, dims)});
            } while (!atEnd() && peek().raw == ",");

            expect(";");
        }
        expect("}");

        // 填充 RINFL，计算偏移量
        int startR   = (int)RINFL.size();
        int innerOff = 0;
        for (auto &[mName, mType] : members) {
            RINFL.push_back({mName, innerOff, mType});
            innerOff += getTypeLength(mType);
        }

        // 注册结构体类型
        TYPEL.push_back({"d", startR, innerOff});
        int typeIdx = (int)TYPEL.size() - 1;
        string typeStr = "TYPEL:" + to_string(typeIdx);

        SYNBL.push_back({sName, typeStr, "t", Addr("TYPEL", typeIdx)});
        typeMap[sName] = typeStr; // 使结构体名可作为类型使用

        match(";");
    }

    // 解析函数定义（已拿到返回类型 retType 和函数名 name）
    void parseFunction(const string &retType, const string &name) {
        int pfIdx = (int)PFINFL.size();
        SYNBL.push_back({name, retType, "p", Addr("PFINFL", pfIdx)});
        PFINFL.push_back({name, currentLevel, 0, 0, (int)PARAML.size()});

        currentLevel++;
        offsetStack.push_back(0);
        pos++; // skip '('

        int pCount = 0;
        while (!atEnd() && peek().raw != ")") {
            if (peek().raw == ",") { pos++; continue; }
            Token ptTok = consume();
            if (!typeMap.count(ptTok.raw))
                throw runtime_error("参数类型未知: " + ptTok.raw);
            string pt  = typeMap.at(ptTok.raw);
            string pn  = consume().raw;
            vector<int> dims = parseDims();
            string finalPt   = buildArray(pt, dims);
            int off = offsetStack.back();

            PARAML.push_back({pn, finalPt, "vf", currentLevel, off});
            SYNBL.push_back({pn, finalPt, "v", Addr("PARAML", (int)PARAML.size()-1)});
            VALL.push_back({pn, currentLevel, off});
            offsetStack.back() += getTypeLength(finalPt);
            pCount++;
        }
        PFINFL[pfIdx].PARAM_COUNT = pCount;
        expect(")");
        match("{");
    }

    // 解析普通变量声明（已拿到类型 base 和名称 name）
    void parseVariable(const string &base, const string &name) {
        vector<int> dims = parseDims();
        string finalT = buildArray(base, dims);
        int off = offsetStack.back();
        SYNBL.push_back({name, finalT, "v", Addr("VALL", (int)VALL.size())});
        VALL.push_back({name, currentLevel, off});
        offsetStack.back() += getTypeLength(finalT);
        match(";");
    }

public:
    void analyze(const string &code) {
        IT.clear(); // 每次分析前清空全局标识符表

        try {
            tokens = scanner(code);
        } catch (const exception &e) {
            cerr << "[词法错误] " << e.what() << "\n";
            return;
        }

        int currentFuncIdx = -1;

        try {
            while (!atEnd()) {
                Token t = peek();

                // struct 定义
                if (t.type == 'K' && t.raw == "struct") {
                    consume();
                    parseStruct();
                    continue;
                }

                // 类型关键字开头 → 函数或变量
                if (t.type == 'K' && typeMap.count(t.raw)) {
                    string base = typeMap.at(t.raw);
                    consume();
                    string name = consume().raw;

                    if (!atEnd() && peek().raw == "(") {
                        // 函数
                        parseFunction(base, name);
                        currentFuncIdx = (int)PFINFL.size() - 1;
                    } else {
                        // 变量（支持同行多个：int a, b, c;）
                        parseVariable(base, name);
                        // 同类型多变量声明
                        while (!atEnd() && peek().raw == ",") {
                            consume(); // ','
                            string nextName = consume().raw;
                            parseVariable(base, nextName);
                        }
                    }
                    continue;
                }

                // 用户自定义类型（struct 类型名）作变量声明
                if (t.type == 'I' && typeMap.count(t.raw)) {
                    string base = typeMap.at(t.raw);
                    consume();
                    string name = consume().raw;
                    parseVariable(base, name);
                    continue;
                }

                // 块结束
                if (t.raw == "}") {
                    if (currentFuncIdx != -1) {
                        PFINFL[currentFuncIdx].TOTAL_LEN = offsetStack.back();
                        currentFuncIdx = -1;
                    }
                    currentLevel--;
                    if (!offsetStack.empty()) offsetStack.pop_back();
                    consume();
                    continue;
                }

                // 其余 token 跳过（语句体）
                consume();
            }
        } catch (const exception &e) {
            cerr << "[语法/语义错误] " << e.what()
                 << " (位置: " << pos << ")\n";
        }
    }

    // ================================================================
    // 格式化输出
    // ================================================================

    void printAll() const {
        auto ruler = [](int w = 70) { return string(w, '-'); };
        auto header = [&](const string &s) {
            cout << "\n[ " << s << " ]\n" << ruler() << "\n";
        };

        // KT
        header("KT - 关键字表");
        for (auto &[k, v] : KT)
            cout << setw(3) << v << ": " << left << setw(10) << k;
        cout << "\n";

        // BT
        header("BT - 界符表");
        int col = 0;
        for (auto &[k, v] : BT) {
            cout << setw(3) << v << ": " << left << setw(6) << k;
            if (++col % 8 == 0) cout << "\n";
        }
        cout << "\n";

        // IT
        header("IT - 标识符表");
        for (int i = 0; i < (int)IT.size(); i++)
            cout << i << ":" << IT[i] << "  ";
        cout << "\n";

        // Token 串
        header("Token 串（二元组形式）");
        for (auto &tk : tokens) cout << tk.toString() << " ";
        cout << "\n";

        // SYNBL
        header("SYNBL - 主符号表");
        cout << left << setw(14) << "NAME"
                     << setw(18) << "TYPE"
                     << setw(8)  << "CAT"
                     << "ADDR\n";
        cout << ruler() << "\n";
        for (auto &s : SYNBL)
            cout << setw(14) << s.NAME
                 << setw(18) << s.TYPE
                 << setw(8)  << s.CAT
                 << s.ADDR.toString() << "\n";

        // PFINFL
        header("PFINFL - 函数属性表");
        cout << left << setw(12) << "NAME"
                     << setw(8)  << "LEVEL"
                     << setw(12) << "TOTAL_LEN"
                     << setw(10) << "PARAM_N"
                     << "ENTRY\n";
        cout << ruler() << "\n";
        for (auto &p : PFINFL)
            cout << setw(12) << p.NAME
                 << setw(8)  << p.LEVEL
                 << setw(12) << p.TOTAL_LEN
                 << setw(10) << p.PARAM_COUNT
                 << "PARAML[" << p.ENTRY_INDEX << "]\n";

        // TYPEL
        header("TYPEL - 类型表");
        cout << left << setw(6) << "IDX"
                     << setw(10) << "TVAL"
                     << setw(12) << "TPOINT"
                     << "LEN\n";
        cout << ruler() << "\n";
        for (int i = 0; i < (int)TYPEL.size(); i++)
            cout << setw(6)  << i
                 << setw(10) << TYPEL[i].TVAL
                 << setw(12) << TYPEL[i].TPOINT
                 << TYPEL[i].LEN << "\n";

        // AINFL
        header("AINFL - 数组信息表");
        cout << left << setw(6) << "IDX"
                     << setw(8) << "SIZE"
                     << setw(18) << "CTP"
                     << "CLEN\n";
        cout << ruler() << "\n";
        for (int i = 0; i < (int)AINFL.size(); i++)
            cout << setw(6)  << i
                 << setw(8)  << AINFL[i].SIZE
                 << setw(18) << AINFL[i].CTP
                 << AINFL[i].CLEN << "\n";

        // RINFL
        header("RINFL - 结构体成员表");
        cout << left << setw(14) << "ID"
                     << setw(8)  << "OFF"
                     << "TP\n";
        cout << ruler() << "\n";
        for (auto &r : RINFL)
            cout << setw(14) << r.ID
                 << setw(8)  << r.OFF
                 << r.TP << "\n";

        // VALL
        header("VALL - 活动记录（变量偏移量）");
        cout << left << setw(14) << "NAME"
                     << setw(8)  << "LEVEL"
                     << "OFFSET\n";
        cout << ruler() << "\n";
        for (auto &v : VALL)
            cout << setw(14) << v.NAME
                 << setw(8)  << v.LEVEL
                 << v.OFFSET << "\n";

        // PARAML
        header("PARAML - 形参表");
        cout << left << setw(14) << "NAME"
                     << setw(18) << "TYPE"
                     << setw(8)  << "KIND"
                     << setw(8)  << "LEVEL"
                     << "OFFSET\n";
        cout << ruler() << "\n";
        for (auto &p : PARAML)
            cout << setw(14) << p.NAME
                 << setw(18) << p.TYPE
                 << setw(8)  << p.KIND
                 << setw(8)  << p.LEVEL
                 << p.OFFSET << "\n";
    }
};

// ================================================================
// 主函数：多用例测试
// ================================================================

int main() {
    struct TestCase { string label; string code; };

    vector<TestCase> cases = {
        {
            "基本变量 + 数组 + 函数",
            "int a[10]; float b; int F1(int p, float q) { int x; char c; }"
        },
        {
            "结构体定义与使用",
            "struct Node { int val; float weight; char tag; }; "
            "int buildTree(int n) { Node root; int depth; }"
        },
        {
            "多维数组",
            "int matrix[3][4]; double cube[2][3][4];"
        },
        {
            "自定义测试（原始用例）",
            "int a[10]; struct Node { int x; float y; }; int F1(int p) { int q; }"
        }
    };

    for (auto &tc : cases) {
        cout << "\n" << string(72, '=') << "\n";
        cout << "  测试: " << tc.label << "\n";
        cout << "  代码: " << tc.code << "\n";
        cout << string(72, '=') << "\n";

        SymbolTableSystem sts;
        sts.analyze(tc.code);
        sts.printAll();
    }

    return 0;
}