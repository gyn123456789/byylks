#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <cctype>

using namespace std;

// ========================
// ADDR
// ========================
struct Addr {
    string table;
    int index;
    Addr(string t="", int i=-1) : table(t), index(i) {}
};

// ========================
// TYPEL
// ========================
struct TypeNode {
    string TVAL; // i/r/c/b/a/d
    int TPOINT;  // 指向数组/结构体表索引
    int LEN;     // 类型长度
    TypeNode(string tv="", int tp=-1, int len=0)
        : TVAL(tv), TPOINT(tp), LEN(len) {}
};

// ========================
// 数组表 AINFL
// ========================
struct ArrayInfo {
    int LOW;
    int UP;
    string CTP; // 基类型 itp/rtp/ctp/btp 或 TYPEL[index]
    int CLEN;   // 元素长度
    ArrayInfo(int l=0,int u=0,string bt="",int clen=0)
        : LOW(l), UP(u), CTP(bt), CLEN(clen) {}
};

// ========================
// 结构体表 RINFL
// ========================
struct RecordInfo {
    string ID;
    int OFF;
    string TP; // 基类型 itp/rtp/ctp/btp 或 TYPEL[index]
    RecordInfo(string id="", int off=0, string tp="")
        : ID(id), OFF(off), TP(tp) {}
};

// ========================
// 活动记录表 VALL
// ========================
struct ValueInfo {
    string NAME;
    int LEVEL;
    int OFFSET;
    ValueInfo(string n="",int l=0,int off=0)
        : NAME(n), LEVEL(l), OFFSET(off) {}
};

// ========================
// 总符号表 SYNBL
// ========================
struct Symbol {
    string NAME;
    string TYPE; // 基本类型或 TYPEL[index]
    string CAT;  // v/c/f/t
    Addr ADDR;
    Symbol(string n="", string t="", string c="", Addr a=Addr())
        : NAME(n), TYPE(t), CAT(c), ADDR(a) {}
};

// ========================
// 符号表系统
// ========================
class SymbolTableSystem {
private:
    vector<string> CONSL;
    vector<TypeNode> TYPEL;
    vector<ArrayInfo> AINFL;
    vector<RecordInfo> RINFL;
    vector<ValueInfo> VALL;
    vector<Symbol> SYNBL;
    int currentOffset = 0;

    // 基本类型长度
    int getTypeLength(const string &type) {
        if(type=="itp") return 4;
        if(type=="rtp") return 8;
        if(type=="ctp") return 1;
        if(type=="btp") return 1;
        if(type.substr(0,5)=="TYPEL") {
            int idx = stoi(type.substr(6));
            return TYPEL[idx].LEN;
        }
        return 0;
    }

    bool isNumber(const string &s) {
        if(s.empty()) return false;
        bool dot=false;
        for(char c:s){
            if(c=='.'){ if(dot) return false; dot=true; }
            else if(!isdigit(c)) return false;
        }
        return true;
    }

    int lastStructTypeIndex = -1; // 保存上一次定义的结构体类型索引

public:
    void analyzeCode(const string &code) {
        string codeCopy = code;
        for(auto &c: codeCopy) if(c=='\n') c=' ';
        stringstream ss(codeCopy);
        string token;
        string prevType="";
        bool inStruct=false;
        string structName;
        vector<pair<string,string>> structMembers;

        while(ss>>token){
            if(token=="int") prevType="itp";
            else if(token=="float") prevType="rtp";
            else if(token=="char") prevType="ctp";
            else if(token=="bool") prevType="btp";
            else if(token=="struct"){
                ss>>structName;
                if(!structName.empty() && (structName.back()=='{' || structName.back()==';')) 
                    structName.pop_back();
                inStruct=true;
                structMembers.clear();
            }
            else if(inStruct){
                if(token=="{") continue;
                if(token.find('}') != string::npos){
                    lastStructTypeIndex = addStruct(structName, structMembers);
                    inStruct=false;
                    continue;
                }
                // 成员类型
                string memberType = token;
                string memberName;
                if(ss >> memberName){
                    if(!memberName.empty() && memberName.back()==';') memberName.pop_back();
                    structMembers.push_back({memberName, memberType});
                }
            }
            else if(prevType!=""){
                size_t pos = token.find('[');
                string varName = (pos!=string::npos) ? token.substr(0,pos) : token;
                if(!varName.empty() && varName.back()==';') varName.pop_back();

                if(pos!=string::npos){
                    string bounds = token.substr(pos+1);
                    size_t closePos = bounds.find(']');
                    if(closePos != string::npos) bounds = bounds.substr(0,closePos);
                    int low=1,up=10;
                    size_t dotpos=bounds.find("..");
                    if(dotpos!=string::npos){
                        low=stoi(bounds.substr(0,dotpos));
                        up=stoi(bounds.substr(dotpos+2));
                    }
                    addArray(varName,low,up,prevType);
                } else {
                    addVariable(varName,prevType);
                }
                prevType="";
            }
            else if(lastStructTypeIndex!=-1 && token == structName){
                // 结构体变量声明
                string varName;
                if(ss >> varName){
                    if(!varName.empty() && varName.back()==';') varName.pop_back();
                    addVariable(varName, "TYPEL:"+to_string(lastStructTypeIndex));
                }
            }
            else if(isNumber(token)){
                addConstant(token);
            }
        }
    }

    int addConstant(const string &value){
        for(int i=0;i<CONSL.size();i++) if(CONSL[i]==value) return i;
        CONSL.push_back(value);
        return CONSL.size()-1;
    }

    void addVariable(const string &name,const string &type){
        VALL.push_back(ValueInfo(name,1,currentOffset));
        int vallIndex = VALL.size()-1;
        SYNBL.push_back(Symbol(name,type,"v",Addr("VALL",vallIndex)));
        currentOffset += getTypeLength(type);
    }

    int addArray(const string &name,int low,int up,const string &baseType){
        int clen = getTypeLength(baseType);
        AINFL.push_back(ArrayInfo(low,up,baseType,clen));
        int arrIndex = AINFL.size()-1;
        int totalLen = (up-low+1)*clen;
        TYPEL.push_back(TypeNode("a",arrIndex,totalLen));
        int typeIndex = TYPEL.size()-1;
        SYNBL.push_back(Symbol(name,"TYPEL:"+to_string(typeIndex),"v",Addr("TYPEL",typeIndex)));
        currentOffset += totalLen;
        return typeIndex;
    }

    int addStruct(const string &name,const vector<pair<string,string>> &members){
        int off=0;
        int startIndex = RINFL.size();
        for(auto &m:members){
            string memberType;
            // 如果成员是基本类型，直接用 itp/rtp/ctp/btp
            if(m.second=="int"||m.second=="float"||m.second=="char"||m.second=="bool")
                memberType = m.second;
            else if(m.second.substr(0,6)=="TYPEL:") // 数组或结构体类型
                memberType = m.second;
            else
                memberType = m.second; // 默认保留原样

            RINFL.push_back(RecordInfo(m.first, off, memberType));
            off += getTypeLength(memberType);
        }
        TYPEL.push_back(TypeNode("d",startIndex,off));
        int typeIndex = TYPEL.size()-1;
        SYNBL.push_back(Symbol(name,"TYPEL:"+to_string(typeIndex),"v",Addr("TYPEL",typeIndex)));
        currentOffset += off;
        return typeIndex;
    }

    void printCONSL(){
        cout<<"\n======== CONSL ========\n";
        for(int i=0;i<CONSL.size();i++)
            cout<<i<<" : "<<CONSL[i]<<endl;
    }
    void printTYPEL(){
        cout<<"\n======== TYPEL ========\n";
        cout<<left<<setw(10)<<"INDEX"<<setw(10)<<"TVAL"<<setw(10)<<"TPOINT"<<setw(10)<<"LEN"<<endl;
        for(int i=0;i<TYPEL.size();i++)
            cout<<left<<setw(10)<<i<<setw(10)<<TYPEL[i].TVAL<<setw(10)<<TYPEL[i].TPOINT<<setw(10)<<TYPEL[i].LEN<<endl;
    }
    void printAINFL(){
        cout<<"\n======== AINFL ========\n";
        cout<<left<<setw(10)<<"INDEX"<<setw(10)<<"LOW"<<setw(10)<<"UP"<<setw(15)<<"BASETYPE"<<setw(10)<<"CLEN"<<endl;
        for(int i=0;i<AINFL.size();i++)
            cout<<left<<setw(10)<<i<<setw(10)<<AINFL[i].LOW<<setw(10)<<AINFL[i].UP<<setw(15)<<AINFL[i].CTP<<setw(10)<<AINFL[i].CLEN<<endl;
    }
    void printRINFL(){
        cout<<"\n======== RINFL ========\n";
        cout<<left<<setw(15)<<"ID"<<setw(10)<<"OFF"<<setw(15)<<"TP"<<endl;
        for(auto &r:RINFL)
            cout<<left<<setw(15)<<r.ID<<setw(10)<<r.OFF<<setw(15)<<r.TP<<endl;
    }
    void printVALL(){
        cout<<"\n======== VALL ========\n";
        cout<<left<<setw(15)<<"NAME"<<setw(10)<<"LEVEL"<<setw(10)<<"OFFSET"<<endl;
        for(auto &v:VALL)
            cout<<left<<setw(15)<<v.NAME<<setw(10)<<v.LEVEL<<setw(10)<<v.OFFSET<<endl;
    }
    void printSYNBL(){
        cout<<"\n======== SYNBL ========\n";
        cout<<left<<setw(15)<<"NAME"<<setw(10)<<"TYPE"<<setw(10)<<"CAT"<<setw(20)<<"ADDR"<<endl;
        for(auto &s:SYNBL){
            string addrStr = s.ADDR.table+"["+to_string(s.ADDR.index)+"]";
            cout<<left<<setw(15)<<s.NAME<<setw(10)<<s.TYPE<<setw(10)<<s.CAT<<setw(20)<<addrStr<<endl;
        }
    }
    void printAll(){
        printCONSL();
        printTYPEL();
        printAINFL();
        printRINFL();
        printVALL();
        printSYNBL();
    }
};

// ========================
// 测试
// ========================
int main(){
    SymbolTableSystem sts;

    string code = R"(
        int a = 10;
        float b = 3.14;
        char c;
        int arr[1..10];
        struct Student {
            int id;
            float score;
        };
        Student s1;
    )";

    sts.analyzeCode(code);
    sts.printAll();
    return 0;
}