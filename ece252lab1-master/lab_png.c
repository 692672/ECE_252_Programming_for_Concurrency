#pragma once
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "lab_png.h"
#include "crc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    fread(out, 1, sizeof(out), fp);
    return 0;
}

int get_png_chunks(struct chunk * out, FILE *fp, long offset, int whence) {
    fseek(fp, offset, whence);
    fread(&out->length, 1, 4, fp);
    fread(out->type, 1, 4, fp);
    out->p_data = (unsigned char *)malloc(((int)ntohl(out->length)) * sizeof(U8));
    fread(out->p_data, 1, ntohl(out->length), fp);
    fread(&out->crc, 1, sizeof(out->crc), fp);

    return 0;
}

int compare_crcs(char * crc_to_calculate, U32 read_crc, unsigned long len, char * chunk_name) {
    unsigned long calculated_crc = 0;

    calculated_crc = crc(crc_to_calculate, len);

    if (calculated_crc != ntohl(read_crc)) {
        printf("%s chunk CRC error: computed %lx, expected: %lx\n", chunk_name, calculated_crc, (unsigned long)ntohl(read_crc));
        return -1;
    }
    return 0;
}
