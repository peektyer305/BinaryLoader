#ifndef LOADER_H
#define LOADER_H

#include <string>
#include <stdint.h>
#include <vector>

using namespace std;
// 循環参照回避
class Binary;
class Section;
class Symbol;

class Symbol
{
public:
    enum SymbolType
    {
        SYM_TYPE_UKN = 0,
        SYM_TYPE_FUNC = 1,
    };
    Symbol() : type(SYM_TYPE_UKN), name(), addr(0) {}

    SymbolType type;
    string name;
    uint64_t addr; // 開始アドレス
};

class Section
{
public:
    enum SectionType
    {
        SEC_TYPE_NONE = 0,
        SEC_TYPE_CODE = 1,
        SEC_TYPE_DATA = 2,
    };

    Section() : binary(NULL), type(SEC_TYPE_NONE), vma(0), size(0), bytes(NULL) {}

    Binary *binary; // このセクションオブジェクトが所属するバイナリ
    string name;
    SectionType type;
    uint64_t vma; // 開始アドレス
    uint64_t size;
    uint8_t *bytes; // セクションデータのバイト列
};

#endif // LOADER_H