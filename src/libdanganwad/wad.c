#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ftw.h>

#include <libdanganwad/wad.h>

#define MAX_DIRS	0xFFF
#define MAX_FILES	0xFFFFF
#define MAX_PATH 4096

// TODO: Does HM2/DR2 need different minor/majors?
#define WAD_MAJOR 0x1
#define WAD_MINOR 0x1

const char* dirNames[MAX_DIRS];
const char* fileNames[MAX_FILES];
long curr_dir = 0;
long curr_file = 0;

bool read_wad_obj(wad_data *wad_dta, wad_obj *obj)
{
	uint32_t wad_int_bfr;

	// Read file name size (1x uint32_t)
	if (fread(&wad_int_bfr, sizeof(uint32_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	obj->nameSize = wad_int_bfr;

	// Read file name
	if (obj->nameSize != 0)
	{
		obj->name = malloc(obj->nameSize + 1);
		if (fread(obj->name, obj->nameSize, 1, wad_dta->wad_fptr) != 1)
			return false;

		obj->name[obj->nameSize] = '\0';
	}

	return true;
}

bool read_wad_file(wad_data *wad_dta, wad_file *file)
{
	uint64_t war_lint_bfr;

	if (!read_wad_obj(wad_dta, &file->obj))
		return false;

	// Read file size (1x uint64_t)
	if (fread(&war_lint_bfr, sizeof(uint64_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	file->fileSize = war_lint_bfr;

	// Read file offset (1x uint64_t)
	if (fread(&war_lint_bfr, sizeof(uint64_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	file->fileOffset = war_lint_bfr;

	return true;
}

bool read_wad_files(wad_data *wad_dta)
{
	int wad_int_bfr;

	// Read the file count
	if (fread(&wad_int_bfr, sizeof(uint32_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	wad_dta->fileCount = wad_int_bfr;

	wad_dta->files = malloc(sizeof(wad_file) * wad_dta->fileCount);

	for (uint32_t i = 0; i < wad_dta->fileCount; i++)
		read_wad_file(wad_dta, &wad_dta->files[i]);

	return true;
}

bool read_wad_sub(wad_data *wad_dta, wad_sub *sub)
{
	if (!read_wad_obj(wad_dta, &sub->obj))
		return false;

	// Read directory/file status
	if (fread(&sub->isDir, 1, 1, wad_dta->wad_fptr) != 1)
		return false;

	return true;
}

bool read_wad_dir(wad_data *wad_dta, wad_dir *dir)
{
	int wad_int_bfr;

	if (!read_wad_obj(wad_dta, &dir->obj))
		return false;

	// Read file count (1x uint32_t)
	if (fread(&wad_int_bfr, sizeof(uint32_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	dir->fileCount = wad_int_bfr;

	dir->subs = malloc(sizeof(wad_sub) * dir->fileCount);

	// Read sub-contents
	for (uint32_t s = 0; s < dir->fileCount; s++)
		read_wad_sub(wad_dta, &dir->subs[s]);

	return true;
}

bool read_wad_dirs(wad_data *wad_dta)
{
	int wad_int_bfr;

	if (fread(&wad_int_bfr, sizeof(uint32_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	wad_dta->dirCount = wad_int_bfr;

	wad_dta->dirs = malloc(sizeof(wad_dir) * wad_dta->dirCount);

	for (uint32_t i = 0; i < wad_dta->dirCount; i++)
		read_wad_dir(wad_dta, &wad_dta->dirs[i]);

	return true;
}

bool read_wad_header(wad_data *wad_dta)
{
	unsigned char wad_mgc_bfr[8];
	int wad_ver_bfr;

	// Read wad magic ('AGAR')
	if (fread(wad_mgc_bfr, 4, 1, wad_dta->wad_fptr) != 1)
		return false;

	if (memcmp(wad_mgc_bfr, "AGAR", 4) != 0)
	{
		wad_mgc_bfr[4] = '\0';
		printf("%s: Invalid wadfile, AGAR != %s\n", __func__, wad_mgc_bfr);
		return false;
	}

	memcpy(wad_dta->magic, wad_mgc_bfr, 4);

	// Read wad version (maj.)
	if (fread(&wad_ver_bfr, sizeof(uint32_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	wad_dta->versionMajor = wad_ver_bfr;

	// Read wad version (min.)
	if (fread(&wad_ver_bfr, sizeof(uint32_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	wad_dta->versionMinor = wad_ver_bfr;

	// Read wad reserved header data
	if (fread(&wad_ver_bfr, sizeof(uint32_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	wad_dta->headerDummy = wad_ver_bfr;

	if (wad_dta->headerDummy != 0x0)
		printf("WAD header data is not 0x0!\nThis may not be an error, just an interesting spot.\n");

	return true;
}

const char* get_wad_obj_filename(const char* in_dir, const char* obj_real)
{
	char in_dir_real[MAX_PATH];

	if (realpath(in_dir, in_dir_real) == NULL)
		return NULL;

	size_t len = strlen(in_dir_real);

	if (strncmp(obj_real, in_dir_real, len) == 0 &&
		(obj_real[len] == '/' || obj_real[len] == '\0'))
	{
		const char *result = obj_real + len;

		if (*result == '/')
			result++;

		return result;
	}
	else
		return NULL;

	return NULL;
}

bool write_wad_header(wad_data* dta)
{
	// Write wad magic
	if (fwrite(dta->magic, 4, 1, dta->wad_fptr) != 1)
		return false;

	// Write wad version (maj.)
	if (fwrite(&dta->versionMajor, sizeof(uint32_t), 1, dta->wad_fptr) != 1)
		return false;

	// Write wad version (min.)
	if (fwrite(&dta->versionMinor, sizeof(uint32_t), 1, dta->wad_fptr) != 1)
		return false;

	// Write wad reserved header data
	if (fwrite(&dta->headerDummy, sizeof(uint32_t), 1, dta->wad_fptr) != 1)
		return false;

	return true;
}

bool write_wad_obj(FILE* fptr, wad_obj* obj)
{
	// Write file name length
	if (fwrite(&obj->nameSize, sizeof(uint32_t), 1, fptr) != 1)
		return false;

	// Write file name if size > 0
	if (obj->nameSize > 0)
	{
		if (fwrite(obj->name, obj->nameSize, 1, fptr) != 1)
			return false;
	}

	return true;
}

bool write_wad_file(FILE* fptr, wad_file* fle)
{
	if (!write_wad_obj(fptr, &fle->obj))
		return false;

	// Write file size
	if (fwrite(&fle->fileSize, sizeof(uint64_t), 1, fptr) != 1)
		return false;

	// Write file offset
	if (fwrite(&fle->fileOffset, sizeof(uint64_t), 1, fptr) != 1)
		return false;

	return true;
}

bool write_wad_sub(FILE* fptr, wad_sub* sub)
{
	if (!write_wad_obj(fptr, &sub->obj))
		return false;

	if (fwrite(&sub->isDir, sizeof(bool), 1, fptr) != 1)
		return false;

	return true;
}

bool write_wad_dir(FILE* fptr, wad_dir* dir)
{
	bool ret = false;

	if (!write_wad_obj(fptr, &dir->obj))
		return false;

	// Write file count
	if (fwrite(&dir->fileCount, sizeof(uint32_t), 1, fptr) != 1)
		return false;

	// Write subs
	for (uint32_t i = 0; i < dir->fileCount; i++)
		ret = write_wad_sub(fptr, &dir->subs[i]);

	return ret;
}

bool write_wad_files(wad_data* dta)
{
	if (fwrite(&dta->fileCount, sizeof(uint32_t), 1, dta->wad_fptr) != 1)
		return false;

	for (uint32_t i = 0; i < dta->fileCount; i++)
	{
		if (!write_wad_file(dta->wad_fptr, &dta->files[i]))
			return false;
	}

	return true;
}

bool write_wad_dirs(wad_data* dta)
{
	if (fwrite(&dta->dirCount, sizeof(uint32_t), 1, dta->wad_fptr) != 1)
		return false;

	for (uint32_t i = 0; i < dta->dirCount; i++)
	{
		if (!write_wad_dir(dta->wad_fptr, &dta->dirs[i]))
			return false;
	}

	return true;
}

void add_dir(const char* filepath)
{
	dirNames[curr_dir] = strdup(filepath);
	curr_dir++;
}

void add_fle(const char* filepath)
{
	fileNames[curr_file] = strdup(filepath);
	curr_file++;
}

int print_entry(const char *filepath, const struct stat *info,
				const int typeflag, struct FTW *pathinfo)
{
	(void)info;
	(void)pathinfo;

	if (typeflag == FTW_D) // Directory
		add_dir(filepath);
	else if (typeflag == FTW_F) // File
		add_fle(filepath);
	else
	{
		printf("Unknown typeflag %i!\tFile: %s\n", typeflag, filepath);
		return -1;
	}

	return 0;
}

int populate_dirs(const char* dir)
{
	return nftw(dir, print_entry, 15, FTW_PHYS);
}

long int get_file_size(char file_name[])
{
	FILE* fp = fopen(file_name, "r");

	if (fp == NULL)
		return -1;

	fseek(fp, 0L, SEEK_END);

	long int res = ftell(fp);

	fclose(fp);

	return res;
}

wad_data* wad_load(FILE* fptr)
{
	wad_data *wad_dta;

	if (!fptr)
		return NULL;

	wad_dta = (wad_data*)malloc(sizeof(wad_data));
	if (!wad_dta)
	{
		printf("%s: Invalid malloc\n", __func__);
		goto late_fail_exit;
	}

	wad_dta->wad_fptr = fptr;

	if (!read_wad_header(wad_dta))
		goto late_fail_exit;

	if (!read_wad_files(wad_dta))
		goto late_fail_exit;

	if (!read_wad_dirs(wad_dta))
		goto late_fail_exit;

	wad_dta->wad_data_sec_off = ftell(wad_dta->wad_fptr);

	return wad_dta;

late_fail_exit:
	free(wad_dta);
	return NULL;
}

void wad_list_files(wad_data* wad_dta)
{
	printf("WAD contains %i files:\n\n", wad_dta->fileCount);

	printf("\tName\tSize (bytes)\tOffset (bytes, relative to wad_data_sec_off)\n");
	printf("------------------------------------------------------------------\n");

	for (uint32_t i = 0; i < wad_dta->fileCount; i++)
		printf("\t%s\t%lu\t0x%lx\n", wad_dta->files[i].obj.name, wad_dta->files[i].fileSize, wad_dta->files[i].fileOffset);
}

void wad_list_dirs(wad_data* wad_dta)
{
	printf("WAD contains %i directories:\n\n", wad_dta->dirCount);

	printf("\tName\n");
	printf("------------------------------------------------------------------\n");

	for (uint32_t i = 0; i < wad_dta->dirCount; i++)
	{
		printf("\t |%s\n", wad_dta->dirs[i].obj.name);
		if (wad_dta->dirs[i].fileCount != 0)
		{
			for (uint32_t s = 0; s < wad_dta->dirs[i].fileCount; s++)
				if (wad_dta->dirs[i].subs[s].isDir)
					printf("\t | - %s/\n", wad_dta->dirs[i].subs[s].obj.name);
				else
					printf("\t | - %s\n", wad_dta->dirs[i].subs[s].obj.name);
		}
	}
}

void wad_list_all(wad_data* wad_dta)
{
	wad_list_files(wad_dta);
	wad_list_dirs(wad_dta);
}

wad_file* wad_get_file(wad_data* wad_dta, const char* name)
{
	for (uint32_t i = 0; i < wad_dta->fileCount; i++)
		if (strcmp(wad_dta->files[i].obj.name, name) == 0)
			return &wad_dta->files[i];

	return NULL;
}

long wad_get_file_offset(wad_data* wad_dta, wad_file* wad_fle)
{
	return wad_dta->wad_data_sec_off + wad_fle->fileOffset;
}

wad_dir* wad_get_dir(wad_data* wad_dta, const char* name)
{
	for (uint32_t i = 0; i < wad_dta->dirCount; i++)
		if (wad_dta->dirs[i].obj.name) // name may be null for the root object
			if (strcmp(wad_dta->dirs[i].obj.name, name) == 0)
				return &wad_dta->dirs[i];

	return NULL;
}

int wad_pack_from_dir(const char* in_dir, FILE* out_file)
{
	wad_data *wad_dta;
	char obj_real[MAX_PATH];
	const char* fn_result;
	int fileOffset = 0;
	int dirOffset = 0;

	(void)dirOffset;

	if (!in_dir || !out_file)
		return -1;

	if (populate_dirs(in_dir))
		return -2;

	// Create a wad_data structure
	wad_dta = (wad_data*)malloc(sizeof(wad_data));
	if (!wad_dta)
	{
		printf("%s: Invalid malloc\n", __func__);
		goto late_fail_exit;
	}

	// Set up wad_data
	wad_dta->wad_fptr = out_file;
	wad_dta->magic[0] = 'A';
	wad_dta->magic[1] = 'G';
	wad_dta->magic[2] = 'A';
	wad_dta->magic[3] = 'R';
	wad_dta->versionMajor = WAD_MAJOR;
	wad_dta->versionMinor = WAD_MINOR;
	wad_dta->headerDummy = 0x0;
	wad_dta->dirCount = curr_dir;
	wad_dta->fileCount = curr_file;

	wad_dta->files = calloc(curr_file, sizeof(wad_file));
	if (wad_dta->files == NULL)
		return -3;

	wad_dta->dirs = calloc(curr_dir, sizeof(wad_dir));
	if (wad_dta->dirs == NULL)
		return -4;

	// Set up wad_file(s)
	for (uint32_t i = 0; i < curr_file; i++)
	{
		int filesize;

		// Object
		if (realpath(fileNames[i], obj_real) == NULL)
			return -5;

		fn_result = get_wad_obj_filename(in_dir, obj_real);
		wad_dta->files[i].obj.name = strdup(fn_result);
		wad_dta->files[i].obj.nameSize = strlen(fn_result);

		// File
		filesize = get_file_size(obj_real);
		wad_dta->files[i].fileSize = filesize;
		wad_dta->files[i].fileOffset = fileOffset;

		fileOffset += filesize;
	}

	// Set up wad_dir(s)
	for (uint32_t i = 0; i < curr_dir; i++)
	{
		struct dirent *de;
		int subCount = 0;

		// Object
		if (!dirNames[i])
			continue;

		if (realpath(dirNames[i], obj_real) == NULL)
			return -6;

		fn_result = get_wad_obj_filename(in_dir, obj_real);
		wad_dta->dirs[i].obj.name = strdup(fn_result);
		wad_dta->dirs[i].obj.nameSize = strlen(fn_result);

		// Dir
		DIR *dr = opendir(obj_real);

		if (dr == NULL)
			return -7;

		while ((de = readdir(dr)) != NULL)
		{
			if (de->d_name[0] != '.')
			{
				wad_sub *tmp = realloc(
					wad_dta->dirs[i].subs,
					sizeof(wad_sub) * (subCount + 1)
				);

				if (tmp == NULL)
				{
					closedir(dr);
					return -8;
				}

				wad_dta->dirs[i].subs = tmp;
				wad_dta->dirs[i].subs[subCount].isDir = (de->d_type == DT_DIR);
				wad_dta->dirs[i].subs[subCount].obj.name = strdup(de->d_name);

				if (wad_dta->dirs[i].subs[subCount].obj.name == NULL)
				{
					closedir(dr);
					return -9;
				}

				wad_dta->dirs[i].subs[subCount].obj.nameSize = strlen(de->d_name);

				subCount++;
			}
		}
		wad_dta->dirs[i].fileCount = subCount;
		closedir(dr);
	}

	if (!write_wad_header(wad_dta))
		goto late_fail_exit;

	if (!write_wad_files(wad_dta))
		goto late_fail_exit;

	if (!write_wad_dirs(wad_dta))
		goto late_fail_exit;

	for (uint32_t i = 0; i < curr_file; i++) {
		FILE* in_f = fopen(fileNames[i], "rb");
		if (in_f) {
			char buffer[8192];
			size_t bytes;
			while ((bytes = fread(buffer, 1, sizeof(buffer), in_f)) > 0) {
				fwrite(buffer, 1, bytes, out_file);
			}
			fclose(in_f);
		}
	}

	wad_close(wad_dta);

	for (long i = 0; i < curr_dir; i++) free((void*)dirNames[i]);
	for (long i = 0; i < curr_file; i++) free((void*)fileNames[i]);
	curr_dir = 0;
	curr_file = 0;

	return 0;

late_fail_exit:
	// If we fail early, we still need to clean up globals
	for (long i = 0; i < curr_dir; i++) free((void*)dirNames[i]);
	for (long i = 0; i < curr_file; i++) free((void*)fileNames[i]);
	curr_dir = 0;
	curr_file = 0;

	if (wad_dta) wad_close(wad_dta);
	return -1;
}

void wad_close(wad_data* wad_dta)
{
	if (!wad_dta) return;

	for (uint32_t i = 0; i < wad_dta->fileCount; i++)
		free(wad_dta->files[i].obj.name);

	free(wad_dta->files);

	for (uint32_t i = 0; i < wad_dta->dirCount; i++)
	{
		for (uint32_t s = 0; s < wad_dta->dirs[i].fileCount; s++)
			free(wad_dta->dirs[i].subs[s].obj.name);

		free(wad_dta->dirs[i].subs);
		free(wad_dta->dirs[i].obj.name);
	}

	free(wad_dta->dirs);
	free(wad_dta);
}
