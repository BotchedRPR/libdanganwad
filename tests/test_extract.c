#include <stdlib.h>

#include <libdanganwad/wad.h>

int main()
{
	FILE *wad_ptr;
	wad_data *dta;

	wad_ptr = fopen("/home/igor/Downloads/Danganronpa: Trigger Happy Havoc!/Danganronpa: Trigger Happy Havoc!/dr1_data.wad", "rb");

	if (!wad_ptr)
	{
		printf("Invalid wadfile\n");
		exit(-1);
	}

	dta = wad_load(wad_ptr);
	if (!dta)
	{
		printf("Invalid wadfile\n");
		exit(-1);
	}
}
