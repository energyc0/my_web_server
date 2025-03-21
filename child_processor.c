#include "child_processor.h"
#include "web_util.h"
#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
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

struct request_thread_create_info{
    struct query_info* q_info;
    pthread_barrier_t barrier;
};

//must call destroy_query()
static void copy_query(struct query_info* dst, const struct query_info* src);
//must be called only after copy_query()
static void destroy_query(struct query_info* q);

static void set_env_for_cgi(const struct query_info* info);

static enum method_t get_method(char* s);

//HTTP answers
static void invalid_request(const struct query_info* info);
static void print_header(char* result, char* content_type, unsigned int content_len, FILE* fp);

static void head_result(const struct query_info* info);
static void post_result(const struct query_info* info);
static void cat_file(const struct query_info* info);
static void ls_dir(const struct query_info* info);
static void exec_cgi(const struct query_info* info);
static void undefined_file(const struct query_info* info);

static void process_request_thread(struct request_thread_create_info* q_info);

void process_request(struct query_info* q_info){
    pthread_t thr;
    pthread_attr_t attr;
    struct request_thread_create_info create_info;
    create_info.q_info = q_info;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_barrier_init(&create_info.barrier, NULL, 2);

    pthread_create(&thr, &attr, (void*)process_request_thread, &create_info);
    pthread_barrier_wait(&create_info.barrier);
}

static void invalid_request(const struct query_info* info){
    print_header(BAD_REQUEST_400, "text/html", 0, info->client_fp);
    print_log("%s requested \"%s\" with result %s\n",inet_ntoa(info->client_addr.sin_addr), info->query, BAD_REQUEST_400);
}

static void cat_file(const struct query_info* info){
    char* file_ext = get_file_extension(info->http_filename);
    print_header(OK_REQUEST_200,get_content_type(file_ext), info->file_info->st_size, info->client_fp);

    print_log("%s requested \"%s\" with result %s\n",inet_ntoa(info->client_addr.sin_addr), info->query, OK_REQUEST_200);

    char command[512];
    sprintf(command, "cat %s", info->http_filename);
    FILE* fp = popen(command, "r");
    if(fp != NULL){
        copy_stream(info->client_fp,fp);
    }else{
        undefined_file(info);
    }
}
static void ls_dir(const struct query_info* info){
    print_header(OK_REQUEST_200, "text/plain",0, info->client_fp);
    print_log("%s requested \"%s\" with result %s\n",inet_ntoa(info->client_addr.sin_addr), info->query, OK_REQUEST_200);

    char command[512];
    sprintf(command, "ls -l %s", info->http_filename);
    FILE* fp = popen(command, "r");
    if(fp != NULL){
        copy_stream(info->client_fp, fp);
    }else{
        undefined_file(info);
    }
}
static void exec_cgi(const struct query_info* info){
    print_header(OK_REQUEST_200, NULL,0, info->client_fp);
    print_log("%s requested \"%s\" with result %s\n",inet_ntoa(info->client_addr.sin_addr), info->query, OK_REQUEST_200);

    set_env_for_cgi(info);

    FILE* fp = popen(info->http_filename, "r");
    if(fp != NULL){
        copy_stream(info->client_fp,fp);
    }else{
        undefined_file(info);
    }
}

static void undefined_file(const struct query_info* info){
    print_header(NOT_FOUND_REQUEST_404, "text/html",0, info->client_fp);
    print_log("%s requested \"%s\" with result %s\n", inet_ntoa(info->client_addr.sin_addr), info->query, NOT_FOUND_REQUEST_404);
}

static void set_env_for_cgi(const struct query_info* info){
    char* client_ip = inet_ntoa(info->client_addr.sin_addr);

    setenv("REMOTE_ADDR", client_ip, 1);
    setenv("SERVER_NAME", "localhost", 1);
    setenv("SERVER_PORT", info->server_port, 1);
    setenv("SCRIPT_NAME", info->http_filename, 1);
}

static void print_header(char* result, char* content_type, unsigned int content_len, FILE* fp){
    char hostname[256];
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&mutex);
    if(result) fprintf(fp,"%s\n", result);
    if(content_type){
        if(hostname[0] == '\0')
            gethostname(hostname, sizeof(hostname));
        time_t t = time(NULL);
        fprintf(fp,"Date: %s", ctime(&t));
        fprintf(fp,"Content type: %s\n", content_type);
        if(content_len != 0) fprintf(fp,"Content length: %u\n", content_len);
        fprintf(fp,"Connection: closed\n");
        fprintf(fp,"Server: %s\n\n", hostname);
    }
    pthread_mutex_unlock(&mutex);
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
    print_header(NULL, get_content_type(get_file_extension(info->http_filename)), info->file_info->st_size, info->client_fp);
    print_log("%s requested \"%s\"\n", inet_ntoa(info->client_addr.sin_addr), info->query);
}

//#TODO multithreading
static void post_result(const struct query_info* info){
    int fd;
    if((info->file_info && !S_ISREG(info->file_info->st_mode)) ||
     (fd = open(info->http_filename, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1 || 
     write(fd, info->add_info, strlen(info->add_info)) == -1){
        print_header(NOT_FOUND_REQUEST_404, "text/plain", 0, info->client_fp);
    }

    print_header(OK_REQUEST_200, "text/plain", strlen(info->add_info), info->client_fp);
    print_log("Received POST data from %s:\n%s\n", inet_ntoa(info->client_addr.sin_addr), info->add_info);

}

static void process_request_thread(struct request_thread_create_info* info){
    struct query_info q_info;
    copy_query(&q_info, info->q_info);
    pthread_barrier_wait(&info->barrier);

    char method[11], filename[512], http_ver[256];
    if(sscanf(q_info.query, "%10s %511s %255s", method,filename,http_ver) != 3 || (q_info.http_method=get_method(method)) == M_NONE ){
        invalid_request(&q_info);
        destroy_query(&q_info);
        return;
    }

    q_info.http_filename = filename;
    q_info.http_ver = http_ver;
    sanitize_string(q_info.http_filename);

    //#TODO multithreading
    //set_env_for_cgi(&q_info);

    struct stat file_info;
    q_info.file_info = &file_info;
    if(stat(filename, &file_info) == -1){
        if(q_info.http_method != M_POST){
            undefined_file(&q_info);
            destroy_query(&q_info);
            return;
        }
        q_info.file_info = NULL;
    }
    if(q_info.http_method == M_POST){
        post_result(&q_info);
    }else if(q_info.http_method == M_HEAD){
        if(S_ISREG(file_info.st_mode))
            head_result(&q_info);
        else
            invalid_request(&q_info);
    }else if(q_info.http_method == M_POST)
        post_result(&q_info);
    else if(S_ISDIR(file_info.st_mode)){
        ls_dir(&q_info);
    }else if(S_ISREG(file_info.st_mode)){
        if(is_cgi_file(filename))
            exec_cgi(&q_info);
        else
            cat_file(&q_info);
    }else{
        undefined_file(&q_info);
    }

    destroy_query(&q_info);
}

//must call destroy_query()
static void copy_query(struct query_info* dst, const struct query_info* src){
    dst->server_ip = strdup(src->server_ip);
    dst->server_port = strdup(src->server_port);

    dst->query = strdup(src->query);
    dst->add_info = strdup(src->add_info);
    dst->client_addr = src->client_addr;
    dst->addr_len = src->addr_len;
    dst->client_fp = src->client_fp;
}
//must be called only after copy_query()
static void destroy_query(struct query_info* q){
    free(q->server_ip);
    free(q->server_port);
    free(q->query);
    free(q->add_info);

    fclose(q->client_fp);
}