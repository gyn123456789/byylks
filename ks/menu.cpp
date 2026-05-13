/*
 * ================================================================
 *  C 语言子集 → 四元式生成器
 *  词法分析：复用原有 scanner / KT / BT / IT
 *  语法分析：递归子程序法（手写递归下降）
 *  中间代码：四元式  (op, arg1, arg2, result)
 *
 *  支持语法子集：
 *    - 全局/局部变量声明（int/float/double/char，含数组）
 *    - struct 定义
 *    - 函数定义（含形参）
 *    - 赋值语句
 *    - if / if-else
 *    - while
 *    - for
 *    - return
 *    - 函数调用（作表达式或语句）
 *    - 算术表达式（+  -  *  /，支持括号、负号）
 *    - 关系 / 逻辑布尔表达式（< > <= >= == != && ||  !）
 *    - 数组下标（一维/多维）
 * ================================================================
 */

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
// ① 词法分析（与原代码完全一致）
// ================================================================

const map<string, int> KT = {
    {"int",1},{"float",2},{"double",3},{"char",4},{"bool",5},
    {"if",6},{"else",7},{"while",8},{"for",9},{"return",10},
    {"void",11},{"struct",12}
};
const map<string, int> BT = {
    {"+",1},{"-",2},{"*",3},{"/",4},{"=",5},{"==",6},
    {"<",7},{">",8},{"(",9},{")",10},{"{",11},{"}",12},
    {";",13},{",",14},{"[",15},{"]",16},{"!=",17},{"<=",18},
    {">=",19},{"&&",20},{"||",21}
};
vector<string> IT; // 标识符表（全局）

struct Token {
    char   type;  // 'K' 'B' 'I' 'C'
    int    id;
    string raw;
    string toString() const {
        return string("(") + type + "," + to_string(id) + ")";
    }
};

int getOrInsertIdent(const string &s) {
    auto it = find(IT.begin(), IT.end(), s);
    if (it != IT.end()) return (int)distance(IT.begin(), it);
    IT.push_back(s);
    return (int)IT.size() - 1;
}

vector<Token> scanner(const string &code) {
    vector<Token> tokens;
    int i = 0, len = (int)code.size();
    while (i < len) {
        if (isspace((unsigned char)code[i])) { i++; continue; }
        if (i+1<len && code[i]=='/' && code[i+1]=='/') {
            while (i<len && code[i]!='\n') i++; continue;
        }
        if (i+1<len && code[i]=='/' && code[i+1]=='*') {
            i+=2;
            while (i+1<len && !(code[i]=='*' && code[i+1]=='/')) i++;
            i+=2; continue;
        }
        if (isalpha((unsigned char)code[i]) || code[i]=='_') {
            string s;
            while (i<len && (isalnum((unsigned char)code[i])||code[i]=='_')) s+=code[i++];
            if (KT.count(s)) tokens.push_back({'K', KT.at(s), s});
            else              tokens.push_back({'I', getOrInsertIdent(s), s});
            continue;
        }
        if (isdigit((unsigned char)code[i])) {
            string s;
            while (i<len && isdigit((unsigned char)code[i])) s+=code[i++];
            if (i<len && code[i]=='.') {
                s+=code[i++];
                while (i<len && isdigit((unsigned char)code[i])) s+=code[i++];
            }
            tokens.push_back({'C', (int)stod(s), s});
            continue;
        }
        {
            string two = (i+1<len) ? string{code[i],code[i+1]} : "";
            string one(1, code[i]);
            if (!two.empty() && BT.count(two)) { tokens.push_back({'B', BT.at(two), two}); i+=2; }
            else if (BT.count(one))             { tokens.push_back({'B', BT.at(one), one}); i++;  }
            else { cerr<<"[词法警告] 未识别字符:'"<<code[i]<<"'\n"; i++; }
        }
    }
    return tokens;
}

// ================================================================
// ② 四元式数据结构
// ================================================================

struct Quad {
    string op, arg1, arg2, result;
    Quad(string o, string a1, string a2, string r)
        : op(move(o)), arg1(move(a1)), arg2(move(a2)), result(move(r)) {}
};

// ================================================================
// ③ 四元式管理器
// ================================================================

class QuadManager {
public:
    vector<Quad> quads;
    int tempCount = 0;

    // 生成新临时变量 t1, t2, ...
    string newTemp() { return "t" + to_string(++tempCount); }

    // 生成一条四元式，返回其编号（从 100 开始）
    int gen(const string &op, const string &a1,
            const string &a2, const string &r) {
        quads.push_back({op, a1, a2, r});
        return (int)quads.size() - 1 + 100;
    }

    // 下一条四元式编号
    int nextQuad() const { return (int)quads.size() + 100; }

    // 回填：将 list 中所有编号对应四元式的 result 填为 target
    void backpatch(const vector<int> &list, int target) {
        for (int idx : list) {
            quads[idx - 100].result = to_string(target);
        }
    }

    // 合并两个待回填链表
    static vector<int> merge(const vector<int> &a, const vector<int> &b) {
        vector<int> res = a;
        res.insert(res.end(), b.begin(), b.end());
        return res;
    }

    // 打印所有四元式
    void print() const {
        cout << "\n[ 四元式序列 ]\n" << string(60,'-') << "\n";
        cout << left << setw(6) << "编号"
                     << setw(8) << "op"
                     << setw(12)<< "arg1"
                     << setw(12)<< "arg2"
                     << "result\n";
        cout << string(60,'-') << "\n";
        for (int i = 0; i < (int)quads.size(); i++) {
            const Quad &q = quads[i];
            cout << left << setw(6) << (i+100)
                         << setw(8) << q.op
                         << setw(12)<< q.arg1
                         << setw(12)<< q.arg2
                         << q.result << "\n";
        }
    }
};

// ================================================================
// ④ 语法分析器（递归子程序）+ 语义动作
// ================================================================

/*
  每个子程序对应一个文法符号：

    parseProgram     → DeclList StmtList（顶层）
    parseDecl        → 变量声明 / 函数定义 / struct定义
    parseStmt        → 语句（if/while/for/return/赋值/调用/块）
    parseExpr        → 赋值表达式或普通表达式（入口）
    parseAssignExpr  → E（含赋值右结合）
    parseLogOr       → B: || 层
    parseLogAnd      → B: && 层
    parseRelation    → B: 关系运算
    parseAddSub      → E: 加减
    parseMulDiv      → E: 乘除
    parseUnary       → E: 一元 -  !
    parsePostfix     → E: 后缀（数组下标、函数调用）
    parsePrimary     → E: 原子（常数、标识符、括号）

  布尔表达式属性（用于回填）：
    struct BExpr { vector<int> truelist, falselist; }

  普通表达式属性：
    string place   — 存放值的变量名或临时变量名
*/

// 布尔表达式综合属性
struct BExpr {
    vector<int> truelist;   // 条件为真时待回填的四元式编号列表
    vector<int> falselist;  // 条件为假时待回填的四元式编号列表
};

// 类型信息（简化）
struct TypeInfo {
    string tval;  // "itp","rtp","ctp","dtp","void","TYPEL:N"
    int    width; // 字节宽度
};

// 符号表条目（简化，只保留类型与偏移）
struct VarEntry {
    string name;
    TypeInfo type;
    int offset;
    int level;
    bool isParam;
};

// ----------------------------------------------------------------
class Parser {
public:
    explicit Parser(QuadManager &qm) : qm(qm) {}

    // 对外接口
    void parse(const string &code) {
        IT.clear();
        tokens = scanner(code);
        pos = 0;
        parseProgram();
    }

private:
    QuadManager       &qm;
    vector<Token>      tokens;
    int                pos        = 0;
    int                tempVarCnt = 0;
    int                labelCnt   = 0;

    // 作用域：层次 → 符号列表（简化：单层 map）
    // 实际可扩展为栈式符号表
    vector<map<string,VarEntry>> scopeStack;
    int currentLevel = 0;

    // 当前函数返回类型（用于 return 检查）
    string currentRetType;

    // 用户自定义类型（struct名 → tval字符串）
    map<string, string> userTypes;

    // ---- Token 操作 ----

    bool atEnd() const { return pos >= (int)tokens.size(); }

    Token peek(int ahead = 0) const {
        int idx = pos + ahead;
        if (idx >= (int)tokens.size()) return {'?', -1, ""};
        return tokens[idx];
    }

    Token consume() {
        if (atEnd()) throw runtime_error("意外的 Token 结束");
        return tokens[pos++];
    }

    bool check(const string &raw, int ahead = 0) const {
        return peek(ahead).raw == raw;
    }

    bool matchTok(const string &raw) {
        if (!atEnd() && peek().raw == raw) { pos++; return true; }
        return false;
    }

    void expect(const string &raw) {
        if (!matchTok(raw))
            throw runtime_error("期望 '" + raw + "' 但得到 '" + peek().raw + "' (pos=" + to_string(pos) + ")");
    }

    // ---- 作用域 ----

    void pushScope() {
        scopeStack.push_back({});
        currentLevel++;
    }

    void popScope() {
        if (!scopeStack.empty()) scopeStack.pop_back();
        currentLevel--;
    }

    void declareVar(const string &name, TypeInfo ti, int offset = 0) {
        if (scopeStack.empty()) scopeStack.push_back({});
        scopeStack.back()[name] = {name, ti, offset, currentLevel, false};
    }

    // 查找变量（从内层向外）
    VarEntry* lookupVar(const string &name) {
        for (int i = (int)scopeStack.size()-1; i >= 0; i--) {
            auto it = scopeStack[i].find(name);
            if (it != scopeStack[i].end()) return &it->second;
        }
        return nullptr;
    }

    // ---- 类型辅助 ----

    bool isTypeName(const string &s) const {
        return (s=="int"||s=="float"||s=="double"||s=="char"||
                s=="bool"||s=="void") || userTypes.count(s);
    }

    TypeInfo baseTypeInfo(const string &s) {
        if (s=="int"  ||s=="bool") return {"itp",4};
        if (s=="float")            return {"rtp",4};
        if (s=="double")           return {"dtp",8};
        if (s=="char")             return {"ctp",1};
        if (s=="void")             return {"void",0};
        if (userTypes.count(s))    return {userTypes.at(s), 0};
        return {"itp",4};
    }

    // ====================================================
    // 1. 程序顶层
    //    Program → { Decl | FuncDef | Stmt }*
    // ====================================================
    void parseProgram() {
        pushScope();
        while (!atEnd()) {
            // struct 定义
            if (peek().raw == "struct") {
                parseStructDecl();
                continue;
            }
            // 类型开头 → 变量声明 或 函数定义
            if (peek().type=='K' && isTypeName(peek().raw)) {
                parseDeclOrFunc();
                continue;
            }
            // 用户自定义类型作变量声明
            if (peek().type=='I' && isTypeName(peek().raw)) {
                parseDeclOrFunc();
                continue;
            }
            // 其他：语句
            parseStmt();
        }
        popScope();
    }

    // ====================================================
    // 2. struct 定义
    //    StructDecl → struct id { FieldList } ;
    // ====================================================
    void parseStructDecl() {
        expect("struct");
        string sName = consume().raw;   // struct 名
        expect("{");
        // 只记录类型名，不生成四元式（类型信息）
        while (!atEnd() && !check("}")) {
            // 跳过成员声明（词法跳过至 ;）
            while (!atEnd() && !check(";") && !check("}")) consume();
            matchTok(";");
        }
        expect("}");
        matchTok(";");
        userTypes[sName] = "struct_" + sName;
    }

    // ====================================================
    // 3. 声明 / 函数定义
    //    DeclOrFunc → Type id ( ParamList ) Block   -- 函数定义
    //               | Type id ArrayDims ;            -- 变量声明
    // ====================================================
    void parseDeclOrFunc() {
        // 读类型
        string typeName = consume().raw;
        TypeInfo ti = baseTypeInfo(typeName);

        // 读名字
        string name = consume().raw;

        if (check("(")) {
            // === 函数定义 ===
            parseFuncDef(ti, name);
        } else {
            // === 变量声明（支持同行多个：int a,b,c[5];）===
            parseVarDeclTail(ti, name);
            while (matchTok(",")) {
                string nextName = consume().raw;
                parseVarDeclTail(ti, nextName);
            }
            matchTok(";");
        }
    }

    // 处理变量声明后半（可能有数组维度，然后可能有初始值）
    // VarDeclTail → ArrayDims [ = E ]
    void parseVarDeclTail(TypeInfo ti, const string &name) {
        // 数组维度（略去：只跳过 [N] 记录类型）
        TypeInfo finalTi = ti;
        while (check("[")) {
            consume(); // [
            string sz = consume().raw;
            consume(); // ]
            finalTi.width *= stoi(sz);
            finalTi.tval   = "array_" + finalTi.tval;
        }
        declareVar(name, finalTi);

        // 可选初始化：= E
        if (matchTok("=")) {
            string rhs = parseAssignExpr();
            // 动作：gen('=', rhs, _, name)
            qm.gen("=", rhs, "_", name);
        }
    }

    // ====================================================
    // 4. 函数定义
    //    FuncDef → Type id ( ParamList ) Block
    // ====================================================
    void parseFuncDef(TypeInfo retTi, const string &name) {
        // 动作：entry 标记
        // gen('entry', name, _, _)
        qm.gen("entry", name, "_", "_");

        currentRetType = retTi.tval;
        pushScope();

        expect("(");
        parseParamList();
        expect(")");

        // 函数体
        expect("{");
        parseStmtList();
        expect("}");

        // 动作：exit 标记
        // gen('exit', name, _, _)
        qm.gen("exit", name, "_", "_");

        popScope();
    }

    // ====================================================
    // 5. 形参列表
    //    ParamList → ε | Param { , Param }
    //    Param     → Type id ArrayDims
    // ====================================================
    void parseParamList() {
        if (check(")")) return;
        parseParam();
        while (matchTok(",")) parseParam();
    }

    void parseParam() {
        string typeName = consume().raw;
        TypeInfo ti = baseTypeInfo(typeName);
        string pName = consume().raw;
        // 数组形参（跳过维度）
        while (check("[")) { consume(); consume(); consume(); }
        declareVar(pName, ti);
        // 动作：param 声明（此处只登记符号表，不生成四元式）
    }

    // ====================================================
    // 6. 语句列表
    //    StmtList → { Stmt }*
    // ====================================================

    // 返回 nextlist（供上层回填）
    vector<int> parseStmtList() {
        vector<int> nextlist;
        while (!atEnd() && !check("}")) {
            vector<int> sl = parseStmt();
            // 若当前语句有 nextlist，记录下来（简化：末尾合并）
            nextlist = QuadManager::merge(nextlist, sl);
        }
        return nextlist;
    }

    // ====================================================
    // 7. 单条语句
    //    Stmt → IfStmt | WhileStmt | ForStmt | ReturnStmt
    //         | Block | DeclStmt | ExprStmt
    // ====================================================
    vector<int> parseStmt() {
        Token t = peek();

        // if
        if (t.raw == "if")     return parseIfStmt();
        // while
        if (t.raw == "while")  return parseWhileStmt();
        // for
        if (t.raw == "for")    return parseForStmt();
        // return
        if (t.raw == "return") return parseReturnStmt();
        // 块
        if (t.raw == "{") {
            consume();
            pushScope();
            auto nl = parseStmtList();
            expect("}");
            popScope();
            return nl;
        }
        // struct 定义（局部）
        if (t.raw == "struct") {
            parseStructDecl();
            return {};
        }
        // 类型开头 → 局部变量声明
        if ((t.type=='K'||t.type=='I') && isTypeName(t.raw)) {
            parseDeclOrFunc();
            return {};
        }
        // 表达式语句
        return parseExprStmt();
    }

    // ====================================================
    // 8. if 语句
    //    S → if ( B ) M1 S1 N else M2 S2
    //      | if ( B ) M1 S1
    //
    //  动作：
    //    M.quad = nextQuad()           ← 记录位置
    //    backpatch(B.truelist,  M1.quad)
    //    backpatch(B.falselist, M2.quad)  （有else时）
    //    backpatch(B.falselist, nextQuad()) （无else时）
    //    N: gen('j',_,_,0) → N.nextlist
    // ====================================================
    vector<int> parseIfStmt() {
        expect("if");
        expect("(");

        // 解析布尔表达式，获取 truelist / falselist
        BExpr be = parseBoolExpr();

        expect(")");

        // M1: 记录 then 分支入口
        int m1 = qm.nextQuad();
        // 动作：backpatch(B.truelist, m1)
        qm.backpatch(be.truelist, m1);

        // 解析 then 分支
        vector<int> s1next = parseStmt();

        if (check("else")) {
            // N: gen('j', _, _, 0)，结束 then 分支后跳过 else
            int nQuad = qm.gen("j", "_", "_", "0");
            vector<int> nlist = {nQuad};

            // M2: 记录 else 分支入口
            int m2 = qm.nextQuad();
            // 动作：backpatch(B.falselist, m2)
            qm.backpatch(be.falselist, m2);

            consume(); // else

            vector<int> s2next = parseStmt();

            // S.nextlist = merge(s1next, merge(nlist, s2next))
            return QuadManager::merge(
                       QuadManager::merge(s1next, nlist), s2next);
        } else {
            // 无 else：B.falselist 和 s1next 都是 nextlist
            // 动作：backpatch(B.falselist, nextQuad())  ← 延迟，由调用者处理
            return QuadManager::merge(be.falselist, s1next);
        }
    }

    // ====================================================
    // 9. while 语句
    //    S → while M1 ( B ) M2 S1
    //
    //  动作：
    //    M1.quad = nextQuad()
    //    backpatch(S1.nextlist, M1.quad)
    //    backpatch(B.truelist,  M2.quad)
    //    gen('j', _, _, M1.quad)
    //    S.nextlist = B.falselist
    // ====================================================
    vector<int> parseWhileStmt() {
        expect("while");

        // M1：记录循环条件入口
        int m1 = qm.nextQuad();

        expect("(");
        BExpr be = parseBoolExpr();
        expect(")");

        // M2：循环体入口
        int m2 = qm.nextQuad();
        // 动作：backpatch(B.truelist, m2)
        qm.backpatch(be.truelist, m2);

        // 循环体
        vector<int> s1next = parseStmt();

        // 动作：backpatch(S1.nextlist, m1)  — continue 回到循环头
        qm.backpatch(s1next, m1);

        // 动作：gen('j', _, _, m1)  — 无条件跳回
        qm.gen("j", "_", "_", to_string(m1));

        // S.nextlist = B.falselist
        return be.falselist;
    }

    // ====================================================
    // 10. for 语句
    //     S → for ( E1 ; M1 B ; M2 E2 M3 ) S1
    //
    //  动作：
    //    执行 E1
    //    M1.quad = nextQuad()
    //    if B.truelist → M3（循环体入口）
    //    if B.falselist → 出口
    //    M2.quad = nextQuad()  ← 步进表达式入口
    //    执行 E2
    //    gen('j', _, _, M1.quad)  ← 跳回条件
    //    M3.quad = nextQuad()
    //    执行 S1
    //    backpatch(S1.nextlist, M2.quad)  ← continue → 步进
    //    gen('j', _, _, M1.quad)  ← 循环末尾跳回条件
    //    S.nextlist = B.falselist
    // ====================================================
    vector<int> parseForStmt() {
        expect("for");
        expect("(");

        // E1（初始化）
        if (!check(";")) parseAssignExpr();
        expect(";");

        // M1：条件入口
        int m1 = qm.nextQuad();

        // B（条件）
        BExpr be;
        if (!check(";")) {
            be = parseBoolExpr();
        } else {
            // 空条件 → 恒真，gen('j', _, _, 0) 占位
            int q = qm.gen("j", "_", "_", "0");
            be.truelist = {q};
        }
        expect(";");

        // M2：步进表达式入口
        // 先生成无条件跳到 M3（循环体）的跳转，等待回填
        int skipStep = qm.gen("j", "_", "_", "0");  // 跳过步进→直接进循环体

        int m2 = qm.nextQuad();
        // 步进表达式 E2
        if (!check(")")) parseAssignExpr();
        // 步进结束后跳回 M1
        qm.gen("j", "_", "_", to_string(m1));

        // M3：循环体入口
        int m3 = qm.nextQuad();
        // 回填：skipStep → m3，truelist → m3
        qm.backpatch({skipStep}, m3);
        qm.backpatch(be.truelist, m3);

        expect(")");

        // 循环体
        vector<int> s1next = parseStmt();

        // backpatch(S1.nextlist, m2)  — continue → 步进
        qm.backpatch(s1next, m2);

        // 循环末尾跳回条件
        qm.gen("j", "_", "_", to_string(m1));

        // S.nextlist = B.falselist
        return be.falselist;
    }

    // ====================================================
    // 11. return 语句
    //     S → return E ; | return ;
    //  动作：gen('ret', E.place, _, _)
    // ====================================================
    vector<int> parseReturnStmt() {
        expect("return");
        if (!check(";")) {
            string place = parseAssignExpr();
            // 动作：gen('ret', place, _, _)
            qm.gen("ret", place, "_", "_");
        } else {
            // 动作：gen('ret', _, _, _)
            qm.gen("ret", "_", "_", "_");
        }
        expect(";");
        return {};
    }

    // ====================================================
    // 12. 表达式语句
    //     ExprStmt → E ; | ;
    // ====================================================
    vector<int> parseExprStmt() {
        if (check(";")) { consume(); return {}; }
        parseAssignExpr();
        matchTok(";");
        return {};
    }

    // ====================================================
    // ============ 表达式递归子程序 ====================
    // ====================================================

    // ====================================================
    // 13. 赋值表达式（右结合）
    //     AssignExpr → PostfixExpr = AssignExpr
    //                | LogOr
    //
    //  动作：gen('=', rhs, _, lhs.place)
    // ====================================================
    string parseAssignExpr() {
        // 尝试识别赋值：id[...] = E  或  id = E
        // 简单策略：先解析加减，若下一个是 '=' 则视为赋值
        string lhs = parseLogOr_asPlace();

        if (check("=") && peek(0).raw == "=") {
            consume(); // =
            string rhs = parseAssignExpr();
            // 动作：gen('=', rhs, _, lhs)
            qm.gen("=", rhs, "_", lhs);
            return lhs;
        }
        return lhs;
    }

    // ====================================================
    // 布尔表达式（返回 BExpr 含 truelist/falselist）
    // 入口：parseBoolExpr
    // ====================================================

    // 将普通表达式位置转成 BExpr（用于非比较布尔值）
    BExpr placeToB(const string &place) {
        // gen('j!=', place, '0', 0)  → truelist
        // gen('j',   _,    _,  0)  → falselist
        int qt = qm.gen("j!=", place, "0", "0");
        int qf = qm.gen("j",   "_",   "_", "0");
        return {{qt}, {qf}};
    }

    // 布尔表达式总入口（可能包含 ||）
    BExpr parseBoolExpr() {
        return parseLogOr();
    }

    // ====================================================
    // 14. || 层（短路）
    //     B → B1 || M B2
    //  动作：
    //    backpatch(B1.falselist, M.quad)
    //    B.truelist  = merge(B1.truelist,  B2.truelist)
    //    B.falselist = B2.falselist
    // ====================================================
    BExpr parseLogOr() {
        BExpr b = parseLogAnd();
        while (check("||")) {
            consume();
            // M：记录 B2 入口
            int m = qm.nextQuad();
            // 动作：backpatch(b.falselist, m)
            qm.backpatch(b.falselist, m);

            BExpr b2 = parseLogAnd();
            b.truelist  = QuadManager::merge(b.truelist,  b2.truelist);
            b.falselist = b2.falselist;
        }
        return b;
    }

    // ====================================================
    // 15. && 层（短路）
    //     B → B1 && M B2
    //  动作：
    //    backpatch(B1.truelist, M.quad)
    //    B.truelist  = B2.truelist
    //    B.falselist = merge(B1.falselist, B2.falselist)
    // ====================================================
    BExpr parseLogAnd() {
        BExpr b = parseNot();
        while (check("&&")) {
            consume();
            int m = qm.nextQuad();
            // 动作：backpatch(b.truelist, m)
            qm.backpatch(b.truelist, m);

            BExpr b2 = parseNot();
            b.falselist = QuadManager::merge(b.falselist, b2.falselist);
            b.truelist  = b2.truelist;
        }
        return b;
    }

    // ====================================================
    // 16. ! 层
    //     B → ! B1
    //  动作：swap truelist / falselist
    // ====================================================
    BExpr parseNot() {
        if (matchTok("!")) {
            BExpr b = parseNot();
            swap(b.truelist, b.falselist);
            return b;
        }
        return parseRelation();
    }

    // ====================================================
    // 17. 关系运算层
    //     B → E1 relop E2
    //  动作：
    //    gen('j'+relop, E1.place, E2.place, 0) → truelist
    //    gen('j', _, _, 0)                     → falselist
    // ====================================================
    BExpr parseRelation() {
        string e1 = parseAddSub();
        static const vector<string> relops = {"==","!=","<",">","<=",">="};
        string op = peek().raw;
        bool isRel = find(relops.begin(), relops.end(), op) != relops.end();
        if (isRel) {
            consume();
            string e2 = parseAddSub();
            // 动作
            int qt = qm.gen("j" + op, e1, e2, "0"); // 待回填
            int qf = qm.gen("j",      "_","_","0");  // 待回填
            return {{qt}, {qf}};
        }
        // 非关系表达式 → 转成布尔
        return placeToB(e1);
    }

    // 辅助：parseLogOr 但返回 place（用于赋值右侧）
    // 若表达式是纯算术则直接返回 place
    // 若包含布尔运算则将结果物化到临时变量
    string parseLogOr_asPlace() {
        // 先尝试 parseAddSub，若下一个不是 relop / && / || / = 则直接返回
        string e = parseAddSub();
        // 检查是否后面有关系/逻辑运算
        static const vector<string> boolOps = {"==","!=","<",">","<=",">=","&&","||"};
        string nxt = peek().raw;
        bool isBool = find(boolOps.begin(), boolOps.end(), nxt) != boolOps.end();
        if (!isBool) return e; // 纯算术

        // 将 e 重新包装回来，继续解析布尔层
        // （简化：把已解析的 e 作为 relation 左侧，继续匹配）
        string op = nxt;
        if (op=="==" || op=="!=" || op=="<" || op==">" || op=="<=" || op==">=") {
            consume();
            string e2 = parseAddSub();
            int qt = qm.gen("j"+op, e, e2, "0");
            int qf = qm.gen("j",    "_","_","0");
            BExpr be{{qt},{qf}};
            // 继续 && / ||
            be = continueLogAnd(be);
            be = continueLogOr(be);
            return materialize(be);
        }
        // 若直接遇到 && / ||，将 e 视为布尔值
        BExpr be = placeToB(e);
        be = continueLogAnd(be);
        be = continueLogOr(be);
        return materialize(be);
    }

    // 将布尔表达式物化为 0/1 临时变量
    string materialize(BExpr &be) {
        string t = qm.newTemp();
        int trueEntry  = qm.nextQuad();
        qm.backpatch(be.truelist, trueEntry);
        qm.gen("=", "1", "_", t);
        int skipFalse = qm.gen("j", "_", "_", "0");
        int falseEntry = qm.nextQuad();
        qm.backpatch(be.falselist, falseEntry);
        qm.gen("=", "0", "_", t);
        qm.backpatch({skipFalse}, qm.nextQuad());
        return t;
    }

    // 继续解析 && 链（在已有 BExpr 基础上）
    BExpr continueLogAnd(BExpr b) {
        while (check("&&")) {
            consume();
            int m = qm.nextQuad();
            qm.backpatch(b.truelist, m);
            BExpr b2 = parseNot();
            b.falselist = QuadManager::merge(b.falselist, b2.falselist);
            b.truelist  = b2.truelist;
        }
        return b;
    }

    // 继续解析 || 链（在已有 BExpr 基础上）
    BExpr continueLogOr(BExpr b) {
        while (check("||")) {
            consume();
            int m = qm.nextQuad();
            qm.backpatch(b.falselist, m);
            BExpr b2 = parseLogAnd();
            b.truelist  = QuadManager::merge(b.truelist,  b2.truelist);
            b.falselist = b2.falselist;
        }
        return b;
    }

    // ====================================================
    // 18. 加减层
    //     E → E + T | E - T | T
    //  动作：
    //    t = newTemp()
    //    gen('+'/'-', E.place, T.place, t)
    //    E.place = t
    // ====================================================
    string parseAddSub() {
        string e = parseMulDiv();
        while (check("+") || check("-")) {
            string op = consume().raw;
            string t2 = parseMulDiv();
            string tmp = qm.newTemp();
            // 动作：gen(op, e, t2, tmp)
            qm.gen(op, e, t2, tmp);
            e = tmp;
        }
        return e;
    }

    // ====================================================
    // 19. 乘除层
    //     T → T * F | T / F | F
    //  动作：同加减
    // ====================================================
    string parseMulDiv() {
        string t = parseUnary();
        while (check("*") || check("/")) {
            string op = consume().raw;
            string f  = parseUnary();
            string tmp = qm.newTemp();
            // 动作：gen(op, t, f, tmp)
            qm.gen(op, t, f, tmp);
            t = tmp;
        }
        return t;
    }

    // ====================================================
    // 20. 一元运算
    //     U → - F | F
    //  动作：
    //    t = newTemp()
    //    gen('neg', F.place, _, t)
    // ====================================================
    string parseUnary() {
        if (check("-")) {
            consume();
            string f = parsePostfix();
            string tmp = qm.newTemp();
            // 动作：gen('neg', f, _, tmp)
            qm.gen("neg", f, "_", tmp);
            return tmp;
        }
        return parsePostfix();
    }

    // ====================================================
    // 21. 后缀运算（数组下标 / 函数调用）
    //     P → Primary [ E ]          -- 数组取值
    //       | Primary [ E ][ E ]...  -- 多维数组
    //       | Primary ( ArgList )    -- 函数调用
    //       | Primary
    //
    //  数组动作：
    //    t = newTemp()
    //    gen('[]', arr, idx, t)
    //    P.place = t
    //
    //  函数调用动作：
    //    for each arg: gen('param', arg, _, _)
    //    t = newTemp()
    //    gen('call', name, n, t)
    //    P.place = t
    // ====================================================
    string parsePostfix() {
        string base = parsePrimary();

        while (true) {
            if (check("[")) {
                consume(); // [
                string idx = parseAssignExpr();
                expect("]");
                string tmp = qm.newTemp();
                // 动作：gen('[]', base, idx, tmp)
                qm.gen("[]", base, idx, tmp);
                base = tmp;
            } else if (check("(")) {
                // 函数调用
                consume(); // (
                vector<string> args;
                if (!check(")")) {
                    args.push_back(parseAssignExpr());
                    while (matchTok(","))
                        args.push_back(parseAssignExpr());
                }
                expect(")");
                // 动作：逆序 gen('param', arg, _, _)
                for (int i = (int)args.size()-1; i >= 0; i--)
                    qm.gen("param", args[i], "_", "_");
                string tmp = qm.newTemp();
                // 动作：gen('call', base, n, tmp)
                qm.gen("call", base, to_string(args.size()), tmp);
                base = tmp;
            } else {
                break;
            }
        }
        return base;
    }

    // ====================================================
    // 22. 原子表达式
    //     Primary → num | id | ( E )
    // ====================================================
    string parsePrimary() {
        Token t = peek();

        // 数字常量
        if (t.type == 'C') {
            consume();
            return t.raw;
        }

        // 括号表达式
        if (t.raw == "(") {
            consume();
            string e = parseAssignExpr();
            expect(")");
            return e;
        }

        // 标识符
        if (t.type == 'I' || t.type == 'K') {
            consume();
            return t.raw;
        }

        throw runtime_error("parseP: 不是有效的原子表达式, 得到 '" + t.raw + "'");
    }
};

// ================================================================
// ⑤ 主函数：多用例测试
// ================================================================

int main() {
    struct TC { string label, code; };

    vector<TC> cases = {
        {
            "基础赋值 + 算术",
            "int a; int b; int c; a = 1; b = 2; c = a + b * 2;"
        },
        {
            "if-else 语句",
            "int x; int y; int z;"
            "x = 5; y = 3;"
            "if (x < y) { z = x; } else { z = y; }"
        },
        {
            "while 循环",
            "int i; int s;"
            "i = 0; s = 0;"
            "while (i < 10) { s = s + i; i = i + 1; }"
        },
        {
            "for 循环",
            "int i; int sum;"
            "sum = 0;"
            "for (i = 1; i <= 100; i = i + 1) { sum = sum + i; }"
        },
        {
            "函数定义 + 调用 + return",
            "int add(int a, int b) { return a + b; }"
            "int main() { int r; r = add(3, 4); }"
        },
        {
            "数组下标",
            "int arr[10]; int i; int v;"
            "i = 3;"
            "v = arr[i];"
            "arr[i] = v + 1;"
        },
        {
            "复合布尔表达式",
            "int a; int b; int c; int r;"
            "a = 1; b = 2; c = 3;"
            "if (a < b && b < c) { r = 1; } else { r = 0; }"
        },
        {
            "综合用例",
            "int F1(int p, float q) {"
            "  int x; int y;"
            "  x = p + 1;"
            "  if (x > 0) { y = x * q; } else { y = 0; }"
            "  while (x > 0) { x = x - 1; }"
            "  return y;"
            "}"
        }
    };

    for (auto &tc : cases) {
        cout << "\n" << string(68,'=') << "\n";
        cout << "  测试: " << tc.label << "\n";
        cout << "  代码: " << tc.code << "\n";
        cout << string(68,'=') << "\n";

        QuadManager qm;
        Parser      parser(qm);

        try {
            parser.parse(tc.code);
        } catch (const exception &e) {
            cerr << "[错误] " << e.what() << "\n";
        }

        qm.print();
    }

    return 0;
}