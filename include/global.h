#ifndef GLOBAL_H_
#define GLOBAL_H_

#define HOSTNAME_LEN 128
#define PATH_LEN 256
#define MAX_CLIENTS 100

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <math.h>
#include <algorithm> 
#include <sstream>
#include <string>
#include <map>

typedef struct {
    int port;
    char hostname[256];
    char ip[INET_ADDRSTRLEN];
} ClientInfo;

extern ClientInfo client_list[MAX_CLIENTS];
extern int client_count;

extern bool client_logged_in;
void login_to_server(const std::string& server_ip, int server_port, int client_port);

void print_client_list(ClientInfo* clients, int count);

int my_strcmp(const char *s1, const char *s2);
int get_own_ip(char *ip_buf);

void print_port_success(int port);
void print_port_error();

typedef struct {
    std::string hostname;
    std::string ip;
    int port;
    std::string status;
    int num_msg_sent;
    int num_msg_rcv;
    std::vector<std::string> blocked_ips;
    std::vector<std::string> buffered_messages;
    int fd;
} SocketEntry;

extern std::vector<SocketEntry> connected_clients;

void print_list_command();

void refresh_client_list(int client_socket_fd, const std::string& client_ip, int port_number);

#endif
