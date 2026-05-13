#ifndef MENU_H
#define MENU_H

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

const map<string, int> KT = {
    {"int", 1}, {"float", 2}, {"double", 3}, {"char", 4}, {"bool", 5},
    {"if", 6}, {"else", 7}, {"while", 8}, {"for", 9}, {"return", 10},
    {"void", 11}, {"struct", 12}
};

const map<string, int> BT = {
    {"+", 1}, {"-", 2}, {"*", 3}, {"/", 4}, {"=", 5}, {"==", 6},
    {"<", 7}, {">", 8}, {"(", 9}, {")", 10}, {"{", 11}, {"}", 12},
    {";", 13}, {",", 14}, {"[", 15}, {"]", 16}, {"!=", 17}, {"<=", 18},
    {">=", 19}, {"&&", 20}, {"||", 21}
};

// 全局标识符表
extern vector<string> IT;

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
    string TVAL;
    int    TPOINT;
    int    LEN;
    TypeNode(string tv = "", int tp = -1, int len = 0)
        : TVAL(move(tv)), TPOINT(tp), LEN(len) {}
};

struct ArrayInfo {
    int    SIZE;
    string CTP;
    int    CLEN;
    ArrayInfo(int s = 0, string c = "", int cl = 0)
        : SIZE(s), CTP(move(c)), CLEN(cl) {}
};

struct RecordInfo {
    string ID;
    int    OFF;
    string TP;
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
    string CAT;
    Addr   ADDR;
    Symbol(string n = "", string t = "", string c = "", Addr a = Addr())
        : NAME(move(n)), TYPE(move(t)), CAT(move(c)), ADDR(move(a)) {}
};

struct ParamNode {
    string NAME;
    string TYPE;
    string KIND;
    int    LEVEL;
    int    OFFSET;
    ParamNode(string n = "", string t = "", string k = "vf", int l = 0, int off = 0)
        : NAME(move(n)), TYPE(move(t)), KIND(move(k)), LEVEL(l), OFFSET(off) {}
};

struct ProcInfo {
    string NAME;
    int    LEVEL;
    int    TOTAL_LEN;
    int    PARAM_COUNT;
    int    ENTRY_INDEX;
    ProcInfo(string n = "", int l = 0, int tl = 0, int pc = 0, int entry = -1)
        : NAME(move(n)), LEVEL(l), TOTAL_LEN(tl), PARAM_COUNT(pc), ENTRY_INDEX(entry) {}
};

// ================================================================
// 词法分析器函数
// ================================================================

int getOrInsertIdent(const string &s);
vector<Token> scanner(const string &code);

// ================================================================
// 符号表系统
// ================================================================

class SymbolTableSystem {
public:
    vector<TypeNode>   TYPEL;
    vector<ArrayInfo>  AINFL;
    vector<RecordInfo> RINFL;
    vector<ValueInfo>  VALL;
    vector<Symbol>     SYNBL;
    vector<ProcInfo>   PFINFL;
    vector<ParamNode>  PARAML;

    void analyze(const string &code);
    void printAll() const;
};

#endif // MENU_H