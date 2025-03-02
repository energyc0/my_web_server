#include "socklib.h"
#include "web_util.h"
#include "child_processor.h"
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

#define LISTENERS_COUNT 10

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

    char ip_buf[INET_ADDRSTRLEN];
    int socket_fd;
    if((socket_fd = setup_socket_for_server(argv[1], LISTENERS_COUNT, ip_buf)) == -1)
        return 1;

    printf("Server opened at address http://%s:%s\n", ip_buf, argv[1]);
    print_log("Server opened: ");
    time_t t = time(NULL);
    print_log(ctime(&t));

    while(1){
        int client_fd;
        struct sockaddr_in claddr;
        socklen_t adlen = sizeof(claddr);
        if((client_fd = accept(socket_fd, (struct sockaddr*)&claddr, &adlen)) == -1){
            exit(1);
        }
           // OOPS("accept()");

        char buf[BUFSIZ];
        FILE* client_fp;
        if((client_fp = fdopen(client_fd, "r")) == NULL)
            OOPS("fdopen()");
        if(fgets(buf, sizeof(buf), client_fp) == NULL){
            printf("%s - err\n", buf);
            if(!feof(client_fp))
                perror("fgets()");
            fclose(client_fp);
            continue;
        }
        read_until_crnl(client_fp);
        buf[strcspn(buf, "\r\n")] = '\0';
        process_request(buf,inet_ntoa(claddr.sin_addr), client_fp);
        fclose(client_fp);
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