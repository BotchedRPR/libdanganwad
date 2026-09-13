#include <libdanganwad/wad.h>

// Filenames
#if !defined(SRCDIR)
#warning "Please define SRCDIR!"
#define SRCDIR "./"
#endif

#define TEST0_FILENAME	SRCDIR "tests/data/repack_test_data_00/"
#define TEST1_FILENAME	SRCDIR "tests/data/repack_test_data_01/"
#define TEST2_FILENAME	SRCDIR "tests/data/repack_test_data_02/"

// Repack test data
#define FILE0_HASH	0x3523102f
#define FILE1_HASH	0x143225f2
#define FILE2_HASH	0xff7e7290

#define CRC32_POLY 0xEDB88320

uint32_t crc32_file(const char *filename)
{
	static uint32_t table[256];
	static int table_initialized = 0;
	uint32_t crc = 0xFFFFFFFF;
	int ch;

	FILE *file = fopen(filename, "rb");
	if (!file) return 0;

	if (!table_initialized) {
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t remainder = i;
			for (int j = 0; j < 8; j++) {
				if (remainder & 1) {
					remainder = (remainder >> 1) ^ CRC32_POLY;
				} else {
					remainder >>= 1;
				}
			}
			table[i] = remainder;
		}
		table_initialized = 1;
	}

	while ((ch = fgetc(file)) != EOF) {
		uint8_t byte = (uint8_t)ch;
		crc = (crc >> 8) ^ table[(crc & 0xFF) ^ byte];
	}

	fclose(file);
	return ~crc; // Invert bits for the final result
}

int pack_wad_tmp(char* fName)
{
	FILE* wad_ptr;
	int ret = 0;

	wad_ptr = fopen("tmp.wad", "wb");
	if (!wad_ptr)
	{
		printf("Invalid output filename\n");
		return -2;
	}

	// To avoid conflicts between the wad header and the file contents,
	// the entire repacking process is handled by libdanganwad.
	ret = wad_pack_from_dir(fName, wad_ptr);
	if (ret)
		printf("Repacking WAD failed!\n");

	fclose(wad_ptr);
	return ret;
}

int run_test_00(void)
{
	int ret = 0;

	ret = pack_wad_tmp(TEST0_FILENAME);
	if (ret != 0)
		goto clean;

	if (crc32_file("tmp.wad") != FILE0_HASH)
		ret = -1;

clean:
	remove("tmp.wad");
	return ret;
}

int run_test_01(void)
{
	int ret = 0;

	ret = pack_wad_tmp(TEST1_FILENAME);
	if (ret != 0)
		goto clean;

	if (crc32_file("tmp.wad") != FILE1_HASH)
		ret = -1;

clean:
	remove("tmp.wad");
	return ret;
}

int run_test_02(void)
{
	int ret = 0;

	ret = pack_wad_tmp(TEST2_FILENAME);
	if (ret != 0)
		goto clean;

	if (crc32_file("tmp.wad") != FILE2_HASH)
		ret = -1;

clean:
	remove("tmp.wad");
	return ret;
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Invalid number of arguments!\n\twad_test_repack<test no>\n");
		return -1;
	}

	switch(argv[1][0])
	{
		// Pack a single file to a wad
		case '0':
			return run_test_00();

		// Pack multiple files to a wad
		case '1':
			return run_test_01();

		// Pack multiple files in multiple directories to a wad
		case '2':
			return run_test_02();
	}

	return -1;
}
