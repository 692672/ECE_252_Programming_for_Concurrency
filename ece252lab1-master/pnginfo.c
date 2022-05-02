#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lab_png.h"
#include "crc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const unsigned long MAX_MALLOC = 16000000;

void init_data(U8 *buf, int len)
{
    int i;
    for ( i = 0; i < len; i++) {
        buf[i] = i%256;
    }
}

int main(int argc, char * argv[]) {
		
	int i;
	int realpng = 0;
	int file_size = 0;
	int is_corrupt = 0;
	char * file_name;
	char * directory;
	FILE * pngpointer;
	unsigned char * tmp = malloc(MAX_MALLOC);
 	unsigned char png_header [8];

	simple_PNG_p png_file_format;
	data_IHDR_p pointer_to_IHDR_data;
	chunk_p pointer_to_IHDR_chunk;
	chunk_p pointer_to_IDAT_data;
	chunk_p pointer_to_IEND_data;

	png_file_format = (simple_PNG_p)malloc(sizeof(simple_PNG_p));
	pointer_to_IHDR_data = (data_IHDR_p)malloc(13);
	pointer_to_IHDR_chunk = (chunk_p)malloc(sizeof(chunk_p));
	pointer_to_IDAT_data = (chunk_p)malloc(sizeof(chunk_p));
	pointer_to_IEND_data = (chunk_p)malloc(sizeof(chunk_p));
	
	if (argc == 1) {
		printf("Please pass in a file path");
		return -1;
	}
    
	for (i = 0; i < argc-1; i++) {
	    directory = argv[i + 1];
	}

	file_name = get_file_name(directory);
	pngpointer = fopen(directory, "rb");

	fread(png_header, 1, sizeof(png_header), pngpointer);
	get_png_data_IHDR(pointer_to_IHDR_data, pngpointer, 16, SEEK_SET);
	realpng = is_png(png_header, sizeof(png_header));

	if (realpng != 1){
		printf("%s: Not a PNG file\n", file_name);
	} else {
		printf("%s: %u x %u\n", file_name, ntohl(pointer_to_IHDR_data->width), ntohl(pointer_to_IHDR_data->height));
		
		get_png_chunks(pointer_to_IHDR_chunk, pngpointer, 8, SEEK_SET);
        fseek(pngpointer, 12, SEEK_SET);
        fread(tmp, 1, ntohl(pointer_to_IHDR_chunk->length) + 4, pngpointer);
        is_corrupt = compare_crcs(tmp, pointer_to_IHDR_chunk->crc, ntohl(pointer_to_IHDR_chunk->length) + 4, "IHDR");

		if (is_corrupt == 0) {
			get_png_chunks(pointer_to_IDAT_data, pngpointer, 33, SEEK_SET);
        	fseek(pngpointer, 37, SEEK_SET);
        	fread(tmp, 1, ntohl(pointer_to_IDAT_data->length) + 4, pngpointer);
			compare_crcs(tmp, pointer_to_IDAT_data->crc, ntohl(pointer_to_IDAT_data->length) + 4, "IDAT");
		}

		if (is_corrupt == 0) {
		    get_png_chunks(pointer_to_IEND_data, pngpointer, ntohl(pointer_to_IDAT_data->length) + 45, SEEK_SET);
		    fseek(pngpointer, ntohl(pointer_to_IDAT_data->length) + 49, SEEK_SET);
            fread(tmp, 1, ntohl(pointer_to_IEND_data->length) + 4, pngpointer);
			compare_crcs(tmp, pointer_to_IEND_data->crc, ntohl(pointer_to_IEND_data->length) + 4, "IEND"); 
	    }
	}

    fclose(pngpointer);
	free(tmp);
	free(png_file_format);
	free(pointer_to_IHDR_chunk->p_data);
	free(pointer_to_IDAT_data->p_data);
	free(pointer_to_IEND_data->p_data);
	free(pointer_to_IHDR_data);
	free(pointer_to_IHDR_chunk);
	free(pointer_to_IDAT_data);
	free(pointer_to_IEND_data);	
	return 0;
}

