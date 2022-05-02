#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <stdio.h>   
#include <stdlib.h> 
#include <string.h> 
#include "zutil.h"
#include "zlib.h"
#include "lab_png.h"
#include "crc.h"

#define BUF_LEN  (256*16)
#define BUF_LEN2 (2000*2000)

const unsigned long MAX_MALLOC = 16000000;

U8 gp_buf_def[BUF_LEN2];
U8 gp_buf_inf[BUF_LEN2];
U8 img_id[8] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
U8 ihdr_extra[5] = {0x08, 0x06, 0x00, 0x00, 0x00};


char * str;
U64 tot = 0;
int new_width = 0;
int new_height = 0;
U64 len_def = 0;
U64 len_inf = 0;

void cat_helper(char * pref) {
	FILE * fp;
	struct stat buf;

	data_IHDR_p pointer_to_IHDR;
	chunk_p pointer_to_IHDR_data;
	chunk_p pointer_to_IDAT_data;
	
	pointer_to_IHDR = (data_IHDR_p)malloc(13);
	pointer_to_IHDR_data = (chunk_p)malloc(sizeof(struct chunk)); 	
	pointer_to_IDAT_data = (chunk_p)malloc(sizeof(struct chunk));

	fp = fopen(pref, "r");

	if (stat(pref, &buf) == 0 && S_ISREG(buf.st_mode)) {
		fflush(stdout);
		get_png_data_IHDR(pointer_to_IHDR, fp, 16, SEEK_SET);
        new_width = ntohl(pointer_to_IHDR->width);
		new_height += ntohl(pointer_to_IHDR->height);
		
		if (ntohl(pointer_to_IHDR->height) != 6) {
			printf("ERROR: we got the wrong height from image %s\n", pref);
			printf("ERROR: width: %d, height: %d\n", ntohl(pointer_to_IHDR->width), ntohl(pointer_to_IHDR->height));
			fflush(stdout);
		}

		get_png_chunks(pointer_to_IDAT_data, fp, 33, SEEK_SET);
        int inf_ret = mem_inf(&gp_buf_inf[tot], &len_inf, pointer_to_IDAT_data->p_data, ntohl(pointer_to_IDAT_data->length));
                
		if (inf_ret == 0) {
        	tot += len_inf;
        }
        else {
        	zerr(inf_ret);
        }
	}
	fclose(fp);
	
	free(pointer_to_IHDR);
	free(pointer_to_IHDR_data);
	free(pointer_to_IDAT_data->p_data);
	free(pointer_to_IDAT_data);
	return;
}

void cat_images( ) {

	mem_def(gp_buf_def, &len_def, gp_buf_inf, tot, Z_DEFAULT_COMPRESSION);
	
	FILE* fp = fopen("images/0.png", "r");
	FILE* all_image_pointer = fopen("all.png", "w+");

	data_IHDR_p pointer_to_IHDR;
    chunk_p pointer_to_IHDR_data;
    chunk_p pointer_to_IDAT_data;
    chunk_p pointer_to_IEND_data;

    pointer_to_IHDR = (data_IHDR_p)malloc(13);
    pointer_to_IHDR_data = (chunk_p)malloc(sizeof(struct chunk));
    pointer_to_IDAT_data = (chunk_p)malloc(sizeof(struct chunk));
    pointer_to_IEND_data = (chunk_p)malloc(sizeof(struct chunk));

	get_png_data_IHDR(pointer_to_IHDR, fp, 16, SEEK_SET);
	get_png_chunks(pointer_to_IHDR_data, fp, 8, SEEK_SET);
	get_png_chunks(pointer_to_IDAT_data, fp, 33, SEEK_SET);
	get_png_chunks(pointer_to_IEND_data, fp, ntohl(pointer_to_IDAT_data->length) + 45, SEEK_SET);

	unsigned char * tmp = (unsigned char*)malloc(MAX_MALLOC);
    unsigned long calculated_crc;

    // CREATING THE NEW IMAGE =====================================================

    // HEADER
    fwrite(img_id, sizeof(img_id), 1, all_image_pointer);
	
    // IHDR
    fwrite(pointer_to_IHDR_data, 1, 8, all_image_pointer);
    new_width = ntohl(new_width);
    new_height = ntohl(new_height);
    fwrite(&new_width, 4, 1, all_image_pointer);
    fwrite(&new_height, 4, 1, all_image_pointer);
    fwrite(ihdr_extra, sizeof(ihdr_extra), 1, all_image_pointer);

    fseek(all_image_pointer, 12, SEEK_SET);
    fread(tmp, 1, ntohl(pointer_to_IHDR_data->length) + 4, all_image_pointer);
    
	calculated_crc = crc(tmp, ntohl(pointer_to_IHDR_data->length) + 4);
    calculated_crc = ntohl(calculated_crc);
    fwrite(&calculated_crc, 4, 1, all_image_pointer); // SHOULD I BE NTOHL-ing THIS?

    //fwrite(&pointer_to_IHDR_data->crc, 4, 1, all_image_pointer); // THIS IS TEMPORARY. RECOMPUTE with crc()  :)

    // IDAT
    len_def = ntohl(len_def);
    fwrite(&len_def, 1, 4, all_image_pointer);
    len_def = ntohl(len_def);
    fwrite(&pointer_to_IDAT_data->type, 1, 4, all_image_pointer);
    fwrite(&gp_buf_def, 1, len_def, all_image_pointer);
    //fwrite(&pointer_to_IDAT_data->crc, 4, 1, all_image_pointer); // THIS IS TEMPORARY. RECOMPUTE

    fseek(all_image_pointer, 37, SEEK_SET);
    fread(tmp, 1, len_def + 4, all_image_pointer);

    calculated_crc = crc(tmp, len_def + 4);
    calculated_crc = ntohl(calculated_crc);
    fwrite(&calculated_crc, 4, 1, all_image_pointer); // ONCE AGAIN, SHOULD I BE NTOHL-ing THIS?

    // IEND
    fwrite(&pointer_to_IEND_data->length, 4, 1, all_image_pointer);
    fwrite(&pointer_to_IEND_data->type, 1, 4, all_image_pointer);
    fwrite(&pointer_to_IEND_data->crc, 4, 1, all_image_pointer); // THIS IS TEMPORARY. RECOMPUTE

    fseek(all_image_pointer, len_def + 45, SEEK_SET);
    fread(tmp, 1, 8, all_image_pointer);
    calculated_crc = crc(tmp, 8);
    calculated_crc = ntohl(calculated_crc);
    //fwrite(&calculated_crc, 4, 1, all_image_pointer); THIS IS THE RECOMPUTED ONE. FINDPNG DOESNT LIKE IT THOUGH. SO STICKING TO OLD
    //=============================================================================

	free(tmp);
	fclose(fp);
	fclose(all_image_pointer);
	free(pointer_to_IHDR);
	free(pointer_to_IHDR_data->p_data);
	free(pointer_to_IHDR_data);
	free(pointer_to_IDAT_data->p_data);
	free(pointer_to_IDAT_data);
	free(pointer_to_IEND_data->p_data);
	free(pointer_to_IEND_data);
	return;
}

