#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libdanganwad/wad.h>

bool read_wad_file(wad_data *wad_dta, wad_file *file)
{
	int wad_int_bfr;
	uint64_t war_lint_bfr;

	// Read file name size (1x uint32_t)
	if (fread(&wad_int_bfr, sizeof(uint32_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	file->obj.nameSize = wad_int_bfr;

	// Read file name
	file->obj.name = malloc(file->obj.nameSize + 1);
	if (fread(file->obj.name, file->obj.nameSize, 1, wad_dta->wad_fptr) != 1)
		return false;

	file->obj.name[file->obj.nameSize] = '\0';

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
	int wad_int_bfr;

	// Read subdir/subfile name size (1x uint32_t)
	if (fread(&wad_int_bfr, sizeof(uint32_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	sub->obj.nameSize = wad_int_bfr;

	// Read subdir/subfile name
	sub->obj.name = malloc(sub->obj.nameSize + 1);
	if (fread(sub->obj.name, sub->obj.nameSize, 1, wad_dta->wad_fptr) != 1)
		return false;

	sub->obj.name[sub->obj.nameSize] = '\0';

	// Read directory/file status
	if (fread(&sub->isDir, 1, 1, wad_dta->wad_fptr) != 1)
		return false;

	return true;
}

bool read_wad_dir(wad_data *wad_dta, wad_dir *dir)
{
	int wad_int_bfr;

	// Read dir name size (1x uint32_t)
	if (fread(&wad_int_bfr, sizeof(uint32_t), 1, wad_dta->wad_fptr) != 1)
		return false;

	dir->obj.nameSize = wad_int_bfr;

	// Read file name
	if (dir->obj.nameSize != 0)
	{
		dir->obj.name = malloc(dir->obj.nameSize + 1);
		if (fread(dir->obj.name, dir->obj.nameSize, 1, wad_dta->wad_fptr) != 1)
			return false;

		dir->obj.name[dir->obj.nameSize] = '\0';
	}

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

void wad_close(wad_data* wad_dta)
{
	free(wad_dta);
}
