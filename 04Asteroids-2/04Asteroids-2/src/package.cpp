#include "package.h"

void init_package() {
	
}

void deinit_package() {
	
}

u8* get_file(const char* fname, size_t* out_filesize) {
	return (u8*) SDL_LoadFile(fname, out_filesize); // @Leak!!!!!!!!!!!!!
}

string get_file_str(const char* fname) {
	size_t filesize;
	u8* filedata = get_file(fname, &filesize);

	if (filedata) {
		return {(char*) filedata, filesize};
	} else {
		return {}; // Make sure result.count is zero
	}
}
