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

    bool contains(uint64_t addr) { return (addr >= vma) && (addr - vma < size); }

    Binary *binary; // このセクションオブジェクトが所属するバイナリ
    string name;
    SectionType type;
    uint64_t vma; // 開始アドレス
    uint64_t size;
    uint8_t *bytes; // セクションデータのバイト列
};

class Binary
{
public:
    enum BinaryType
    {
        BIN_TYPE_AUTO = 0,
        BIN_TYPE_ELF = 1,
        BIN_TYPE_PE = 2,
    };
    enum BinaryArch
    {
        ARCH_NONE = 0,
        ARCH_X86 = 1,
    };

    Binary() : type(BIN_TYPE_AUTO), arch(ARCH_NONE), bits(0), entry(0) {}

    Section *get_text_section()
    {
        for (int i = 0; i < sections.size(); i++)
        {
            Section &sec = sections[i];
            {
                if (sec.name == ".text")
                    return &sec;
            }
            return NULL;
        };
    }

    string filename;
    BinaryType type;
    string type_str;
    BinaryArch arch;
    string arch_str;
    unsigned bits;  // 32 or 64
    uint64_t entry; // エントリポイントアドレス
    vector<Section> sections;
    vector<Symbol> symbols;
};

int load_binary(string &filename, Binary *bin, Binary::BinaryType type);
void unload_binary(Binary *bin);

#endif // LOADER_H