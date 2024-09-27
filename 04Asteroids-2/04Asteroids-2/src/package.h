#pragma once

#include "common.h"

void init_package();
void deinit_package();

u8* get_file(const char* fname, size_t* out_filesize);
string get_file_str(const char* fname);
