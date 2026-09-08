#ifndef LIBDANGANWAD_WAD_H
#define LIBDANGANWAD_WAD_H

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

// Macro definitions
#define MAX_PATHS 1000
#define MAX_PATH_LEN 512

// Structure definitions
typedef struct {
	uint32_t nameSize;
	char* name;
} wad_obj;

typedef struct {
	wad_obj obj;
	uint64_t fileSize; // 53 63 00 00
	uint64_t fileOffset; // 00 00 00 00
} wad_file;

typedef struct {
	wad_obj obj;
	bool isDir;
} wad_sub;

typedef struct {
	wad_obj obj;
	uint32_t fileCount;
	wad_sub* subs; // sub-directories or sub-files of the directory
} wad_dir;

typedef struct {
	// Internal
	FILE* wad_fptr;
	uint64_t wad_data_sec_off;

	// WAD Header
	char magic[4]; // 'AGAR'
	uint32_t versionMajor; // 01 00 00 00
	uint32_t versionMinor; // 01 00 00 00
	uint32_t headerDummy; // 00 00 00 00

	// WAD Files
	uint32_t fileCount; // 07 00 00 00
	wad_file *files;

	// WAD Directories
	uint32_t dirCount; // 07 00 00 00
	wad_dir *dirs;
} wad_data;

// Function definitions

/**
 * wad_data* wad_load(FILE*) - loads a wad file, given a
 * FILE*, enumerates its contents and returns the wad_data structure.
 *
 * @param fptr A file pointer to the opened wad file
 * @return wad_data* on success or NULL on failure.
 */
wad_data* wad_load(FILE* fptr);

/**
 * void wad_list_files(wad_data* wad_dta) - lists files in a wad file to
 * stdout.
 *
 * @param wad_data - the wad data structure
 */
void wad_list_files(wad_data* wad_dta);

/**
 * void wad_list_dirs(wad_data* wad_dta) - lists directories in a wad file to
 * stdout.
 *
 * @param wad_data - the wad data structure
 */
void wad_list_dirs(wad_data* wad_dta);

/**
 * void wad_list_dirs(wad_data* wad_dta) - lists every object in a wad file to
 * stdout.
 *
 * @param wad_data - the wad data structure
 */
void wad_list_all(wad_data* wad_dta);

/**
 * wad_file* wad_get_file(wad_data*, const char*) - get a file by name.
 *
 * @param wad_data - the wad data structure
 * @param name - the filename
 * @return the wad_file structure on success or null on failure
 */
wad_file* wad_get_file(wad_data* wad_dta, const char* name);

/**
 * long wad_get_file_offset(wad_data*, wad_file*) - get a file offset from
 * a wad_file struct
 *
 * @param wad_data - the wad data structure
 * @param wad_fle - the wad file structure
 * @return the offset on success or -1 on failure.
 */
long wad_get_file_offset(wad_data* wad_dta, wad_file* wad_fle);

/**
 * wad_dir* wad_get_dir(wad_data*, const char*) - get a directory by name.
 *
 * @param wad_data - the wad data structure
 * @param name - the dirname
 * @return the wad_dir structure on success or null on failure
 */
wad_dir* wad_get_dir(wad_data* wad_dta, const char* name);

/**
 * int wad_pack_from_dir(const char*, const char*) - repack a wad file based on
 * in_dir contents.
 *
 * @param in_dir - the directory structure handle
 * @param out_file - the output file handle
 * @return number of files written on success, < 0 on failure
 */
int wad_pack_from_dir(const char* in_dir, FILE* out_file);

/**
 * void wad_close(wad_data*) - closes a wad file.
 *
 * @param wad_dta the wad data structure
*/
void wad_close(wad_data* wad_dta);

#endif // LIBDANGANWAD_WAD_H
