#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

using namespace std;

// ======================== 结构定义 ========================
struct Addr {
    string table; int index;
    Addr(string t="", int i=-1) : table(t), index(i) {}
};

struct TypeNode {
    string TVAL; // a: 数组, d: 结构体, itp/rtp/ctp: 基本类型
    int TPOINT;  // 指向 AINFL 或 RINFL 的索引
    int LEN;     // 类型的总字节数
    TypeNode(string tv="", int tp=-1, int len=0) : TVAL(tv), TPOINT(tp), LEN(len) {}
};

struct ArrayInfo {
    int SIZE;    // 元素个数
    string CTP;  // 元素类型描述符
    int CLEN;    // 单个元素长度
    ArrayInfo(int s=0, string c="", int cl=0) : SIZE(s), CTP(c), CLEN(cl) {}
};

struct RecordInfo {
    string ID;   // 成员名
    int OFF;     // 内部偏移量
    string TP;   // 成员类型
    RecordInfo(string id="", int off=0, string tp="") : ID(id), OFF(off), TP(tp) {}
};

struct ValueInfo {
    string NAME; int LEVEL; int OFFSET;
    ValueInfo(string n="", int l=0, int off=0) : NAME(n), LEVEL(l), OFFSET(off) {}
};

struct Symbol {
    string NAME; string TYPE; string CAT; Addr ADDR;
    Symbol(string n="", string t="", string c="", Addr a=Addr()) : NAME(n), TYPE(t), CAT(c), ADDR(a) {}
};

// ====== 参数表节点 (对应图中中间的局部信息/参数表) ======
struct ParamNode {
    string NAME;
    string TYPE;
    string KIND; // vf: 传值 (value formal), vn: 传名/地址 (name formal)
    int LEVEL;
    int OFFSET;
    ParamNode(string n="", string t="", string k="vf", int l=0, int off=0)
        : NAME(n), TYPE(t), KIND(k), LEVEL(l), OFFSET(off) {}
};

// ====== 函数/过程属性表项 (PFINFL) ======
struct ProcInfo {
    string NAME;
    int LEVEL;
    int TOTAL_LEN;     // 局部总分配空间 (LENL)
    int PARAM_COUNT;   
    int ENTRY_INDEX;   // 指向 PARAML 表的首个参数索引
    ProcInfo(string n="", int l=0, int tl=0, int pc=0, int entry=-1) 
        : NAME(n), LEVEL(l), TOTAL_LEN(tl), PARAM_COUNT(pc), ENTRY_INDEX(entry) {}
};

// 用于解析结构体的临时结构
struct StructMemTemp {
    string name; string type; vector<int> dims;
};

// ======================== 符号表系统 ========================
class SymbolTableSystem {
private:
    vector<TypeNode> TYPEL;
    vector<ArrayInfo> AINFL;
    vector<RecordInfo> RINFL;
    vector<ValueInfo> VALL;
    vector<Symbol> SYNBL;
    vector<ProcInfo> PFINFL; 
    vector<ParamNode> PARAML; 

    map<string, string> typeMap = {{"int","itp"}, {"float","rtp"}, {"char","ctp"}};
    
    int currentLevel = 1; 
    vector<int> offsetStack = {0}; // 栈顶为当前作用域的偏移量

    bool isUserType(const string& t) { return typeMap.count(t) && typeMap[t].find("TYPEL:") == 0; }

    int getTypeLength(const string &t) {
        if(t == "itp") return 4;
        if(t == "rtp") return 8;
        if(t == "ctp") return 1;
        
        string actualType = typeMap.count(t) ? typeMap[t] : t;
        if(actualType.find("TYPEL:") == 0) {
            return TYPEL[stoi(actualType.substr(6))].LEN;
        }
        return 0;
    }

    // 构建多维数组类型 (由内向外)
    string buildArrayType(string baseType, const vector<int>& dims) {
        if(dims.empty()) return baseType;
        string currentType = baseType;
        for (int i = (int)dims.size() - 1; i >= 0; --i) {
            int size = dims[i];
            int clen = getTypeLength(currentType);
            AINFL.push_back(ArrayInfo(size, currentType, clen));
            TYPEL.push_back(TypeNode("a", (int)AINFL.size() - 1, size * clen));
            currentType = "TYPEL:" + to_string(TYPEL.size() - 1);
        }
        return currentType;
    }

    void addStructDefinition(const string &sName, const vector<StructMemTemp>& members) {
        int startR = RINFL.size();
        int innerOff = 0;
        for(auto &m : members) {
            string baseType = typeMap.count(m.type) ? typeMap[m.type] : m.type;
            string finalType = buildArrayType(baseType, m.dims);
            RINFL.push_back(RecordInfo(m.name, innerOff, finalType));
            innerOff += getTypeLength(finalType);
        }
        TYPEL.push_back(TypeNode("d", startR, innerOff));
        int typeIdx = (int)TYPEL.size() - 1;
        SYNBL.push_back(Symbol(sName, "TYPEL:"+to_string(typeIdx), "t", Addr("TYPEL", typeIdx)));
        typeMap[sName] = "TYPEL:" + to_string(typeIdx);
    }

    // 将源码转化为规范的 Token 流，分离符号与标识符
    vector<string> tokenize(const string& code) {
        string res = "";
        for(char c : code) {
            if(string("()[]{};,").find(c) != string::npos) {
                res += string(" ") + c + " ";
            } else {
                res += c;
            }
        }
        stringstream ss(res);
        string tk;
        vector<string> tokens;
        while(ss >> tk) tokens.push_back(tk);
        return tokens;
    }

public:
    void analyzeCode(const string &code) {
        vector<string> tokens = tokenize(code);
        int i = 0;
        int currentFuncIdx = -1;

        while(i < tokens.size()) {
            string tk = tokens[i];

            // 1. 处理结构体
            if (tk == "struct") {
                string sName = tokens[++i];
                i += 2; // 跳过 sName 和 '{'
                vector<StructMemTemp> members;
                while(tokens[i] != "}") {
                    string mType = tokens[i++];
                    string mName = tokens[i++];
                    vector<int> dims;
                    while(tokens[i] == "[") {
                        i++; dims.push_back(stoi(tokens[i++])); i++; 
                    }
                    members.push_back({mName, mType, dims});
                    if(tokens[i] == ";") i++;
                }
                i++; // 跳过 '}'
                if(i < tokens.size() && tokens[i] == ";") i++;
                addStructDefinition(sName, members);
            } 
            // 2. 处理变量声明或函数定义
            else if (typeMap.count(tk) || isUserType(tk)) {
                string baseType = typeMap.count(tk) ? typeMap[tk] : tk;
                string nameTk = tokens[i+1];

                // 判断是否为函数定义
                if (i + 2 < tokens.size() && tokens[i+2] == "(") {
                    i += 3; // 跳过 type, name, (
                    
                    int pfinflIdx = PFINFL.size();
                    SYNBL.push_back(Symbol(nameTk, baseType, "p", Addr("PFINFL", pfinflIdx)));
                    
                    PFINFL.push_back(ProcInfo(nameTk, currentLevel, 0, 0, PARAML.size()));
                    currentFuncIdx = pfinflIdx;

                    currentLevel++;              
                    offsetStack.push_back(0);    

                    int paramCount = 0;
                    while(tokens[i] != ")") {
                        if (tokens[i] == ",") { i++; continue; }
                        string pType = tokens[i++];
                        string pName = tokens[i++];
                        string actualPType = typeMap.count(pType) ? typeMap[pType] : pType;
                        
                        int len = getTypeLength(actualPType);
                        
                        PARAML.push_back(ParamNode(pName, actualPType, "vf", currentLevel, offsetStack.back()));
                        SYNBL.push_back(Symbol(pName, actualPType, "v", Addr("PARAML", PARAML.size()-1)));
                        VALL.push_back(ValueInfo(pName, currentLevel, offsetStack.back()));

                        offsetStack.back() += len;
                        paramCount++;
                    }
                    PFINFL[currentFuncIdx].PARAM_COUNT = paramCount;
                    i++; // 跳过 ')'
                    if (tokens[i] == "{") i++; 
                } 
                else {
                    // 普通变量声明
                    i++; // 跳过 type
                    while(i < tokens.size() && tokens[i] != ";") {
                        string vName = tokens[i++];
                        vector<int> dims;
                        while(i < tokens.size() && tokens[i] == "[") {
                            i++; dims.push_back(stoi(tokens[i++])); i++; 
                        }
                        
                        string finalType = buildArrayType(baseType, dims);
                        int len = getTypeLength(finalType);

                        SYNBL.push_back(Symbol(vName, finalType, "v", Addr("VALL", VALL.size())));
                        VALL.push_back(ValueInfo(vName, currentLevel, offsetStack.back()));
                        offsetStack.back() += len;

                        if (tokens[i] == ",") i++;
                        else break;
                    }
                    if (i < tokens.size() && tokens[i] == ";") i++;
                }
            } 
            // 3. 处理作用域块结束
            else if (tk == "}") {
                if (currentFuncIdx != -1) {
                    PFINFL[currentFuncIdx].TOTAL_LEN = offsetStack.back(); 
                    currentFuncIdx = -1;
                }
                currentLevel--;            
                offsetStack.pop_back();    
                i++;
            } 
            else {
                i++; 
            }
        }
    }

    void printAll() {
        auto printHeader = [](string title) {
            cout << "\n[ " << title << " ]\n" << string(70,'-') << endl;
        };

        printHeader("SYNBL - 主符号表");
        cout << left << setw(15) << "NAME" << setw(15) << "TYPE" << setw(8) << "CAT" << "ADDR" << endl;
        for(auto &s : SYNBL)
            cout << setw(15) << s.NAME << setw(15) << s.TYPE << setw(8) << s.CAT
                 << s.ADDR.table << "[" << s.ADDR.index << "]" << endl;

        printHeader("PFINFL - 函数/过程属性表");
        cout << left << setw(15) << "NAME" << setw(10) << "LEVEL" << setw(15) << "TOTAL_LEN"
             << setw(15) << "PARAM_CNT" << "ENTRY (指向)" << endl;
        for(auto &p : PFINFL)
            cout << setw(15) << p.NAME << setw(10) << p.LEVEL << setw(15) << p.TOTAL_LEN 
                 << setw(15) << p.PARAM_COUNT << "PARAML[" << p.ENTRY_INDEX << "]" << endl;

        printHeader("PARAML - 参数/形参表");
        cout << left << setw(8) << "IDX" << setw(15) << "NAME" << setw(15) << "TYPE" 
             << setw(10) << "KIND" << setw(10) << "LEVEL" << "OFFSET" << endl;
        for(size_t i=0; i<PARAML.size(); ++i) {
            cout << setw(8) << i << setw(15) << PARAML[i].NAME << setw(15) << PARAML[i].TYPE 
                 << setw(10) << PARAML[i].KIND << setw(10) << PARAML[i].LEVEL << PARAML[i].OFFSET << endl;
        }

        printHeader("TYPEL - 类型表");
        cout << left << setw(8) << "IDX" << setw(10) << "TVAL" << setw(12) << "TPOINT" << "LEN" << endl;
        for(size_t i=0; i<TYPEL.size(); ++i)
            cout << setw(8) << i << setw(10) << TYPEL[i].TVAL
                 << setw(12) << TYPEL[i].TPOINT << TYPEL[i].LEN << endl;

        // ====== 补充打印：AINFL 数组表 ======
        printHeader("AINFL - 数组信息表");
        cout << left << setw(8) << "IDX" << setw(8) << "SIZE" << setw(18) << "CTP" << "CLEN" << endl;
        for(size_t i=0; i<AINFL.size(); ++i)
            cout << setw(8) << i << setw(8) << AINFL[i].SIZE
                 << setw(18) << AINFL[i].CTP << AINFL[i].CLEN << endl;

        // ====== 补充打印：RINFL 结构体成员表 ======
        printHeader("RINFL - 结构体成员表");
        cout << left << setw(15) << "ID" << setw(10) << "OFF" << "TP" << endl;
        for(auto &r : RINFL)
            cout << setw(15) << r.ID << setw(10) << r.OFF << r.TP << endl;

        printHeader("VALL - 活动记录表 (各层级变量与形参偏移)");
        cout << left << setw(15) << "NAME" << setw(10) << "LEVEL" << "OFFSET" << endl;
        for(auto &v : VALL)
            cout << setw(15) << v.NAME << setw(10) << v.LEVEL << v.OFFSET << endl;
    }
};

int main() {
    SymbolTableSystem sts;
    
    // 全方位综合测试：包含全局多维数组、结构体定义（内含数组），以及带参数和局部变量的函数
    string code = R"(
        int matrix[3][4];

        struct DataBlock {
            int tags[5];
            float score;
        };

        int P1(int x, float y) {
            int m;
            DataBlock myData;
        }
    )";

    sts.analyzeCode(code);
    sts.printAll();
    return 0;
}