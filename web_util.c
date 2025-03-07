#include "web_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define SERVER_LOG_FILE "my_web_server.log"

static int is_in_cgi_bin(char* filename);

void print_log(const char* fmt, ...){
    static FILE* log_fp = NULL;
    if(log_fp == NULL){
        log_fp = fopen(SERVER_LOG_FILE, "a");
        if(log_fp == NULL)
            OOPS("fopen");
    }

    va_list ap;
    va_start(ap, fmt);

    vfprintf(log_fp, fmt, ap);
    fflush(log_fp);

    va_end(ap);
}

void read_until_crnl(FILE* fp){
    char buf[BUFSIZ];
    while (fgets(buf, sizeof(buf), fp) != NULL && strcmp(buf, "\r\n") != 0);
}

char* get_file_extension(char* filename){
    filename = strchr(filename, '.');
    return filename == NULL ? "" : filename + 1;
}

int is_cgi_file(char* filename){
    return strcmp("cgi", get_file_extension(filename)) == 0 || is_in_cgi_bin(filename);
}

char* get_content_type(char* file_extension){
    static char buf[BUFSIZ];
    if(strcmp("png", file_extension) == 0 || strcmp("jpg", file_extension) == 0 ||
     strcmp("jpeg", file_extension) == 0 || strcmp("gif", file_extension) == 0){
        sprintf(buf, "image/%s", file_extension);
    }else if(strcmp("html", file_extension) == 0 || strcmp("xml", file_extension) == 0){
        sprintf(buf, "text/%s", file_extension);
    }else if(strcmp("MP4", file_extension) == 0 || strcmp("mp4", file_extension) == 0){
        sprintf(buf, "video/%s", file_extension);
    }else{
        sprintf(buf, "text/plain");
    }
    return buf;
}

static int is_in_cgi_bin(char* filename){
    char* prev = strchr(filename, '/');
    if(prev){
        for(char* next = strchr(prev+1, '/'); next != NULL; prev = next, next = strchr(prev+1, '/')){
            char temp = *next;
            *next = '\0';
            if(strcmp("/cgi-bin", prev) == 0){
                *next = temp;
                return 1;
            }

            *next = temp;
        }
    }
    return 0;
}