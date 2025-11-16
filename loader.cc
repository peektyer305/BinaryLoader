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
