#include "../include/global.h"
#include "../include/logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 65535

int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int get_own_ip(char *ip_buf) {
    int sock;
    struct sockaddr_in remote_addr;
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &remote_addr.sin_addr);  // Google DNS

    if (connect(sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
        close(sock);
        return -1;
    }

    if (getsockname(sock, (struct sockaddr *)&local_addr, &addr_len) < 0) {
        close(sock);
        return -1;
    }

    if (inet_ntop(AF_INET, &local_addr.sin_addr, ip_buf, INET_ADDRSTRLEN) == NULL) {
        close(sock);
        return -1;
    }

    close(sock);
    return 0;
}

void print_port_success(int port) {
    cse4589_print_and_log("[PORT:SUCCESS]\n");
    cse4589_print_and_log("PORT:%d\n", port);
    cse4589_print_and_log("[PORT:END]\n");
}

void print_port_error() {
    cse4589_print_and_log("[PORT:ERROR]\n");
    cse4589_print_and_log("[PORT:END]\n");
}

void refresh_client_list(int client_socket_fd, const std::string& client_ip, int port_number) {
    char hostname[NI_MAXHOST];
    gethostname(hostname, sizeof(hostname));

    std::stringstream refresh_msg;
    refresh_msg << "LOGIN " << hostname << " " << client_ip << " " << port_number;

    if (send(client_socket_fd, refresh_msg.str().c_str(), refresh_msg.str().length(), 0) < 0) {
        cse4589_print_and_log("[REFRESH:ERROR]\n[REFRESH:END]\n");
        return;
    }

    char buffer[MAX_INPUT_SIZE];
    int bytes_received = recv(client_socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        cse4589_print_and_log("[REFRESH:ERROR]\n[REFRESH:END]\n");
        return;
    }

    buffer[bytes_received] = '\0';

    std::string response(buffer);
    std::istringstream iss(response);
    std::string line;
    client_count = 0;

    while (std::getline(iss, line)) {
        if (line.find("CLIENT") == 0) {
            std::istringstream line_ss(line);
            std::string prefix, hostname, ip;
            int port;

            line_ss >> prefix >> hostname >> ip >> port;

            if (client_count < MAX_CLIENTS) {
                strcpy(client_list[client_count].hostname, hostname.c_str());
                strcpy(client_list[client_count].ip, ip.c_str());
                client_list[client_count].port = port;
                client_count++;
            }
        }
    }

    cse4589_print_and_log("[REFRESH:SUCCESS]\n[REFRESH:END]\n");
}