#ifndef WEB_UTIL_H
#define WEB_UTIL_H

#include <stdio.h>

#define OOPS(msg){perror(msg); exit(EXIT_FAILURE);}

//print formatted log and flush to file
void print_log(const char* fmt, ...);
//skip until EOF or "\r\n" line
void read_until_crnl(FILE* fp);
//return char* pointer on the same string
char* get_file_extension(char* filename);
//return if file extension is .cgi
int is_cgi_file(char* filename);
//get content type for HTTP answer
char* get_content_type(char* file_extension);
//erase all ';' characters
void sanitize_string(char* s);
//copy data from src_fp to dst_fp
void copy_stream(FILE* dst_fp, FILE* src_fp);
#endif