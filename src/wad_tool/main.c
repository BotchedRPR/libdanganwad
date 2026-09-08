#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <libdanganwad/wad.h>

// DEBUG
#define debug(...) printf(__VA_ARGS__)

int print_help()
{
	printf("libdanganwad wad_tool\n");
	printf("\n\twad_tool <operation> <inputs> <output name>\n");
	printf("\nOperations:\n");
	printf("\n\te - Extract all files and directories from a wad file [in: wadfile]\n");
	printf("\n\tf - Extract a single file from a wad file [in: wadfile filename]\n");
	return -1;
}

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

	return 0;
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

int main(int argc, char** argv)
{
	if (argc < 3)
		return print_help();

	if (argv[1][0] == 'e')
		return extract_wad(argv[2], argv[3]);

	if (argv[1][0] == 'f')
		return extract_wad_file(argv[2], argv[3], argv[4]);

	return print_help();
}
