#ifndef SOCKLIB_H
#define SOCKLIB_H

int setup_socket_for_server(const char* serv_port, int listeners, char* ip_buf);
int setup_socket_for_client(const char* serv_host, const char* serv_port);

#endif