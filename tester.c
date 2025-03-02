#include <stdio.h>
#include "socklib.h"

#define SERVER_PORT "13000"

int main(){
    char buf[BUFSIZ];
    int sock_fd = setup_socket_for_server(SERVER_PORT, 1, buf);
    if(sock_fd == -1)
        return 1;

    printf("%s\n", buf);
    return 0;
}