#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <libdanganwad/wad.h>

#define WORKS_STRING "It works!"
#define WORKS_STRING_LEN 10

int extract_wad(char* fName, char* outName)
{
	FILE *wad_ptr;
	FILE *out_fle_ptr;
	wad_data *dta;
	wad_file *fle;
	unsigned char* fileDataBfr;
	char bfr[512];
	int ret = 0;

	wad_ptr = fopen(fName, "rb");

	if (!wad_ptr)
	{
		printf("Invalid wadfile\n");
		return -1;
	}

	dta = wad_load(wad_ptr);
	if (!dta)
	{
		printf("Invalid wadfile\n");
		fclose(wad_ptr);

		return -2;
	}

	mkdir(outName, 0777);

	for (uint32_t i = 0; i < dta->dirCount; i++)
	{
		if (dta->dirs[i].obj.name)
		{
			sprintf(bfr, "%s/%s", outName, dta->dirs[i].obj.name);
			mkdir(bfr, 0777);
		}
	}

	for (uint32_t i = 0; i < dta->fileCount; i++)
	{
		fle = wad_get_file(dta, dta->files[i].obj.name);

		if (!fle)
		{
			printf("File %s is missing from wad archive.\n", dta->files[i].obj.name);
			continue;
		}

		printf("\33[2K\rProcessing file [%i/%i]...\t\t\t%s", i, dta->fileCount, dta->files[i].obj.name);

		fseek(wad_ptr, wad_get_file_offset(dta, fle), SEEK_SET);
		fileDataBfr = malloc(fle->fileSize);

		if (fread(fileDataBfr, fle->fileSize, 1, wad_ptr) != 1)
		{
			printf("File %s failed to read.\n", dta->files[i].obj.name);
			free(fileDataBfr);

			ret++;
			continue;
		}

		sprintf(bfr, "%s/%s", outName, dta->files[i].obj.name);

		out_fle_ptr = fopen(bfr, "wb");

		if (fwrite(fileDataBfr, fle->fileSize, 1, out_fle_ptr) != 1)
		{
			printf("File %s failed to write.\n", dta->files[i].obj.name);
			ret++;
		}

		free(fileDataBfr);
		fclose(out_fle_ptr);
		fflush(stdout);
	}

	wad_close(dta);
	fclose(wad_ptr);

	return ret;
}

int extract_wad_file(char* fName, char* tName, char* outName)
{
	FILE *wad_ptr;
	FILE *out_fle_ptr;
	wad_data *dta;
	wad_file *fle;
	unsigned char* fileDataBfr;
	int ret = 0;

	wad_ptr = fopen(fName, "rb");

	if (!wad_ptr)
	{
		printf("Invalid wadfile\n");
		return -1;
	}

	dta = wad_load(wad_ptr);
	if (!dta)
	{
		printf("Invalid wadfile\n");
		ret = -2;

		goto file_exit;
	}

	fle = wad_get_file(dta, tName);

	if (!fle)
	{
		printf("File %s is missing from wad archive.\n", tName);
		ret = -3;

		goto wad_exit;
	}

	printf("Processing file %s...\n", tName);

	fseek(wad_ptr, wad_get_file_offset(dta, fle), SEEK_SET);
	fileDataBfr = malloc(fle->fileSize);

	if (fread(fileDataBfr, fle->fileSize, 1, wad_ptr) != 1)
	{
		printf("File %s failed to read.\n", tName);
		ret = -4;

		goto bfr_exit;
	}

	out_fle_ptr = fopen(outName, "wb");

	if (fwrite(fileDataBfr, fle->fileSize, 1, out_fle_ptr) != 1)
	{
		printf("File %s failed to write.\n", tName);
		ret = -5;

		// out_file_exit, but its unneeded
	}

	//out_file_exit:
	fclose(out_fle_ptr);
	bfr_exit:
	free(fileDataBfr);
	wad_exit:
	wad_close(dta);
	file_exit:
	fclose(wad_ptr);

	return ret;
}

static int verify_single_file(const char* fName)
{
	char verify_str[WORKS_STRING_LEN];

	FILE* testdata = fopen(fName, "r");
	if (!testdata)
		return -1;

	if (fread(verify_str, WORKS_STRING_LEN, 1, testdata) != 1)
		return -2;

	if (strcmp(verify_str, WORKS_STRING) != 0)
		return -3;

	fclose(testdata);
	remove(fName);

	return 0;
}

static int verify_multi_files(const char* fName1, const char* fName2)
{
	char verify_str[WORKS_STRING_LEN];

	FILE* testdata = fopen(fName1, "r");
	if (!testdata)
		return -1;

	// fName1 -> 'It '
	if (fread(verify_str, 3, 1, testdata) != 1)
	{
		fclose(testdata);
		remove(fName1);
		return -2;
	}

	fclose(testdata);
	remove(fName1);

	testdata = fopen(fName2, "r");
	if (!testdata)
		return -3;

	// fName2 -> 'works!'
	if (fread(verify_str + 3, 6, 1, testdata) != 1)
	{
		fclose(testdata);
		remove(fName1);
		return -4;
	}

	fclose(testdata);
	remove(fName2);

	if (strcmp(verify_str, WORKS_STRING) != 0)
		return -5;

	return 0;
}

int run_test_00(void)
{
	int ret = 0;

	ret = extract_wad_file("tests/data/test_data_00_single_file.wad", "data.dat", "tmp.dat");
	if (!ret)
		return ret;

	return verify_single_file("data.dat");
}

int run_test_01(void)
{
	int ret = 0;

	ret = extract_wad_file("tests/data/test_data_01_single_file_in_folder.wad", "dataDir/data.dat", "tmp.dat");
	if (!ret)
		return ret;

	return verify_single_file("data.dat");
}

int run_test_02(void)
{
	int ret = 0;

	ret = extract_wad("tests/data/test_data_02_multiple_files_in_root.wad", "out");
	if (ret != 0)
		goto clean;

	ret = verify_multi_files("out/data00.dat", "out/data01.dat");

clean:
		rmdir("out");
		return ret;
}

int run_test_03(void)
{
	int ret = 0;

	ret = extract_wad("tests/data/test_data_03_files_in_multiple_directories.wad", "out");
	if (ret != 0)
		goto clean;

	ret = verify_multi_files("out/data00/data.dat", "out/data01/data.dat");

clean:
	rmdir("out/data00");
	rmdir("out/data01");
	rmdir("out");
	return ret;
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Invalid number of arguments!\n\twad_test_extract <test no>\n");
		return -1;
	}

	switch(argv[1][0])
	{
		// Extract single file from root dir of wad
		case '0':
			return run_test_00();

		// Extract single file from a child dir of wad
		case '1':
			return run_test_01();

		// Extract multiple files from a root dir of wad
		case '2':
			return run_test_02();

		// Extract multiple files from a child dir of wad
		case '3':
			return run_test_03();
	}

	return -1;
}
