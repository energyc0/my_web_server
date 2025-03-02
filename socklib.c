#include "socklib.h"
#include <string.h>
#include <netinet/in.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>

static struct addrinfo* get_host_info(struct addrinfo** res, const char* serv_host, const char* serv_port);

int setup_socket_for_server(const char* serv_port, int listeners, char* info_buf){
    int socket_fd;
    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket()");
        return -1;
    }

    int opt = 1;
    if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        perror("setsockopt()");
        return -1;
    }
    
    struct addrinfo* p, *res;
    if((p = get_host_info(&res, NULL, serv_port)) == NULL)
        return -1; 

    if(bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1){
        perror("bind()");
        return -1;
    }

    if(listen(socket_fd, listeners) == -1){
        perror("listen()");
        return -1;
    }

    if(info_buf != NULL){
        strcpy(info_buf, inet_ntoa(((struct sockaddr_in*)p->ai_addr)->sin_addr));
    }
    freeaddrinfo(res);
    return socket_fd;
}
int setup_socket_for_client(const char* serv_host, const char* serv_port){
    int socket_fd;
    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket()");
        return -1;
    }
    struct addrinfo *p, *res;

    if((p = get_host_info(&res, serv_host, serv_host)) == NULL)
        return -1;


    if(connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1){
        perror("connect()");
        return -1;
    }
    freeaddrinfo(res);
    return 0;
}

static struct addrinfo* get_host_info(struct addrinfo** res, const char* serv_host, const char* serv_port){
    struct addrinfo req;
    memset(&req, 0, sizeof(req));
    req.ai_socktype = SOCK_STREAM;
    req.ai_family = PF_INET;

    if(getaddrinfo(NULL, serv_port, &req, res) == -1){
        perror("getaddrinfo()");
        return NULL;
    }
    
    struct addrinfo* p;
    for(p = *res; p != NULL; p = p->ai_next){
        if(p->ai_family == PF_INET && p->ai_socktype == SOCK_STREAM)
            break;
    }
    if(p == NULL){
        fprintf(stderr, "failed to find sockaddr_in struct\n");
        return NULL;
    }
    return p;
}