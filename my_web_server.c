#include "socklib.h"
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#define LISTENERS_COUNT 1
#define OOPS(msg){perror(msg); exit(EXIT_FAILURE);}

static void read_until_crnl(FILE* fp);

static void process_request(char* request, FILE* client_fp);

static char* get_file_extension(char* filename);
static int is_cgi_file(char* filename);
static char* get_content_type(char* file_extension);

//child process functions
static void invalid_request();
static void additional_info(char* content_type);
static void cat_file(char* content);
static void ls_dir(char* content);
static void exec_cgi(char* content);
static void undefined_file(char* content);

int main(int argc, char** argv){
    if(argc != 2){
        fprintf(stderr, "Server port expected\n");
        return 1;
    }

    char ip_buf[INET_ADDRSTRLEN];
    int socket_fd;
    if((socket_fd = setup_socket_for_server(argv[1], LISTENERS_COUNT, ip_buf)) == -1)
        return 1;

    printf("Server opened at address http://%s:%s\n", ip_buf, argv[1]);

    while(1){
        int client_fd;
        if((client_fd = accept(socket_fd, NULL, NULL)) == -1)
            OOPS("accept()");
        char buf[BUFSIZ];

        FILE* client_fp;
        if((client_fp = fdopen(client_fd, "r+")) == NULL)
            OOPS("fdopen()");
        if(fgets(buf, sizeof(buf), client_fp) == NULL){
            fprintf(stderr, "fgets()\n");
            fclose(client_fp);
            continue;
        }
        read_until_crnl(client_fp);
        process_request(buf, client_fp);
        wait(NULL);
        fclose(client_fp);
    }
    return 0;
}

static void read_until_crnl(FILE* fp){
    char buf[BUFSIZ];
    while (fgets(buf, sizeof(buf), fp) != NULL && strcmp(buf, "\r\n") != 0);
}

static void process_request(char* request, FILE* client_fp){
    if(fork() != 0)
        return;

    int fd = fileno(client_fp);
    dup2(fd, 1);
    fclose(client_fp);
    
    char method[11], filename[512], http_ver[256];
    if(sscanf(request, "%10s %511s %255s", method,filename,http_ver) != 3 || strcmp("GET", method) != 0){
        invalid_request();
    }
    struct stat file_info;
    if(stat(filename, &file_info) == -1){
        undefined_file(filename);
    }else if(S_ISDIR(file_info.st_mode)){
        ls_dir(filename);
    }else if(S_ISREG(file_info.st_mode)){
        if(is_cgi_file(filename))
            exec_cgi(filename);
        else
            cat_file(filename);
    }else{
        undefined_file(filename);
    }
}

static void invalid_request(){
    printf("HTTP/1.0 400 Bad request\n");
    additional_info("text/html");
    exit(EXIT_FAILURE);
}

static void additional_info(char* content_type){
    static char hostname[256];
    if(hostname[0] == '\0')
        gethostname(hostname, sizeof(hostname));
    time_t t = time(NULL);
    printf("Date: %s", ctime(&t));
    printf("Content type: %s\n", content_type);
    printf("Connection: closed\n");
    printf("Server: %s\n\n", hostname);
}

static void cat_file(char* content){
    printf("HTTP/1.0 200 OK\n");

    char* file_ext = get_file_extension(content);
    additional_info(get_content_type(file_ext));
    execlp("cat", "cat", content, NULL);
    exit(EXIT_FAILURE);
}
static void ls_dir(char* content){
    printf("HTTP/1.0 200 OK\n");
    additional_info(content);
    execlp("ls", "ls", "-l", content, NULL);
    exit(EXIT_FAILURE);
}
static void exec_cgi(char* content){
    printf("HTTP/1.0 200 OK\n");
    execlp(content, content, NULL);
    exit(EXIT_FAILURE);
}
static void undefined_file(char* content){
    printf("HTTP/1.0 404 Not found\n");
    additional_info("text/html");
    exit(EXIT_FAILURE);
}

static char* get_file_extension(char* filename){
    filename = strchr(filename, '.');
    return filename == NULL ? "" : filename + 1;
}

static int is_cgi_file(char* filename){
    return strcmp("cgi", get_file_extension(filename)) == 0;
}

static char* get_content_type(char* file_extension){
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