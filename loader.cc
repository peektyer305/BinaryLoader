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
