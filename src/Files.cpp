#include <stdio.h>
#include <malloc.h>
#include "Files.h"

bool SaveFile(const char *filename, void* data, size_t size)
{
	FILE* f;
	if (fopen_s(&f, filename, "w") == 0 && f != nullptr) {
		fwrite(data, size, 1, f);
		fclose(f);
		return true;
	}
	return false;
}

uint8_t* LoadBinary(const char* name, size_t& size)
{
	FILE* f;
	if (fopen_s(&f, name, "rb") == 0 && f) {
		fseek(f, 0, SEEK_END);
		size_t sizeAdd = ftell(f);
		fseek(f, 0, SEEK_SET);
		uint8_t* add = (uint8_t*)malloc(sizeAdd);
		if (add) { fread(add, sizeAdd, 1, f); }
		fclose(f);
		size = sizeAdd;
		return add;
	}
	return nullptr;
}
