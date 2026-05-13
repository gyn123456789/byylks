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
    string TVAL; // a: array, d: struct, itp/rtp/ctp: basic
    int TPOINT;  // 指向 AINFL 或 RINFL
    int LEN;     // 总长度
    TypeNode(string tv="", int tp=-1, int len=0) : TVAL(tv), TPOINT(tp), LEN(len) {}
};

struct ArrayInfo {
    int SIZE;
    string CTP; // 元素基类型
    int CLEN;   // 元素长度
    ArrayInfo(int s=0, string c="", int cl=0) : SIZE(s), CTP(c), CLEN(cl) {}
};

struct RecordInfo {
    string ID; int OFF; string TP;
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

// ======================== 符号表系统 ========================
class SymbolTableSystem {
private:
    vector<TypeNode> TYPEL;
    vector<ArrayInfo> AINFL;
    vector<RecordInfo> RINFL;
    vector<ValueInfo> VALL;
    vector<Symbol> SYNBL;
    map<string, string> typeMap = {{"int","itp"}, {"float","rtp"}, {"char","ctp"}};
    int currentOffset = 0;

    int getTypeLength(const string &t) {
        if(t == "itp") return 4;
        if(t == "rtp") return 8;
        if(t == "ctp") return 1;
        if(t.find("TYPEL:") == 0) return TYPEL[stoi(t.substr(6))].LEN;
        return 0;
    }

public:
    void addVariable(const string &name, const string &type) {
        int len = getTypeLength(type);
        SYNBL.push_back(Symbol(name, type, "v", Addr("VALL", (int)VALL.size())));
        VALL.push_back(ValueInfo(name, 1, currentOffset));
        currentOffset += len;
    }

    void addArray(const string &name, int size, const string &baseType) {
        int clen = getTypeLength(baseType);
        int totalLen = size * clen;

        AINFL.push_back(ArrayInfo(size, baseType, clen));
        TYPEL.push_back(TypeNode("a", (int)AINFL.size() - 1, totalLen));
        string typeStr = "TYPEL:" + to_string(TYPEL.size() - 1);

        SYNBL.push_back(Symbol(name, typeStr, "v", Addr("VALL", (int)VALL.size())));
        VALL.push_back(ValueInfo(name, 1, currentOffset));
        currentOffset += totalLen;
    }

    void addStruct(const string &name, vector<pair<string,string>> members) {
        int startR = RINFL.size();
        int innerOff = 0;
        for(auto &m : members) {
            string mType = typeMap.count(m.second) ? typeMap[m.second] : m.second;
            RINFL.push_back(RecordInfo(m.first, innerOff, mType));
            innerOff += getTypeLength(mType);
        }
        TYPEL.push_back(TypeNode("d", startR, innerOff));
        string tStr = "TYPEL:" + to_string(TYPEL.size() - 1);
        SYNBL.push_back(Symbol(name, tStr, "t", Addr("TYPEL", (int)TYPEL.size()-1)));
        typeMap[name] = tStr; 
    }

    void analyzeCode(const string &code) {
        stringstream ss(code);
        string token;
        while(ss >> token) {
            if(typeMap.count(token)) {
                string type = typeMap[token];
                string var; ss >> var;
                // 数组
                size_t pos = var.find('[');
                if(pos != string::npos) {
                    string name = var.substr(0, pos);
                    size_t pos2 = var.find(']');
                    int size = stoi(var.substr(pos+1, pos2-pos-1));
                    addArray(name, size, type);
                } else {
                    if(var.back() == ';') var.pop_back();
                    addVariable(var, type);
                }
            } else if(token == "struct") {
                string sName; ss >> sName;
                if(sName.back() == '{') sName.pop_back(); else { string b; ss >> b; }
                vector<pair<string,string>> members;
                string mt, mn;
                while(ss >> mt && mt != "};") {
                    ss >> mn; if(mn.back() == ';') mn.pop_back();
                    members.push_back({mn, mt});
                }
                addStruct(sName, members);
            } else {
                string var = token;
                if(var.back() == ';') var.pop_back();
                if(typeMap.count(var)) continue;
                addVariable(var, var);
            }
        }
    }

    void printAll() {
        cout << "\n[ SYNBL - 主符号表 ]\n" << string(60,'-') << endl;
        cout << left << setw(12) << "NAME" << setw(12) << "TYPE" << setw(6) << "CAT" << "ADDR" << endl;
        for(auto &s : SYNBL)
            cout << setw(12) << s.NAME << setw(12) << s.TYPE << setw(6) << s.CAT
                 << s.ADDR.table << "[" << s.ADDR.index << "]" << endl;

        cout << "\n[ TYPEL - 类型表 ]\n" << string(50,'-') << endl;
        cout << left << setw(6) << "IDX" << setw(8) << "TVAL" << setw(10) << "TPOINT" << "LEN" << endl;
        for(int i=0;i<TYPEL.size();++i)
            cout << setw(6) << i << setw(8) << TYPEL[i].TVAL
                 << setw(10) << TYPEL[i].TPOINT << TYPEL[i].LEN << endl;

        cout << "\n[ AINFL - 数组信息表 ]\n" << string(50,'-') << endl;
        cout << left << setw(6) << "IDX" << setw(6) << "SIZE" << setw(12) << "CTP" << "CLEN" << endl;
        for(int i=0;i<AINFL.size();++i)
            cout << setw(6) << i << setw(6) << AINFL[i].SIZE
                 << setw(12) << AINFL[i].CTP << AINFL[i].CLEN << endl;

        cout << "\n[ RINFL - 结构体成员表 ]\n" << string(50,'-') << endl;
        cout << left << setw(12) << "ID" << setw(8) << "OFF" << "TP" << endl;
        for(auto &r : RINFL)
            cout << setw(12) << r.ID << setw(8) << r.OFF << r.TP << endl;

        cout << "\n[ VALL - 活动记录表 ]\n" << string(50,'-') << endl;
        cout << left << setw(12) << "NAME" << setw(8) << "LEVEL" << "OFFSET" << endl;
        for(auto &v : VALL)
            cout << setw(12) << v.NAME << setw(8) << v.LEVEL << v.OFFSET << endl;
    }
};

int main() {
    SymbolTableSystem sts;
    string code = R"(
        int a;
        float b;
        int arr[5];
        struct Point {
            int x;
            int y;
        };
        struct Rect {
            Point topLeft;
            Point bottomRight;
        };
        Rect r1;
        Point points[3];
    )";

    sts.analyzeCode(code);
    sts.printAll();
    return 0;
}