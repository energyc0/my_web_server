#ifndef CHILD_PROCESSOR_H
#define CHILD_PROCESSOR_H

#include <stdio.h>
#include <netinet/in.h>

struct query_info{
    //char ip_buf[INET_ADDRSTRLEN];
    char* query;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    FILE* client_fp;
};

void process_request(struct query_info* q_info);

#endif