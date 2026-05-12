#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <cctype>

using namespace std;

//
// ======================================================
// 编译原理 —— 完整符号表系统
//
// 包含：
// 1. 关键字表
// 2. 界符表
// 3. 常数表 CONSL
// 4. 类型表 TYPEL
// 5. 数组表 AINFL
// 6. 结构表 RINFL
// 7. 函数表 PFINFL
// 8. 活动记录表 VALL
// 9. 长度表 LENL
// 10. 总符号表 SYNBL
//
// ======================================================
//

//
// ======================================================
// 基本结构定义
// ======================================================
//

// --------------------
// 类型表 TYPEL
// --------------------
struct TypeNode {
    string TVAL;     // i r c b a d
    int TPOINT;      // 指向其他表

    TypeNode(string t = "", int p = -1)
        : TVAL(t), TPOINT(p) {}
};

// --------------------
// 数组表 AINFL
// --------------------
struct ArrayInfo {
    int LOW;
    int UP;
    int CTP;      // 成分类型指针
    int CLEN;     // 成分长度

    ArrayInfo(int l=0, int u=0,
              int ctp=-1, int clen=0)
        : LOW(l), UP(u),
          CTP(ctp), CLEN(clen) {}
};

// --------------------
// 结构表 RINFL
// --------------------
struct RecordInfo {
    string ID;
    int OFF;
    int TP;

    RecordInfo(string id="",
               int off=0,
               int tp=-1)
        : ID(id), OFF(off), TP(tp) {}
};

// --------------------
// 函数表 PFINFL
// --------------------
struct FunctionInfo {

    int LEVEL;
    int OFF;
    int FN;
    string ENTRY;

    vector<string> PARAMS;

    FunctionInfo(
        int level=0,
        int off=0,
        int fn=0,
        string entry=""
    )
        : LEVEL(level),
          OFF(off),
          FN(fn),
          ENTRY(entry) {}
};

// --------------------
// 活动记录表 VALL
// --------------------
struct ValueInfo {

    string NAME;
    int LEVEL;
    int OFFSET;

    ValueInfo(string n="",
              int l=0,
              int off=0)
        : NAME(n),
          LEVEL(l),
          OFFSET(off) {}
};

// --------------------
// 总符号表 SYNBL
// --------------------
struct Symbol {

    string NAME;
    int TYPE;      // 指向TYPEL
    string CAT;    // v c f t vn vf
    int ADDR;      // 指向其他表

    Symbol(string n="",
           int t=-1,
           string c="",
           int a=-1)
        : NAME(n),
          TYPE(t),
          CAT(c),
          ADDR(a) {}
};

//
// ======================================================
// 符号表系统类
// ======================================================
//
class SymbolTableSystem {

private:

    //
    // 关键字表
    //
    map<string, int> keywordTable;

    //
    // 界符表
    //
    map<string, int> delimiterTable;

    //
    // 常数表 CONSL
    //
    vector<string> CONSL;

    //
    // 长度表 LENL
    //
    vector<int> LENL;

    //
    // 类型表 TYPEL
    //
    vector<TypeNode> TYPEL;

    //
    // 数组表 AINFL
    //
    vector<ArrayInfo> AINFL;

    //
    // 结构表 RINFL
    //
    vector<RecordInfo> RINFL;

    //
    // 函数表 PFINFL
    //
    vector<FunctionInfo> PFINFL;

    //
    // 活动记录表 VALL
    //
    vector<ValueInfo> VALL;

    //
    // 总符号表 SYNBL
    //
    vector<Symbol> SYNBL;

public:

    //
    // ==================================================
    // 构造函数
    // ==================================================
    //
    SymbolTableSystem() {

        initKeywordTable();
        initDelimiterTable();
        initBasicTypes();
    }

    //
    // ==================================================
    // 初始化关键字表
    // ==================================================
    //
    void initKeywordTable() {

        keywordTable = {

            {"int",1},
            {"float",2},
            {"double",3},
            {"char",4},
            {"bool",5},
            {"if",6},
            {"else",7},
            {"while",8},
            {"for",9},
            {"return",10},
            {"void",11},
            {"struct",12}
        };
    }

    //
    // ==================================================
    // 初始化界符表
    // ==================================================
    //
    void initDelimiterTable() {

        delimiterTable = {

            {"+",1},
            {"-",2},
            {"*",3},
            {"/",4},
            {"=",5},
            {"==",6},
            {"<",7},
            {">",8},
            {"(",9},
            {")",10},
            {"{",11},
            {"}",12},
            {";",13},
            {",",14}
        };
    }

    //
    // ==================================================
    // 初始化基本类型
    // ==================================================
    //
    void initBasicTypes() {

        //
        // TYPEL
        //

        TYPEL.push_back(TypeNode("i",-1)); // 0
        TYPEL.push_back(TypeNode("r",-1)); // 1
        TYPEL.push_back(TypeNode("c",-1)); // 2
        TYPEL.push_back(TypeNode("b",-1)); // 3

        //
        // LENL
        //

        LENL.push_back(4); // int
        LENL.push_back(8); // real
        LENL.push_back(1); // char
        LENL.push_back(1); // bool
    }

    //
    // ==================================================
    // 插入常数
    // ==================================================
    //
    int addConstant(string value) {

        CONSL.push_back(value);

        return CONSL.size() - 1;
    }

    //
    // ==================================================
    // 插入类型
    // ==================================================
    //
    int addType(string tval, int tpoint) {

        TYPEL.push_back(TypeNode(tval, tpoint));

        return TYPEL.size() - 1;
    }

    //
    // ==================================================
    // 插入数组类型
    // ==================================================
    //
    int addArrayInfo(
        int low,
        int up,
        int ctp,
        int clen
    ) {

        AINFL.push_back(
            ArrayInfo(low, up, ctp, clen)
        );

        return AINFL.size() - 1;
    }

    //
    // ==================================================
    // 插入结构体信息
    // ==================================================
    //
    int addRecordInfo(
        string id,
        int off,
        int tp
    ) {

        RINFL.push_back(
            RecordInfo(id, off, tp)
        );

        return RINFL.size() - 1;
    }

    //
    // ==================================================
    // 插入函数信息
    // ==================================================
    //
    int addFunctionInfo(
        int level,
        int off,
        int fn,
        string entry
    ) {

        PFINFL.push_back(
            FunctionInfo(level, off, fn, entry)
        );

        return PFINFL.size() - 1;
    }

    //
    // ==================================================
    // 插入活动记录
    // ==================================================
    //
    int addValueInfo(
        string name,
        int level,
        int offset
    ) {

        VALL.push_back(
            ValueInfo(name, level, offset)
        );

        return VALL.size() - 1;
    }

    //
    // ==================================================
    // 插入总符号表
    // ==================================================
    //
    int addSymbol(
        string name,
        int type,
        string cat,
        int addr
    ) {

        SYNBL.push_back(
            Symbol(name, type, cat, addr)
        );

        return SYNBL.size() - 1;
    }

    //
    // ==================================================
    // 创建数组类型
    // ==================================================
    //
    int createArrayType(
        int low,
        int up,
        int baseType
    ) {

        int len = LENL[baseType];

        int arrayInfoIndex =
            addArrayInfo(
                low,
                up,
                baseType,
                len
            );

        int typeIndex =
            addType("a", arrayInfoIndex);

        int totalLength =
            (up - low + 1) * len;

        LENL.push_back(totalLength);

        return typeIndex;
    }

    //
    // ==================================================
    // 输出关键字表
    // ==================================================
    //
    void printKeywordTable() {

        cout << "\n======== KEYWORD TABLE ========\n";

        for (auto& kv : keywordTable) {

            cout << "<"
                 << kv.first
                 << ", "
                 << kv.second
                 << ">\n";
        }
    }

    //
    // ==================================================
    // 输出界符表
    // ==================================================
    //
    void printDelimiterTable() {

        cout << "\n======== DELIMITER TABLE ========\n";

        for (auto& kv : delimiterTable) {

            cout << "<"
                 << kv.first
                 << ", "
                 << kv.second
                 << ">\n";
        }
    }

    //
    // ==================================================
    // 输出常数表
    // ==================================================
    //
    void printCONSL() {

        cout << "\n======== CONSL ========\n";

        for (int i=0;i<CONSL.size();i++) {

            cout << i
                 << " : "
                 << CONSL[i]
                 << endl;
        }
    }

    //
    // ==================================================
    // 输出长度表
    // ==================================================
    //
    void printLENL() {

        cout << "\n======== LENL ========\n";

        for (int i=0;i<LENL.size();i++) {

            cout << i
                 << " : "
                 << LENL[i]
                 << endl;
        }
    }

    //
    // ==================================================
    // 输出类型表
    // ==================================================
    //
    void printTYPEL() {

        cout << "\n======== TYPEL ========\n";

        cout << left
             << setw(10) << "INDEX"
             << setw(10) << "TVAL"
             << setw(10) << "TPOINT"
             << endl;

        for (int i=0;i<TYPEL.size();i++) {

            cout << left
                 << setw(10) << i
                 << setw(10) << TYPEL[i].TVAL
                 << setw(10) << TYPEL[i].TPOINT
                 << endl;
        }
    }

    //
    // ==================================================
    // 输出数组表
    // ==================================================
    //
    void printAINFL() {

        cout << "\n======== AINFL ========\n";

        cout << left
             << setw(10) << "INDEX"
             << setw(10) << "LOW"
             << setw(10) << "UP"
             << setw(10) << "CTP"
             << setw(10) << "CLEN"
             << endl;

        for (int i=0;i<AINFL.size();i++) {

            cout << left
                 << setw(10) << i
                 << setw(10) << AINFL[i].LOW
                 << setw(10) << AINFL[i].UP
                 << setw(10) << AINFL[i].CTP
                 << setw(10) << AINFL[i].CLEN
                 << endl;
        }
    }

    //
    // ==================================================
    // 输出函数表
    // ==================================================
    //
    void printPFINFL() {

        cout << "\n======== PFINFL ========\n";

        cout << left
             << setw(10) << "INDEX"
             << setw(10) << "LEVEL"
             << setw(10) << "OFF"
             << setw(10) << "FN"
             << setw(15) << "ENTRY"
             << endl;

        for (int i=0;i<PFINFL.size();i++) {

            cout << left
                 << setw(10) << i
                 << setw(10) << PFINFL[i].LEVEL
                 << setw(10) << PFINFL[i].OFF
                 << setw(10) << PFINFL[i].FN
                 << setw(15) << PFINFL[i].ENTRY
                 << endl;
        }
    }

    //
    // ==================================================
    // 输出活动记录表
    // ==================================================
    //
    void printVALL() {

        cout << "\n======== VALL ========\n";

        cout << left
             << setw(15) << "NAME"
             << setw(10) << "LEVEL"
             << setw(10) << "OFFSET"
             << endl;

        for (auto& v : VALL) {

            cout << left
                 << setw(15) << v.NAME
                 << setw(10) << v.LEVEL
                 << setw(10) << v.OFFSET
                 << endl;
        }
    }

    //
    // ==================================================
    // 输出总符号表
    // ==================================================
    //
    void printSYNBL() {

        cout << "\n======== SYNBL ========\n";

        cout << left
             << setw(15) << "NAME"
             << setw(10) << "TYPE"
             << setw(10) << "CAT"
             << setw(10) << "ADDR"
             << endl;

        for (auto& s : SYNBL) {

            cout << left
                 << setw(15) << s.NAME
                 << setw(10) << s.TYPE
                 << setw(10) << s.CAT
                 << setw(10) << s.ADDR
                 << endl;
        }
    }

    //
    // ==================================================
    // 输出所有表
    // ==================================================
    //
    void printAll() {

        printKeywordTable();

        printDelimiterTable();

        printCONSL();

        printLENL();

        printTYPEL();

        printAINFL();

        printPFINFL();

        printVALL();

        printSYNBL();
    }
};

//
// ======================================================
// 测试
// ======================================================
//
int main() {

    SymbolTableSystem sts;

    //
    // 常量
    //
    int c1 = sts.addConstant("3.14");

    //
    // 创建数组类型
    // arr[1..10] of int
    //
    int arrType =
        sts.createArrayType(
            1,
            10,
            0
        );

    //
    // 函数信息
    //
    int funcAddr =
        sts.addFunctionInfo(
            1,
            0,
            2,
            "main_entry"
        );

    //
    // 活动记录
    //
    int aAddr =
        sts.addValueInfo(
            "a",
            1,
            0
        );

    //
    // 总符号表
    //

    // 常量 pai
    sts.addSymbol(
        "pai",
        1,
        "c",
        c1
    );

    // 类型 arr
    sts.addSymbol(
        "arr",
        arrType,
        "t",
        arrType
    );

    // 变量 a
    sts.addSymbol(
        "a",
        arrType,
        "v",
        aAddr
    );

    // 函数 main
    sts.addSymbol(
        "main",
        1,
        "f",
        funcAddr
    );

    //
    // 输出所有表
    //
    sts.printAll();

    return 0;
}