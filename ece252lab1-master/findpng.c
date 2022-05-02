#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h> /* for strcat().  man strcat   */

const unsigned long MAX_MALLOC = 16000000;

char* str;
struct stat buf;

int count = 0;

int is_png2(unsigned char * buf, size_t n) {
    for (int i = 0; i < n; i++) {
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

int is_png_helper(char* file) {
    FILE* fp = fopen(file, "r");
    unsigned char header[8];
    fread(&header, 1, 8, fp);
	fclose(fp);
    return is_png2(header, 8);
}


void helper(char* pref) {
	DIR* p_dir = opendir(pref);
	if (p_dir) {
		struct dirent* p_dirent;
        strcpy(str, pref);
        int pref_len = strlen(pref);

        char* banned1 = ".";
        char* banned2 = "..";

        while(p_dirent = readdir(p_dir)) {
            strcpy(&str[pref_len], p_dirent->d_name);
            if (lstat(str, &buf) == 0) {
                if (S_ISREG(buf.st_mode)) {
                    if (is_png_helper(str)) {
                        printf("%s\n", str);
                        count++;
                    }
                }
                else if (S_ISDIR(buf.st_mode)) {
                    if (strcmp(p_dirent->d_name, banned1) && strcmp(p_dirent->d_name, banned2)) {
                        char* tmp = "/";
                        strcat(str, tmp);
                        helper(str);
                    }
                }
            }
        }
        closedir(p_dir);
	}
	return;
}

int main(int argc, char** argv) {
	if (argc == 2) {
		str = (char*)malloc(MAX_MALLOC);
		strcpy(str, argv[1]);
		int pref_len = strlen(argv[1]);
		int ends_with_slash = 0;
		for (int i = 0; i < pref_len; i++) {
			if (strcmp(&str[i], "/") == 0) {
				ends_with_slash = 1;
			}
		}
		if (!ends_with_slash) {
			strcpy(&str[pref_len], "/");
		}
		helper(str);
		free(str);
		if (count == 0) {
			printf("findpng: No PNG file found\n");
		}
	}
	return 0;
}
