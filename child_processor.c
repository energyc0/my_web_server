#include "child_processor.h"
#include "web_util.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

#define NOT_FOUND_REQUEST_404 "HTTP/1.0 404 Not found"
#define BAD_REQUEST_400 "HTTP/1.0 400 Bad request"
#define OK_REQUEST_200 "HTTP/1.0 200 OK"

static void set_env_for_cgi(struct query_info* info);

static enum method_t get_method(char* s);

//HTTP answers
static void invalid_request(const struct query_info* info);
static void print_header(char* result, char* content_type, unsigned int content_len);

static void head_result(const struct query_info* info);
static void post_result(const struct query_info* info);
static void cat_file(const struct query_info* info);
static void ls_dir(const struct query_info* info);
static void exec_cgi(const struct query_info* info);
static void undefined_file(const struct query_info* info);

void process_request(struct query_info* q_info){
    if(fork() != 0)
        return;

    int fd = fileno(q_info->client_fp);
    dup2(fd, 1);
    fclose(q_info->client_fp);
    
    char method[11], filename[512], http_ver[256];
    if(sscanf(q_info->query, "%10s %511s %255s", method,filename,http_ver) != 3 || (q_info->http_method=get_method(method)) == M_NONE ){
        invalid_request(q_info);
    }

    q_info->http_filename = filename;
    q_info->http_ver = http_ver;

    set_env_for_cgi(q_info);

    struct stat file_info;
    q_info->file_info = &file_info;
    if(stat(filename, &file_info) == -1){
        if(q_info->http_method != M_POST)
            undefined_file(q_info);
        q_info->file_info = NULL;
    }
    if(q_info->http_method == M_POST){
        post_result(q_info);
    }
    if(q_info->http_method == M_HEAD){
        if(S_ISREG(file_info.st_mode))
            head_result(q_info);
        else
            invalid_request(q_info);
    }
    if(q_info->http_method == M_POST)
        post_result(q_info);
    else if(S_ISDIR(file_info.st_mode)){
        ls_dir(q_info);
    }else if(S_ISREG(file_info.st_mode)){
        if(is_cgi_file(filename))
            exec_cgi(q_info);
        else
            cat_file(q_info);
    }else{
        undefined_file(q_info);
    }
}

static void invalid_request(const struct query_info* info){
    print_header(BAD_REQUEST_400, "text/html", 0);
    print_log("%s requested \"%s\" with result %s\n",inet_ntoa(info->client_addr.sin_addr), info->query, BAD_REQUEST_400);
    exit(EXIT_FAILURE);
}

static void cat_file(const struct query_info* info){
    char* file_ext = get_file_extension(info->http_filename);
    print_header(OK_REQUEST_200,get_content_type(file_ext), info->file_info->st_size);

    print_log("%s requested \"%s\" with result %s\n",inet_ntoa(info->client_addr.sin_addr), info->query, OK_REQUEST_200);

    execlp("cat", "cat", info->http_filename, NULL);
    exit(EXIT_FAILURE);
}
static void ls_dir(const struct query_info* info){
    print_header(OK_REQUEST_200, "text/plain",0);
    print_log("%s requested \"%s\" with result %s\n",inet_ntoa(info->client_addr.sin_addr), info->query, OK_REQUEST_200);

    execlp("ls", "ls", "-l", info->http_filename, NULL);
    exit(EXIT_FAILURE);
}
static void exec_cgi(const struct query_info* info){
    print_header(OK_REQUEST_200, NULL,0);
    print_log("%s requested \"%s\" with result %s\n",inet_ntoa(info->client_addr.sin_addr), info->query, OK_REQUEST_200);

    execlp(info->http_filename, info->http_filename, NULL);
    exit(EXIT_FAILURE);
}

static void undefined_file(const struct query_info* info){
    print_header(NOT_FOUND_REQUEST_404, "text/html",0);
    print_log("%s requested \"%s\" with result %s\n", inet_ntoa(info->client_addr.sin_addr), info->query, NOT_FOUND_REQUEST_404);

    exit(EXIT_FAILURE);
}

static void set_env_for_cgi(struct query_info* info){
    char* client_ip = inet_ntoa(info->client_addr.sin_addr);

    setenv("REMOTE_ADDR", client_ip, 1);
    setenv("SERVER_NAME", "localhost", 1);
    setenv("SERVER_PORT", info->server_port, 1);
    setenv("SCRIPT_NAME", info->http_filename, 1);
    //setenv("REMOTE_ADDR", clien_ip, 1);
    //setenv("REMOTE_ADDR", clien_ip, 1);
}

static void print_header(char* result, char* content_type, unsigned int content_len){
    static char hostname[256];
    if(result) printf("%s\n", result);
    if(content_type){
        if(hostname[0] == '\0')
            gethostname(hostname, sizeof(hostname));
        time_t t = time(NULL);
        printf("Date: %s", ctime(&t));
        printf("Content type: %s\n", content_type);
        if(content_len != 0) printf("Content length: %u\n", content_len);
        printf("Connection: closed\n");
        printf("Server: %s\n\n", hostname);
    }
}

static enum method_t get_method(char* s){
    if(strcmp("GET", s) == 0)
        return M_GET;
    else if(strcmp("HEAD", s) == 0)
        return M_HEAD;
    else if(strcmp("POST", s) == 0)
        return M_POST;
    return M_NONE;
}

static void head_result(const struct query_info* info){
    print_header(NULL, get_content_type(get_file_extension(info->http_filename)), info->file_info->st_size);
    print_log("%s requested \"%s\"\n", inet_ntoa(info->client_addr.sin_addr), info->query);
    exit(EXIT_SUCCESS);
}

static void post_result(const struct query_info* info){
    int fd;
    if((info->file_info && !S_ISREG(info->file_info->st_mode)) ||
     (fd = open(info->http_filename, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1 || 
     write(fd, info->add_info, strlen(info->add_info)) == -1){
        print_header(NOT_FOUND_REQUEST_404, "text/plain", 0);
        exit(EXIT_FAILURE);
    }

    print_header(OK_REQUEST_200, "text/plain", strlen(info->add_info));
    //printf("Received POST data\n");
    print_log("Received POST data from %s:\n%s\n", inet_ntoa(info->client_addr.sin_addr), info->add_info);

    exit(EXIT_SUCCESS);
}