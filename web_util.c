#include "web_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_LOG_FILE "my_web_server.log"

void print_log(char* msg){
    static FILE* log_fp = NULL;
    if(log_fp == NULL){
        log_fp = fopen(SERVER_LOG_FILE, "a");
        if(log_fp == NULL)
            OOPS("fopen");
    }
    fprintf(log_fp, "%s", msg);
    fflush(log_fp);
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
    return strcmp("cgi", get_file_extension(filename)) == 0;
}

char* get_content_type(char* file_extension){
    static char buf[BUFSIZ];
    if(strcmp("png", file_extension) == 0 || strcmp("jpg", file_extension) == 0 ||
     strcmp("jpeg", file_extension) == 0 || strcmp("gif", file_extension) == 0){
        sprintf(buf, "image/%s", file_extension);
    }else if(strcmp("html", file_extension) == 0 || strcmp("xml", file_extension) == 0){
        sprintf(buf, "text/%s", file_extension);
    }else if(strcmp("MP4", file_extension) == 0 || strcmp("mp4", file_extension)){
        sprintf(buf, "video/%s", file_extension);
    }else{
        sprintf(buf, "text/plain");
    }
    return buf;
}
