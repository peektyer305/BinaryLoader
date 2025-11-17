#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <string>
#include <vector>

#include <bfd.h>

#include "loader.h"

using namespace std;

// libbfdを初期化して、指定されたファイル名のBFDハンドルを開く
static bfd *open_bfd(string &fname)
{
    static int bfd_inited = 0;

    bfd *bfd_handler;

    if (!bfd_inited)
    {
        bfd_init();
        bfd_inited = 1;
    }

    bfd_handler = bfd_openr(fname.c_str(), NULL); // fnameをC++からC文字列に変換して渡す
    if (!bfd_handler)
    {
        fprintf(stderr, "Error: failed to open binary '%s': %s\n", fname.c_str(), bfd_errmsg(bfd_get_error()));
        return NULL;
    }

    if (!bfd_check_format(bfd_handler, bfd_object))
    {
        fprintf(stderr, "file '%s' does not look like an executable (%s)\n", fname.c_str(), bfd_errmsg(bfd_get_error()));
        return NULL;
    }

    bfd_set_error(bfd_error_no_error); // エラー状態をクリア

    if (bfd_get_flavour(bfd_handler) == bfd_target_unknown_flavour)
    {
        fprintf(stderr, "unrecognized binary format '%s' (%s)\n", fname.c_str(), bfd_errmsg(bfd_get_error()));
        return NULL;
    }
    return bfd_handler;
}

static int load_symbols_bfd(bfd *bfd_handler, Binary *bin)
{
    int ret;
    long n, nsyms, i;
    asymbol **bfd_symtab;
    Symbol *sym;

    bfd_symtab = NULL;

    n = bfd_get_symtab_upper_bound(bfd_handler);
    if (n < 0)
    {
        fprintf(stderr, "failed to read symbol table: %s\n", bfd_errmsg(bfd_get_error()));
        goto fail;
    }
    else if (n)
    {
        bfd_symtab = (asymbol **)malloc(n);
        if (!bfd_symtab)
        {
            fprintf(stderr, "out of memory\n");
            goto fail;
        }
        nsyms = bfd_canonicalize_symtab(bfd_handler, bfd_symtab);

        if (nsyms < 0)
        {
            fprintf(stderr, "failed to read symbol table: %s\n", bfd_errmsg(bfd_get_error()));
            goto fail;
        }

        for (i = 0; i < nsyms; i++)
        {
            if (bfd_symtab[i]->flags & BSF_FUNCTION)
            {
                bin->symbols.push_back(Symbol()); // 一時オブジェクトを追加
                sym = &bin->symbols.back();       // 追加したオブジェクトへのポインタを取得
                sym->type = Symbol::SYM_TYPE_FUNC;
                sym->name = string(bfd_symtab[i]->name);
                sym->addr = bfd_asymbol_value(bfd_symtab[i]);
            }
        }
    }

    ret = 0;
    goto cleanup;

fail:
    ret = -1;
cleanup:
    if (bfd_symtab)
        free(bfd_symtab);

    return ret;
}

static int load_dynsym_bfd(bfd *bfd_handler, Binary *bin)
{
    int ret;
    long n, nsyms, i;
    asymbol **bfd_dynsym;
    Symbol *sym;
    bfd_dynsym = NULL;

    n = bfd_get_dynamic_symtab_upper_bound(bfd_handler);
    if (n < 0)
    {
        fprintf(stderr, "failed to read dynamic symbol table: %s\n", bfd_errmsg(bfd_get_error()));
        goto fail;
    }
    else if (n)
    {
        bfd_dynsym = (asymbol **)malloc(n);
        if (!bfd_dynsym)
        {
            fprintf(stderr, "out of memory\n");
            goto fail;
        }
        nsyms = bfd_canonicalize_dynamic_symtab(bfd_handler, bfd_dynsym);
        if (nsyms < 0)
        {
            fprintf(stderr, "failed to read dynamic symbol table: %s\n", bfd_errmsg(bfd_get_error()));
            goto fail;
        }
        for (i = 0; i < nsyms; i++)
        {
            if (bfd_dynsym[i]->flags & BSF_FUNCTION)
            {
                bin->symbols.push_back(Symbol()); // 一時オブジェクトを追加
                sym = &bin->symbols.back();       // 追加したオブジェクトへのポインタを取得
                sym->type = Symbol::SYM_TYPE_FUNC;
                sym->name = string(bfd_dynsym[i]->name);
                sym->addr = bfd_asymbol_value(bfd_dynsym[i]);
            }
        }
    }
    ret = 0;
    goto cleanup;
fail:
    ret = -1;
cleanup:
    if (bfd_dynsym)
        free(bfd_dynsym);

    return ret;
}

static int load_sections_bfd(bfd *bfd_handler, Binary *bin)
{
    int bfd_flags;
    uint64_t vma, size;
    const char *secname;
    asection *bfd_sec;
    Section *sec;
    Section::SectionType sectype;

    for (bfd_sec = bfd_handler->sections; bfd_sec != NULL; bfd_sec = bfd_sec->next)
    {
        bfd_flags = bfd_get_section_flags(bfd_handler, bfd_sec);

        sectype = Section::SEC_TYPE_NONE;
        if (bfd_flags & SEC_CODE)
        {
            sectype = Section::SEC_TYPE_CODE;
        }
        else if (bfd_flags & SEC_DATA)
        {
            sectype = Section::SEC_TYPE_DATA;
        }
        else
        {
            continue; // コードセクションでもデータセクションでもない場合はスキップ
        }

        vma = bfd_section_vma(bfd_handler, bfd_sec);
        size = bfd_section_size(bfd_handler, bfd_sec);
        secname = bfd_section_name(bfd_handler, bfd_sec);
        if (!secname)
            secname = "<unnamed>";

        bin->sections.push_back(Section()); // 一時オブジェクトを追加
        sec = &bin->sections.back();

        sec->binary = bin;
        sec->name = string(secname);
        sec->type = sectype;
        sec->vma = vma;
        sec->size = size;
        sec->bytes = (uint8_t *)malloc(size);
        if (!sec->bytes)
        {
            fprintf(stderr, "out of memory\n");
            return -1;
        }

        if (!bfd_get_section_contents(bfd_handler, bfd_sec, sec->bytes, 0, size))
        {
            fprintf(stderr, "failed to read section '%s' contents: %s\n", secname, bfd_errmsg(bfd_get_error()));
            return -1;
        }
    }
    return 0;
}

static int load_binary_bfd(string &fname, Binary *bin, Binary::BinaryType bintype)
{
    int ret;
    bfd *bfd_handler;
    const bfd_arch_info_type *arch_info;

    bfd_handler = NULL;

    bfd_handler = open_bfd(fname);
    if (!bfd_handler)
    {
        goto fail;
    }

    bin->filename = string(fname); // Cの文字列をC++のstringオブジェクトにコピー
    bin->entry = bfd_get_start_address(bfd_handler);

    bin->type_str = string(bfd_handler->xvec->name);
    switch (bfd_handler->xvec->flavour)
    {

    case bfd_target_elf_flavour:
        bin->type = Binary::BIN_TYPE_ELF;
        break;

    case bfd_target_coff_flavour:
        bin->type = Binary::BIN_TYPE_PE;
        break;

    case bfd_target_unknown_flavour:
    default:
        fprintf(stderr, "unsupported binary format '%s'¥n", bfd_handler->xvec->name);
        goto fail;
    }
    bfd_info = bfd_get_arch_info(bfd_handler);
    bin->arch_str = string(bfd_info->printable_name);
    switch (bfd_info->mach)
    {
    case bfd_mach_i386_i386:
        bin->arch = Binary::ARCH_X86;
        bin->bits = 32;
        break;
    case bfd_mach_x86_64:
        bin->arch = Binary::ARCH_X86;
        bin->bits = 64;
        break;
    default:
        fprintf(stderr, "unsupported architecture '%s'¥n", arch_info->printable_name);
        goto fail;
    }

    load_symbols_bfd(bfd_handler, bin);
    load_dynsym_bfd(bfd_handler, bin);

    if (load_sections_bfd(bfd_handler, bin) < 0)
        goto fail;

    ret = 0;
    goto cleanup;

fail:
    ret = -1;

cleanup:
    if (bfd_handler)
        bfd_close(bfd_handler);

    return ret;
}

int load_binary(string &fname, Binary *bin, Binary::BinaryType bintype)
{
    return load_binary_bfd(fname, bin, bintype);
}

void unload_binary(Binary *bin)
{
    size_t i;
    Section *sec;

    for (i = 0; i < bin->sections.size(); i++)
    {
        sec = &bin->sections[i];
        if (sec->bytes)
        {
            free(sec->bytes);
            sec->bytes = NULL;
        }
    }
}
