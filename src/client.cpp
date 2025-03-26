#include "../include/global.h"
#include "../include/logger.h"
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define STDIN 0
#define MAX_INPUT_SIZE 65535

ClientInfo client_list[MAX_CLIENTS];
int client_count = 0;
bool is_logged_in = false;
int sockfd = -1;
int client_port = -1;
std::vector<std::string> blocked_ips;

void print_logged_clients() {
    for (int i = 0; i < client_count - 1; ++i) {
        for (int j = 0; j < client_count - i - 1; ++j) {
            if (client_list[j].port > client_list[j + 1].port) {
                ClientInfo temp = client_list[j];
                client_list[j] = client_list[j + 1];
                client_list[j + 1] = temp;
            }
        }
    }

    cse4589_print_and_log("[LIST:SUCCESS]\n");
    for (int i = 0; i < client_count; ++i) {
        cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",
                              i + 1,
                              client_list[i].hostname,
                              client_list[i].ip,
                              client_list[i].port);
    }
    cse4589_print_and_log("[LIST:END]\n");
}

void login_to_server(const std::string& server_ip, int server_port, int client_port) {
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cse4589_print_and_log("[LOGIN:ERROR]\n[LOGIN:END]\n");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        sockfd = -1;
        cse4589_print_and_log("[LOGIN:ERROR]\n[LOGIN:END]\n");
        return;
    }

    // Send login message
    char ip_address[INET_ADDRSTRLEN];
    if (get_own_ip(ip_address) != 0) {
        cse4589_print_and_log("[LOGIN:ERROR]\n[LOGIN:END]\n");
        close(sockfd);
        sockfd = -1;
        return;
    }

    char hostname[NI_MAXHOST];
    gethostname(hostname, sizeof(hostname));
    std::stringstream login_msg;
    login_msg << "LOGIN " << hostname << " " << ip_address << " " << client_port;

    if (send(sockfd, login_msg.str().c_str(), login_msg.str().length(), 0) < 0) {
        perror("send");
        cse4589_print_and_log("[LOGIN:ERROR]\n[LOGIN:END]\n");
        close(sockfd);
        sockfd = -1;
        return;
    }

    // Receive and process server response
    char buffer[MAX_INPUT_SIZE];
    int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        cse4589_print_and_log("[LOGIN:ERROR]\n[LOGIN:END]\n");
        close(sockfd);
        sockfd = -1;
        return;
    }

    buffer[bytes_received] = '\0';

    // TODO: Parse actual client list and buffered messages from server here
    // For now, assume server handles all related printing (RELAYEDs before LOGIN:SUCCESS)
    // Parse client list from response
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
    is_logged_in = true;
    cse4589_print_and_log("[LOGIN:SUCCESS]\n[LOGIN:END]\n");
}

void run_client(int port) {
    client_port = port;
    char input_buffer[MAX_INPUT_SIZE];
    fd_set master_set, read_fds;

    FD_ZERO(&master_set);
    FD_SET(STDIN_FILENO, &master_set);
    int fdmax = STDIN_FILENO;

    printf("[PA1-Client@CSE489/589]$ ");
    fflush(stdout);

    while (1) {
        read_fds = master_set;
        /*
        if (sockfd != -1) {
            FD_SET(sockfd, &read_fds);
            if (sockfd > fdmax)
                fdmax = sockfd;
        }
        */

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        // Handle stdin
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            printf("[PA1-Client@CSE489/589]$ ");
            fflush(stdout);

            if (fgets(input_buffer, MAX_INPUT_SIZE, stdin) == NULL)
                continue;

            input_buffer[strcspn(input_buffer, "\n")] = '\0';

            char* token = strtok(input_buffer, " ");
            if (!token) continue;

            std::string command = token;
            for (char& c : command)
                c = toupper(c);

        if (command == "AUTHOR") {
            cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
            cse4589_print_and_log("I, syedzuba, have read and understood the course academic integrity policy.\n");
            cse4589_print_and_log("[AUTHOR:END]\n");

        } else if (command == "IP") {
            char ip_address[INET_ADDRSTRLEN];
            if (get_own_ip(ip_address) == 0) {
                cse4589_print_and_log("[IP:SUCCESS]\n");
                cse4589_print_and_log("IP:%s\n", ip_address);
                cse4589_print_and_log("[IP:END]\n");
            } else {
                cse4589_print_and_log("[IP:ERROR]\n");
                cse4589_print_and_log("[IP:END]\n");
            }

        } else if (command == "PORT") {
            print_port_success(port);

        } else if (command == "LIST") {
            if (is_logged_in)
                print_logged_clients();
            else {
                cse4589_print_and_log("[LIST:ERROR]\n");
                cse4589_print_and_log("[LIST:END]\n");
            }

        } else if (command == "LOGIN") {
            char* server_ip = strtok(NULL, " ");
            char* port_str = strtok(NULL, " ");
            if (server_ip && port_str) {
                int server_port = atoi(port_str);
                login_to_server(server_ip, server_port, port);
                if (sockfd != -1) {
                        FD_SET(sockfd, &master_set);
                        if (sockfd > fdmax) fdmax = sockfd;
                    }
            } else {
                cse4589_print_and_log("[LOGIN:ERROR]\n[LOGIN:END]\n");
            }

        } else if (command == "REFRESH") {
            if (is_logged_in) {
                char ip_address[INET_ADDRSTRLEN];
                if (get_own_ip(ip_address) == 0) {
                    refresh_client_list(sockfd, std::string(ip_address), port);
                } else {
                    cse4589_print_and_log("[REFRESH:ERROR]\n[REFRESH:END]\n");
                }
            } else {
                cse4589_print_and_log("[REFRESH:ERROR]\n[REFRESH:END]\n");
            }
        } else if (command == "SEND") {
            if (!is_logged_in) {
                cse4589_print_and_log("[SEND:ERROR]\n");
                cse4589_print_and_log("[SEND:END]\n");
                continue;
            }

            char* dest_ip = strtok(NULL, " ");
            if (!dest_ip) {
                cse4589_print_and_log("[SEND:ERROR]\n");
                cse4589_print_and_log("[SEND:END]\n");
                continue;
            }

            // Find start of message
            char* msg_start = strstr(input_buffer, dest_ip);
            if (!msg_start) {
                cse4589_print_and_log("[SEND:ERROR]\n");
                cse4589_print_and_log("[SEND:END]\n");
                continue;
            }
            msg_start += strlen(dest_ip);
            while (*msg_start == ' ') msg_start++;  // Trim any leading space

            if (strlen(msg_start) == 0 || strlen(msg_start) > 256) {
                cse4589_print_and_log("[SEND:ERROR]\n");
                cse4589_print_and_log("[SEND:END]\n");
                continue;
            }

            // Validate dest_ip exists in current client list
            bool found = false;
            for (int i = 0; i < client_count; ++i) {
                if (strcmp(client_list[i].ip, dest_ip) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                cse4589_print_and_log("[SEND:ERROR]\n");
                cse4589_print_and_log("[SEND:END]\n");
                continue;
            }

            // Build and send: <dest_ip> <msg>
            std::stringstream msg_out;
            msg_out << dest_ip << " " << msg_start;

            if (send(sockfd, msg_out.str().c_str(), msg_out.str().length(), 0) < 0) {
                cse4589_print_and_log("[SEND:ERROR]\n");
                cse4589_print_and_log("[SEND:END]\n");
            } else {
                cse4589_print_and_log("[SEND:SUCCESS]\n");
                cse4589_print_and_log("[SEND:END]\n");
            }
        } else if (command == "BROADCAST") {
            if (!is_logged_in) {
                cse4589_print_and_log("[BROADCAST:ERROR]\n");
                cse4589_print_and_log("[BROADCAST:END]\n");
                continue;
            }

            // Get the full message after the word "BROADCAST"
            char* msg_start = strstr(input_buffer, " ");
            if (!msg_start || strlen(msg_start + 1) == 0) {
                cse4589_print_and_log("[BROADCAST:ERROR]\n");
                cse4589_print_and_log("[BROADCAST:END]\n");
                continue;
            }

            msg_start++;  // Move past the space after BROADCAST
            if (strlen(msg_start) > 256) {
                cse4589_print_and_log("[BROADCAST:ERROR]\n");
                cse4589_print_and_log("[BROADCAST:END]\n");
                continue;
            }

            std::stringstream msg_out;
            msg_out << "255.255.255.255 " << msg_start;

            if (send(sockfd, msg_out.str().c_str(), msg_out.str().length(), 0) < 0) {
                cse4589_print_and_log("[BROADCAST:ERROR]\n");
                cse4589_print_and_log("[BROADCAST:END]\n");
            } else {
                cse4589_print_and_log("[BROADCAST:SUCCESS]\n");
                cse4589_print_and_log("[BROADCAST:END]\n");
            }
        } else if (command == "BLOCK") {
            if (!is_logged_in) {
                cse4589_print_and_log("[BLOCK:ERROR]\n");
                cse4589_print_and_log("[BLOCK:END]\n");
                continue;
            }
            
            char* target_ip = strtok(NULL, " ");
            if (!target_ip) {
                cse4589_print_and_log("[BLOCK:ERROR]\n");
                cse4589_print_and_log("[BLOCK:END]\n");
                continue;
            }
            
            // Validate IP format
            struct sockaddr_in sa;
            if (inet_pton(AF_INET, target_ip, &(sa.sin_addr)) != 1) {
                cse4589_print_and_log("[BLOCK:ERROR]\n");
                cse4589_print_and_log("[BLOCK:END]\n");
                continue;
            }
            
            // Check if already blocked
            if (std::find(blocked_ips.begin(), blocked_ips.end(), target_ip) != blocked_ips.end()) {
                cse4589_print_and_log("[BLOCK:ERROR]\n");
                cse4589_print_and_log("[BLOCK:END]\n");
                continue;
            }
            
            // Check if IP exists in client list
            bool exists = false;
            for (int i = 0; i < client_count; ++i) {
                if (strcmp(client_list[i].ip, target_ip) == 0) {
                    exists = true;
                    break;
                }
            }
            
            if (!exists) {
                cse4589_print_and_log("[BLOCK:ERROR]\n");
                cse4589_print_and_log("[BLOCK:END]\n");
                continue;
            }
            
            // Send BLOCK command to server
            std::stringstream msg_out;
            msg_out << "BLOCK " << target_ip;
            
            if (send(sockfd, msg_out.str().c_str(), msg_out.str().length(), 0) < 0) {
                cse4589_print_and_log("[BLOCK:ERROR]\n");
                cse4589_print_and_log("[BLOCK:END]\n");
            } else {
                blocked_ips.push_back(target_ip);  // track locally
                cse4589_print_and_log("[BLOCK:SUCCESS]\n");
                cse4589_print_and_log("[BLOCK:END]\n");
            }
        } else if (command == "UNBLOCK") {
            if (!is_logged_in) {
                cse4589_print_and_log("[UNBLOCK:ERROR]\n");
                cse4589_print_and_log("[UNBLOCK:END]\n");
                continue;
            }

            char* target_ip = strtok(NULL, " ");
            if (!target_ip) {
                cse4589_print_and_log("[UNBLOCK:ERROR]\n");
                cse4589_print_and_log("[UNBLOCK:END]\n");
                continue;
            }

            // Validate IP format
            struct sockaddr_in sa;
            if (inet_pton(AF_INET, target_ip, &(sa.sin_addr)) != 1) {
                cse4589_print_and_log("[UNBLOCK:ERROR]\n");
                cse4589_print_and_log("[UNBLOCK:END]\n");
                continue;
            }

            // Check if IP exists in client list
            bool exists = false;
            for (int i = 0; i < client_count; ++i) {
                if (strcmp(client_list[i].ip, target_ip) == 0) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                cse4589_print_and_log("[UNBLOCK:ERROR]\n");
                cse4589_print_and_log("[UNBLOCK:END]\n");
                continue;
            }

            // Check if this IP is in blocked list
            auto it = std::find(blocked_ips.begin(), blocked_ips.end(), target_ip);
            if (it == blocked_ips.end()) {
                cse4589_print_and_log("[UNBLOCK:ERROR]\n");
                cse4589_print_and_log("[UNBLOCK:END]\n");
                continue;
            }

            // Send UNBLOCK command to server
            std::stringstream msg_out;
            msg_out << "UNBLOCK " << target_ip;

            if (send(sockfd, msg_out.str().c_str(), msg_out.str().length(), 0) < 0) {
                cse4589_print_and_log("[UNBLOCK:ERROR]\n");
                cse4589_print_and_log("[UNBLOCK:END]\n");
            } else {
                blocked_ips.erase(it);  // remove locally
                cse4589_print_and_log("[UNBLOCK:SUCCESS]\n");
                cse4589_print_and_log("[UNBLOCK:END]\n");
            }
        } else if (command == "LOGOUT") {
            if (!is_logged_in) {
                cse4589_print_and_log("[LOGOUT:ERROR]\n");
                cse4589_print_and_log("[LOGOUT:END]\n");
                continue;
            }
            
            close(sockfd);  // Close connection to server
            FD_CLR(sockfd, &master_set);
            sockfd = -1;
            is_logged_in = false;
            
            cse4589_print_and_log("[LOGOUT:SUCCESS]\n");
            cse4589_print_and_log("[LOGOUT:END]\n");
        } else if (command == "EXIT") {
            if (is_logged_in && sockfd != -1) {
                close(sockfd);  // Gracefully close the connection
                FD_CLR(sockfd, &master_set);
                sockfd = -1;
            }
            
            // Cleanup local state
            is_logged_in = false;
            blocked_ips.clear();
            client_count = 0;
            
            exit(0);  // Exit the application immediately
        } else {
            cse4589_print_and_log("[%s:ERROR]\n", command.c_str());
            cse4589_print_and_log("[%s:END]\n", command.c_str());
        }
        // Handle incoming message from server
        if (sockfd != -1 && FD_ISSET(sockfd, &read_fds)) {
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            int nbytes = recv(sockfd, buffer, sizeof(buffer), 0);

            if (nbytes <= 0) {
                close(sockfd);
                FD_CLR(sockfd, &master_set);
                sockfd = -1;
                is_logged_in = false;
                continue;
            }

            std::string incoming(buffer);
            size_t space_pos = incoming.find(' ');
            if (space_pos != std::string::npos) {
                std::string sender_ip = incoming.substr(0, space_pos);
                std::string msg = incoming.substr(space_pos + 1);

                // EVENT logging format
                cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                cse4589_print_and_log("msg from:%s\n[msg]:%s\n", sender_ip.c_str(), msg.c_str());
                cse4589_print_and_log("[RECEIVED:END]\n");
            } 
        }
        printf("[PA1-Client@CSE489/589]$ ");
        fflush(stdout);
    }
}
}