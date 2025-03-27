#include "../include/global.h"
#include "../include/logger.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <algorithm>

#define STDIN 0
#define MAX_INPUT_SIZE 65535

using namespace std;

vector<SocketEntry> connected_clients;

void set_non_blocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

bool sort_by_port(const SocketEntry& a, const SocketEntry& b) {
    return a.port < b.port;
}

void print_statistics_command() {
    std::vector<SocketEntry> sorted_list = connected_clients;

    std::sort(sorted_list.begin(), sorted_list.end(), sort_by_port);

    cse4589_print_and_log("[STATISTICS:SUCCESS]\n");

    int index = 1;
    for (const auto& client : sorted_list) {
        cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n",
            index,
            client.hostname.c_str(),
            client.num_msg_sent,
            client.num_msg_rcv,
            client.status.c_str());
        index++;
    }

    cse4589_print_and_log("[STATISTICS:END]\n");
}

void print_blocked_command(const std::string& ip_arg) {
    auto it = std::find_if(connected_clients.begin(), connected_clients.end(),
        [&ip_arg](const SocketEntry& client) {
            return client.ip == ip_arg;
        });

    if (it == connected_clients.end()) {
        cse4589_print_and_log("[BLOCKED:ERROR]\n");
        fprintf(stderr, "Invalid IP or IP not in connected_clients\n");
        cse4589_print_and_log("[BLOCKED:END]\n");
        return;
    }

    std::vector<SocketEntry> blocked_entries;
    for (const auto& blocked_ip : it->blocked_ips) {
        auto found = std::find_if(connected_clients.begin(), connected_clients.end(),
            [&blocked_ip](const SocketEntry& c) { return c.ip == blocked_ip; });

        if (found != connected_clients.end()) {
            blocked_entries.push_back(*found);
        }
    }

    std::sort(blocked_entries.begin(), blocked_entries.end(), sort_by_port);

    cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
    int index = 1;
    for (const auto& blocked : blocked_entries) {
        cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",
                              index,
                              blocked.hostname.c_str(),
                              blocked.ip.c_str(),
                              blocked.port);
        index++;
    }
    cse4589_print_and_log("[BLOCKED:END]\n");
}

void print_list_command() {
    vector<SocketEntry> clients_only;

    for (const auto& client : connected_clients) {
        if (client.status == "logged-in") {
            clients_only.push_back(client);
        }
    }

    sort(clients_only.begin(), clients_only.end(), sort_by_port);

    cse4589_print_and_log("[LIST:SUCCESS]\n");
    int index = 1;
    for (const auto& client : clients_only) {
        cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",
                              index,
                              client.hostname.c_str(),
                              client.ip.c_str(),
                              client.port);
        index++;
    }
    cse4589_print_and_log("[LIST:END]\n");
}

string get_ip_by_fd(int fd) {
    for (const auto& client : connected_clients) {
        if (client.fd == fd) {
            return client.ip;
        }
    }
    return "";
}

void log_event_message(const string& from_ip, const string& to_ip, const string& msg) {
    cse4589_print_and_log("[RELAYED:SUCCESS]\n");
    cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from_ip.c_str(), to_ip.c_str(), msg.c_str());
    cse4589_print_and_log("[RELAYED:END]\n");
}

void run_server(int port) {
    char input_buffer[MAX_INPUT_SIZE];
    char ip_address[INET_ADDRSTRLEN];

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return;
    }

    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        return;
    }

    fd_set master_set, read_fds;
    FD_ZERO(&master_set);
    FD_SET(STDIN_FILENO, &master_set);
    FD_SET(server_socket, &master_set);
    int fdmax = server_socket;

    printf("[PA1-Server@CSE489/589]$ ");
    fflush(stdout);

    while (1) {
        read_fds = master_set;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("Select error");
            break;
        }

        for (int i = 0; i <= fdmax; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                std::string hostname, ip;
                int client_port = -1;
                if (i == STDIN_FILENO) {
                    printf("\n[PA1-Server@CSE489/589]$ ");
                    fflush(stdout);

                    if (fgets(input_buffer, MAX_INPUT_SIZE, stdin) == NULL)
                        continue;

                    input_buffer[strcspn(input_buffer, "\n")] = '\0';

                    if (strlen(input_buffer) == 0)
                        continue;

                    for (int i = 0; input_buffer[i]; i++) {
                        if (input_buffer[i] >= 'a' && input_buffer[i] <= 'z') {
                            input_buffer[i] = input_buffer[i] - 32;
                        }
                    }

                    if (strcmp(input_buffer, "AUTHOR") == 0) {
                        cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
                        cse4589_print_and_log("I, syedzuba, have read and understood the course academic integrity policy.\n");
                        cse4589_print_and_log("[AUTHOR:END]\n");

                    } else if (my_strcmp(input_buffer, "IP") == 0) {
                        if (get_own_ip(ip_address) == 0) {
                            cse4589_print_and_log("[IP:SUCCESS]\n");
                            cse4589_print_and_log("IP:%s\n", ip_address);
                            cse4589_print_and_log("[IP:END]\n");
                        } else {
                            cse4589_print_and_log("[IP:ERROR]\n");
                            cse4589_print_and_log("[IP:END]\n");
                        }

                    } else if (my_strcmp(input_buffer, "PORT") == 0) {
                        print_port_success(port);

                    } else if (my_strcmp(input_buffer, "LIST") == 0) {
                        print_list_command();
                    } else if (my_strcmp(input_buffer, "STATISTICS") == 0) {
                        print_statistics_command();
                    } else if (strncmp(input_buffer, "BLOCKED ", 8) == 0) {
                        std::string arg_ip = input_buffer + 8;
                        print_blocked_command(arg_ip);
                    } else if (my_strcmp(input_buffer, "EXIT") == 0) {
                        return;
                    } else {
                        cse4589_print_and_log("[%s:ERROR]\n", input_buffer);
                        cse4589_print_and_log("[%s:END]\n", input_buffer);
                    }

                } else if (i == server_socket) {
                    struct sockaddr_in client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    int newfd = accept(server_socket, (struct sockaddr*)&client_addr, &addrlen);

                    if (newfd >= 0) {
                        FD_SET(newfd, &master_set);
                        if (newfd > fdmax) fdmax = newfd;
                    }

                } else {
                    // Handle incoming client messages
                    char buffer[1024];
                    memset(buffer, 0, sizeof(buffer));

                    int nbytes = recv(i, buffer, sizeof(buffer), 0);
                    if (nbytes <= 0) {
                        // Instead of erasing the client, just mark them as logged-out
                        for (auto& client : connected_clients) {
                            if (client.fd == i) {
                                client.status = "logged-out";
                                client.fd = -1;
                                break;
                            }
                        }
                        
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    
                    string raw_msg(buffer);
                    if (raw_msg.rfind("LOGIN ", 0) == 0) {
                        std::istringstream iss(raw_msg);
                        std::string cmd, hostname, ip;
                        int client_port;
                        iss >> cmd >> hostname >> ip >> client_port;

                        bool updated = false;
                        for (auto& client : connected_clients) {
                            if (client.ip == ip && client.port == client_port) {
                                client.hostname = hostname;
                                client.status = "logged-in";
                                client.fd = i;  // make sure to update fd
                                updated = true;
                                break;
                            }
                        }

                        // In case this is truly a new client (shouldn't happen often)
                        if (!updated) {
                            SocketEntry new_client;
                            new_client.hostname = hostname;
                            new_client.ip = ip;
                            new_client.port = client_port;
                            new_client.fd = i;
                            new_client.status = "logged-in";
                            new_client.num_msg_rcv = 0;
                            new_client.num_msg_sent = 0;
                            connected_clients.push_back(new_client);
                        }

                        // Send back logged-in client list
                        std::stringstream response;
                        for (const auto& client : connected_clients) {
                            if (client.status == "logged-in") {
                                response << "CLIENT "
                                         << client.hostname << " "
                                         << client.ip << " "
                                         << client.port << "\n";
                            }
                        }

                        std::string response_str = response.str();
                        send(i, response_str.c_str(), response_str.size(), 0);

                        // Deliver any buffered messages
                        auto it = std::find_if(connected_clients.begin(), connected_clients.end(), [&ip, &client_port](const SocketEntry& c) { return c.ip == ip && c.port == client_port; });

                        if (it != connected_clients.end()) {
                            for (const auto& m : it->buffered_messages) {
                                send(i, m.c_str(), m.length(), 0);
                                usleep(1000); // small delay to avoid TCP congestion
                            }
                            
                            it->buffered_messages.clear();
                        }
                        continue;
                    }

                    else if (raw_msg == "EXIT") {
                        connected_clients.erase(
                            std::remove_if(
                                connected_clients.begin(), connected_clients.end(),[i](const SocketEntry& client) { return client.fd == i; }
                            ),
                            connected_clients.end()
                        );
                        
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }



                    else if (raw_msg == "LOGOUT") {
                        for (auto& client : connected_clients) {
                            if (client.fd == i) {
                                client.status = "logged-out";
                                client.fd = -1;
                                break;
                            }
                        }
                        
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    
                    else if (raw_msg.rfind("UNBLOCK ", 0) == 0) {
                        string unblock_ip = raw_msg.substr(8);
                        string unblocker_ip = get_ip_by_fd(i);

                        bool is_valid_ip = false;
                        bool was_blocked = false;

                        for (auto& client : connected_clients) {
                            if (client.ip == unblocker_ip) {
                                auto it = std::find(client.blocked_ips.begin(), client.blocked_ips.end(), unblock_ip);

                                if (it != client.blocked_ips.end()) {
                                    client.blocked_ips.erase(it);
                                    was_blocked = true;
                                }

                                // Check if unblock_ip is valid (i.e., was in the logged-in client list)
                                for (const auto& other : connected_clients) {
                                    if (other.ip == unblock_ip) {
                                        is_valid_ip = true;
                                        break;
                                    }
                                }

                                break;
                            }
                        }

                        // Error handling if needed
                        if (!is_valid_ip) {
                            cse4589_print_and_log("[UNBLOCK:ERROR]\n");
                            cse4589_print_and_log("[UNBLOCK:END]\n");
                        } else if (!was_blocked) {
                            cse4589_print_and_log("[UNBLOCK:ERROR]\n");
                            cse4589_print_and_log("[UNBLOCK:END]\n");
                        }

                        continue;
                    }
                    else if (raw_msg.rfind("BLOCK ", 0) == 0) {
                        string blocked_ip = raw_msg.substr(6);
                        string blocker_ip = get_ip_by_fd(i);
                        
                        for (auto& client : connected_clients) {
                            if (client.ip == blocker_ip) {
                                // Avoid duplicate entries
                                if (std::find(client.blocked_ips.begin(), client.blocked_ips.end(), blocked_ip) == client.blocked_ips.end()) {
                                    client.blocked_ips.push_back(blocked_ip);
                                }
                                break;
                            }
                        }
                        continue; // Do not treat this like a SEND
                    }


                    // Example format: "<TO-IP> <MESSAGE>"
                    string full_msg(buffer);
                    size_t space_pos = full_msg.find(' ');
                    if (space_pos != string::npos) {
                        string to_ip = full_msg.substr(0, space_pos);
                        string msg = full_msg.substr(space_pos + 1);
                        string from_ip = get_ip_by_fd(i);

                        log_event_message(from_ip, to_ip, msg);

                        if (to_ip == "255.255.255.255") {
                            // Broadcast to all logged-in clients EXCEPT sender and those who blocked the sender
                            for (auto& client : connected_clients) {
                                if (client.ip != from_ip) {
                                    bool blocked = std::find(client.blocked_ips.begin(), client.blocked_ips.end(), from_ip) != client.blocked_ips.end();
                                    
                                    if (client.status == "logged-in") {
                                        if (!blocked) {
                                            string forward = from_ip + " " + msg;
                                            send(client.fd, forward.c_str(), forward.length(), 0);
                                            client.num_msg_rcv++;
                                        }
                                    } else {
                                        // Offline: only buffer if not blocked
                                        if (!blocked) {
                                            string buffer_entry = from_ip + " " + msg;
                                            client.buffered_messages.push_back(buffer_entry);
                                        }
                                    }
                                }
                            }
                            
                            // Log & increment sender's message count
                            log_event_message(from_ip, "255.255.255.255", msg);
                            for (auto& client : connected_clients) {
                                if (client.ip == from_ip) {
                                    client.num_msg_sent++;
                                    break;
                                }
                            }
                        } else {
                            // Unicast SEND message
                            auto sender_it = std::find_if(connected_clients.begin(), connected_clients.end(),[&from_ip](const SocketEntry& c) { return c.ip == from_ip; });
                            auto receiver_it = std::find_if(connected_clients.begin(), connected_clients.end(),[&to_ip](const SocketEntry& c) { return c.ip == to_ip; });
                            
                            if (sender_it != connected_clients.end() && receiver_it != connected_clients.end()) {
                                bool blocked = std::find(receiver_it->blocked_ips.begin(), receiver_it->blocked_ips.end(), from_ip) != receiver_it->blocked_ips.end();

                                if (!blocked) {
                                    string forward = from_ip + " " + msg;

                                    if (receiver_it->status == "logged-in") {
                                        send(receiver_it->fd, forward.c_str(), forward.length(), 0);
                                        receiver_it->num_msg_rcv++;

                                        log_event_message(from_ip, to_ip, msg);
                                    } else {
                                        // Receiver is logged out — buffer the message
                                        receiver_it->buffered_messages.push_back(forward);
                                    }
                                }

                                sender_it->num_msg_sent++;
                            }

                        }
                    }
                }

                printf("\n[PA1-Server@CSE489/589]$ ");
                fflush(stdout);
            }
        }
    }

    close(server_socket);
}
