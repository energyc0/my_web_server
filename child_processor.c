#include "child_processor.h"
#include "web_util.h"
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static void set_env_for_cgi(char* client_ip);

//HTTP answers
static void invalid_request(char* log_buf);
static void additional_info(char* content_type);
static void cat_file(char* content, char* log_buf);
static void ls_dir(char* content, char* log_buf);
static void exec_cgi(char* content, char* log_buf);
static void undefined_file(char* content, char* log_buf);

void process_request(char* request, char* client_ip, FILE* client_fp){
    static char log_buf[BUFSIZ];
    if(fork() != 0)
        return;

    int fd = fileno(client_fp);
    dup2(fd, 1);
    fclose(client_fp);
    
    sprintf(log_buf, "%s requested \"%s\" with result ", client_ip, request);
    
    set_env_for_cgi(client_ip);
    
    char method[11], filename[512], http_ver[256];
    if(sscanf(request, "%10s %511s %255s", method,filename,http_ver) != 3 || strcmp("GET", method) != 0){
        invalid_request(log_buf);
    }
    struct stat file_info;
    if(stat(filename, &file_info) == -1){
        undefined_file(filename,log_buf);
    }else if(S_ISDIR(file_info.st_mode)){
        ls_dir(filename,log_buf);
    }else if(S_ISREG(file_info.st_mode)){
        if(is_cgi_file(filename))
            exec_cgi(filename,log_buf);
        else
            cat_file(filename,log_buf);
    }else{
        undefined_file(filename,log_buf);
    }
}

static void invalid_request(char* log_buf){
    printf("HTTP/1.0 400 Bad request\n");
    strcat(log_buf, "\"HTTP/1.0 400 Bad request\"");
    print_log(log_buf);
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

static void cat_file(char* content, char* log_buf){
    printf("HTTP/1.0 200 OK\n");
    strcat(log_buf, "\"HTTP/1.0 200 OK\"");
    print_log(log_buf);

    char* file_ext = get_file_extension(content);
    additional_info(get_content_type(file_ext));
    execlp("cat", "cat", content, NULL);
    exit(EXIT_FAILURE);
}
static void ls_dir(char* content, char* log_buf){
    printf("HTTP/1.0 200 OK\n");
    strcat(log_buf, "\"HTTP/1.0 200 OK\"");
    print_log(log_buf);

    additional_info(content);
    execlp("ls", "ls", "-l", content, NULL);
    exit(EXIT_FAILURE);
}
static void exec_cgi(char* content, char* log_buf){
    printf("HTTP/1.0 200 OK\n");
    strcat(log_buf, "\"HTTP/1.0 200 OK\"");
    print_log(log_buf);

    execlp(content, content, NULL);
    exit(EXIT_FAILURE);
}
static void undefined_file(char* content, char* log_buf){
    printf("HTTP/1.0 404 Not found\n");
    strcat(log_buf, "\"HTTP/1.0 404 Not found\"");
    print_log(log_buf);

    additional_info("text/html");
    exit(EXIT_FAILURE);
}


static void set_env_for_cgi(char* client_ip){
    setenv("REMOTE_ADDR", client_ip, 1);
}