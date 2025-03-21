#include "socklib.h"
#include "web_util.h"
#include "child_processor.h"
#include <asm-generic/ioctls.h>
#include <signal.h>
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
#include <sys/ioctl.h>

#define LISTENERS_COUNT 1

static void sigchild_handler(int code);
static void sigint_handler(int code);

int main(int argc, char** argv){
    if(argc != 2){
        fprintf(stderr, "Server port expected\n");
        return 1;
    }
    
    struct sigaction sgnl;
    memset(&sgnl, 0, sizeof(sgnl));
    sgnl.sa_handler = sigchild_handler;
    sgnl.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sgnl, NULL) == -1)
        OOPS("sigaction()");

    sgnl.sa_handler = sigint_handler;
    if(sigaction(SIGINT, &sgnl, NULL) == -1)
        OOPS("sigaction()");

    char* ip_buf;
    int socket_fd;
    if((socket_fd = setup_socket_for_server(argv[1], LISTENERS_COUNT, &ip_buf)) == -1)
        return 1;

    time_t t = time(NULL);
    printf("Server opened at address http://%s:%s\n", ip_buf, argv[1]);
    print_log("Server opened: %s", ctime(&t));

    while(1){
        struct query_info q_info;
        memset(&q_info,0,sizeof(q_info));
        q_info.server_port = argv[1];
        q_info.addr_len = sizeof(q_info.client_addr);
        q_info.server_ip = ip_buf;
        int client_fd;

        if((client_fd = accept(socket_fd, (struct sockaddr*)&q_info.client_addr, &q_info.addr_len)) == -1){
            exit(1);
        }

        char buf[BUFSIZ];
        if((q_info.client_fp = fdopen(client_fd, "a+")) == NULL)
            OOPS("fdopen()");
        if(fgets(buf, sizeof(buf), q_info.client_fp) == NULL){
            printf("%s - err\n", buf);
            if(!feof(q_info.client_fp))
                perror("fgets()");
            fclose(q_info.client_fp);
            continue;
        }

        read_until_crnl(q_info.client_fp);
        buf[strcspn(buf, "\r\n")] = '\0';

        char add_info[BUFSIZ];
        int bytes_avail;
        ioctl(0, FIONREAD, &bytes_avail);
        if(bytes_avail > 0){
            if(fgets(add_info, sizeof(add_info), q_info.client_fp) == NULL){
                perror("fgets()");
                fclose(q_info.client_fp);
                continue;
            }
            add_info[strcspn(buf, "\r\n")] = '\0';
        }else{
            add_info[0] = '\0';
        }

        q_info.add_info = add_info;
        q_info.query = buf;
        process_request(&q_info);
    }
    return 0;
}
static void sigchild_handler(int code){
    while (waitpid(-1, NULL, WNOHANG) > 0){
        printf("Process is done\n");
    }
}

static void sigint_handler(int code){
    exit(EXIT_SUCCESS);
}