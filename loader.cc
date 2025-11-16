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