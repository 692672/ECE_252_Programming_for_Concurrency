#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "lab_png.h"
#include "crc.h"
#include "main_write_callback.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MALLOC 16000000

char * get_file_name(char * directory) {
    char * file_name;

    for (int i = strlen(directory)-1; i >= 0; i--) {
        if (directory[i] == '/') {
            file_name = &directory[i + 1];
            break;
        }
    }
    if (file_name != NULL && file_name[0] == '\0')
    {
        file_name = directory;
    }

    return file_name;
}

int is_png(U8 * buf, size_t n) {
    unsigned i;

    for (i = 0; i < n; i++) {
        if (i == 0 && buf[i] != 0x89) {
            return 0;
        } else if (i == 1 && buf[i] != 0x50) {
            return 0;
        } else if (i == 2 && buf[i] != 0x4E) {
            return 0;
        } else if (i == 3 && buf[i] != 0x47) {
            return 0;
        } else if (i == 4 && buf[i] != 0x0D) {
            return 0;
        } else if (i == 5 && buf[i] != 0x0A) {
            return 0;
        } else if (i == 6 && buf[i] != 0x1A) {
            return 0;
        } else if (i == 7 && buf[i] != 0x0A) {
            return 0;
        }
    }

    return 1;
}

int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence) {
    fseek(fp, offset, whence);
    fread(out, 1, 13, fp);	
    return 0;
}

int get_png_chunks(struct chunk * out, FILE *fp, long offset, int whence) {
    fseek(fp, offset, whence);
    fread(&out->length, 1, 4, fp);
    fread(out->type, 1, 4, fp);
    out->p_data = (unsigned char *)malloc(((int)ntohl(out->length)) * sizeof(U8));
    fread(out->p_data, 1, (int)ntohl(out->length)*sizeof(U8), fp);
    fread(&out->crc, 1, sizeof(out->crc), fp);

    return 0;
}

int get_png_chunks_buf(struct chunk * out, char * source, long offset) {
	memcpy(&out->length, source + offset, sizeof(int));
	offset += sizeof(int);
	memcpy(out->type, source + offset, 4 * sizeof(char));
	offset += (4 * sizeof(char));
	out->p_data = (unsigned char *)malloc(((int)ntohl(out->length)) * sizeof(U8));
	memcpy(out->p_data, source + offset, (int)ntohl(out->length)*sizeof(U8));
	offset += (int)ntohl(out->length)*sizeof(U8);
	memcpy(&out->crc, source + offset, sizeof(out->crc));
	return 0;
}

int compare_crcs(unsigned char * crc_to_calculate, U32 read_crc, unsigned long len, char * chunk_name) {
    unsigned long calculated_crc = 0;

    calculated_crc = crc(crc_to_calculate, len);

    if (calculated_crc != ntohl(read_crc)) {
        printf("%s chunk CRC error: computed %lx, expected: %lx\n", chunk_name, calculated_crc, (unsigned long)ntohl(read_crc));
        return -1;
    }
    return 0;
}

int is_good_png(char * directory) {

	int realpng = 0;
	int is_corrupt = 0;
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
	pointer_to_IHDR_chunk = (chunk_p)malloc(sizeof(struct chunk));
	pointer_to_IDAT_data = (chunk_p)malloc(sizeof(struct chunk));
	pointer_to_IEND_data = (chunk_p)malloc(sizeof(struct chunk));

	pngpointer = fopen(directory, "rb");

	fread(png_header, 1, sizeof(png_header), pngpointer);
	get_png_data_IHDR(pointer_to_IHDR_data, pngpointer, 16, SEEK_SET);
	realpng = is_png(png_header, sizeof(png_header));

	if (realpng != 1){
		is_corrupt = -1;
	} else {
		get_png_chunks(pointer_to_IHDR_chunk, pngpointer, 8, SEEK_SET);
        fseek(pngpointer, 12, SEEK_SET);
        fread(tmp, 1, ntohl(pointer_to_IHDR_chunk->length) + 4, pngpointer);
        is_corrupt = compare_crcs(tmp, pointer_to_IHDR_chunk->crc, ntohl(pointer_to_IHDR_chunk->length) + 4, "IHDR");
		

		if (is_corrupt == 0) {
			get_png_chunks(pointer_to_IDAT_data, pngpointer, 33, SEEK_SET);
        	fseek(pngpointer, 37, SEEK_SET);
        	fread(tmp, 1, ntohl(pointer_to_IDAT_data->length) + 4, pngpointer);
			is_corrupt = compare_crcs(tmp, pointer_to_IDAT_data->crc, ntohl(pointer_to_IDAT_data->length) + 4, "IDAT");
		}

		if (is_corrupt == 0) {
		    get_png_chunks(pointer_to_IEND_data, pngpointer, ntohl(pointer_to_IDAT_data->length) + 45, SEEK_SET);
		    fseek(pngpointer, ntohl(pointer_to_IDAT_data->length) + 49, SEEK_SET);
            fread(tmp, 1, ntohl(pointer_to_IEND_data->length) + 4, pngpointer);
			is_corrupt = compare_crcs(tmp, pointer_to_IEND_data->crc, ntohl(pointer_to_IEND_data->length) + 4, "IEND");
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
	return is_corrupt;
}

int is_good_png_buf(RECV_BUF png_buf) {
	int realpng = 0;
    int is_corrupt = 0;
    unsigned char * tmp = malloc(MAX_MALLOC);
    unsigned char png_header [8];

    simple_PNG_p png_file_format;
    data_IHDR_p pointer_to_IHDR_data;
    chunk_p pointer_to_IHDR_chunk;
    chunk_p pointer_to_IDAT_data;
    chunk_p pointer_to_IEND_data;

    png_file_format = (simple_PNG_p)malloc(sizeof(simple_PNG_p));
    pointer_to_IHDR_data = (data_IHDR_p)malloc(13);
    pointer_to_IHDR_chunk = (chunk_p)malloc(sizeof(struct chunk));
    pointer_to_IDAT_data = (chunk_p)malloc(sizeof(struct chunk));
    pointer_to_IEND_data = (chunk_p)malloc(sizeof(struct chunk));

    memcpy(png_header, png_buf.buf, sizeof(png_header));
    memcpy(pointer_to_IHDR_data, png_buf.buf + 16, sizeof(*pointer_to_IHDR_data));
    realpng = is_png(png_header, sizeof(png_header));

    if (realpng != 1) {
        is_corrupt = -1;
    } else {
        get_png_chunks_buf(pointer_to_IHDR_chunk, png_buf.buf, 8);
		memcpy(tmp, png_buf.buf + 12, ntohl(pointer_to_IHDR_chunk->length) + 4);
        is_corrupt = compare_crcs(tmp, pointer_to_IHDR_chunk->crc, ntohl(pointer_to_IHDR_chunk->length) + 4, "IHDR");
		
		if (is_corrupt == 0) {
            get_png_chunks_buf(pointer_to_IDAT_data, png_buf.buf, 33);
			memcpy(tmp, png_buf.buf + 37, ntohl(pointer_to_IDAT_data->length) + 4);
            is_corrupt = compare_crcs(tmp, pointer_to_IDAT_data->crc, ntohl(pointer_to_IDAT_data->length) + 4, "IDAT");
        }

        if (is_corrupt == 0) {
            get_png_chunks_buf(pointer_to_IEND_data, png_buf.buf, ntohl(pointer_to_IDAT_data->length) + 45);
			memcpy(tmp, png_buf.buf + ntohl(pointer_to_IDAT_data->length) + 49, ntohl(pointer_to_IEND_data->length) + 4);
            is_corrupt = compare_crcs(tmp, pointer_to_IEND_data->crc, ntohl(pointer_to_IEND_data->length) + 4, "IEND");
        }
    }

    free(tmp);
    free(png_file_format);
    free(pointer_to_IHDR_chunk->p_data);
    free(pointer_to_IDAT_data->p_data);
    free(pointer_to_IEND_data->p_data);
    free(pointer_to_IHDR_data);
    free(pointer_to_IHDR_chunk);
    free(pointer_to_IDAT_data);
    free(pointer_to_IEND_data);
    return is_corrupt;
}
