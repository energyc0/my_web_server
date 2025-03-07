#ifndef CHILD_PROCESSOR_H
#define CHILD_PROCESSOR_H

#include <stdio.h>
#include <netinet/in.h>

enum method_t{
    M_NONE = 0,
    M_GET,
    M_HEAD
};

struct query_info{
    char* server_ip;
    char* server_port;

    enum method_t http_method;
    char* http_filename;
    char* http_ver;

    char* query;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    FILE* client_fp;

    const struct stat* file_info;
};

void process_request(struct query_info* q_info);

#endif