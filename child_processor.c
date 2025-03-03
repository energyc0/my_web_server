#include "child_processor.h"
#include "web_util.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define NOT_FOUND_REQUEST_404 "HTTP/1.0 404 Not found"
#define BAD_REQUEST_400 "HTTP/1.0 400 Bad request"
#define OK_REQUEST_200 "HTTP/1.0 200 OK"

struct log_info{
    char* client_ip;
    char* query;
};

static void set_env_for_cgi(char* client_ip);

//HTTP answers
static void invalid_request(const struct log_info* info);
static void additional_info(char* content_type);
static void cat_file(char* content,const struct log_info* info);
static void ls_dir(char* content, const struct log_info* info);
static void exec_cgi(char* content, const struct log_info* info);
static void undefined_file(char* content, const struct log_info* info);

void process_request(struct query_info* q_info){
    if(fork() != 0)
        return;

    int fd = fileno(q_info->client_fp);
    dup2(fd, 1);
    fclose(q_info->client_fp);
    
    struct log_info info;
    info.client_ip = inet_ntoa(q_info->client_addr.sin_addr);
    info.query = q_info->query;
    
    set_env_for_cgi(info.client_ip);
    
    char method[11], filename[512], http_ver[256];
    if(sscanf(q_info->query, "%10s %511s %255s", method,filename,http_ver) != 3 || strcmp("GET", method) != 0){
        invalid_request(&info);
    }
    struct stat file_info;
    if(stat(filename, &file_info) == -1){
        undefined_file(filename,&info);
    }else if(S_ISDIR(file_info.st_mode)){
        ls_dir(filename,&info);
    }else if(S_ISREG(file_info.st_mode)){
        if(is_cgi_file(filename))
            exec_cgi(filename,&info);
        else
            cat_file(filename,&info);
    }else{
        undefined_file(filename,&info);
    }
}

static void invalid_request(const struct log_info* info){
    printf("%s\n", BAD_REQUEST_400);
    print_log("%s requested \"%s\" with result %s\n",info->client_ip, info->query, BAD_REQUEST_400);

    additional_info("text/html");
    exit(EXIT_FAILURE);
}

static void cat_file(char* content,const struct log_info* info){
    printf("%s\n", OK_REQUEST_200);
    print_log("%s requested \"%s\" with result %s\n",info->client_ip, info->query, OK_REQUEST_200);

    char* file_ext = get_file_extension(content);
    additional_info(get_content_type(file_ext));
    execlp("cat", "cat", content, NULL);
    exit(EXIT_FAILURE);
}
static void ls_dir(char* content, const struct log_info* info){
    printf("%s\n", OK_REQUEST_200);
    print_log("%s requested \"%s\" with result %s\n",info->client_ip, info->query, OK_REQUEST_200);

    additional_info(content);
    execlp("ls", "ls", "-l", content, NULL);
    exit(EXIT_FAILURE);
}
static void exec_cgi(char* content, const struct log_info* info){
    printf("%s\n", OK_REQUEST_200);
    print_log("%s requested \"%s\" with result %s\n",info->client_ip, info->query, OK_REQUEST_200);

    execlp(content, content, NULL);
    exit(EXIT_FAILURE);
}
static void undefined_file(char* content, const struct log_info* info){
    printf("%s\n", NOT_FOUND_REQUEST_404);
    print_log("%s requested \"%s\" with result %s\n",info->client_ip, info->query, NOT_FOUND_REQUEST_404);

    additional_info("text/html");
    exit(EXIT_FAILURE);
}


static void set_env_for_cgi(char* client_ip){
    setenv("REMOTE_ADDR", client_ip, 1);
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
