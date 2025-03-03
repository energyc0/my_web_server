#ifndef SOCKLIB_H
#define SOCKLIB_H

//setup server socket,
//return socket_fd on success, -1 on error
//initialize ip_buf if it is not NULL with internal buffer to get server ip
int setup_socket_for_server(const char* serv_port, int listeners, char** ip_buf);

//setup server socket,
//return socket_fd on success, -1 on error
int setup_socket_for_client(const char* serv_host, const char* serv_port);

#endif