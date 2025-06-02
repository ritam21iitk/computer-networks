#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define PORT 12345
#define MAX_CLIENTS 10

// Enum for message types (optional/enumerative use)
enum class MessageType {
    BROADCAST_MESSAGE,
    PRIVATE_MESSAGE,
    GROUP_MESSAGE,
    CREATE_GROUP,
    JOIN_GROUP,
    LEAVE_GROUP,
    UNKNOWN
};

// -----------------------------------
// ErrorHandler Class
// -----------------------------------
class ErrorHandler {
public:
    static void send_error(int client_socket, const std::string& message) {
        send(client_socket, message.c_str(), message.size(), 0);
    }

    static void authentication_failed(int client_socket) {
        std::string msg = "[Error] Authentication failed. Invalid username or password.\n";
        send(client_socket, msg.c_str(), msg.size(), 0);
        close(client_socket);
    }

    static void unknown_command(int client_socket) {
        std::string msg = "[Error] Unknown command. Use /help for available commands.\n";
        send(client_socket, msg.c_str(), msg.size(), 0);
    }

    static void not_a_group_member(int client_socket) {
        std::string msg = "[Error] You are not a member of this group. Join first using /join_group.\n";
        send(client_socket, msg.c_str(), msg.size(), 0);
    }

    static void group_not_exist(int client_socket) {
        std::string msg = "[Error] Group does not exist. Create one using /create_group.\n";
        send(client_socket, msg.c_str(), msg.size(), 0);
    }

    static void group_already_exists(int client_socket) {
        std::string msg = "[Error] Group already exists. Try joining using /join_group.\n";
        send(client_socket, msg.c_str(), msg.size(), 0);
    }

    static void user_not_found(int client_socket) {
        std::string msg = "[Error] User not found. Check the username and try again.\n";
        send(client_socket, msg.c_str(), msg.size(), 0);
    }

    static void not_in_group(int client_socket) {
        std::string msg = "[Error] You are not in this group or the group does not exist.\n";
        send(client_socket, msg.c_str(), msg.size(), 0);
    }

    static void socket_creation_failed() {
        std::cerr << "[Error] Failed to create socket.\n";
        exit(EXIT_FAILURE);
    }

    static void binding_failed() {
        std::cerr << "[Error] Binding failed.\n";
        exit(EXIT_FAILURE);
    }

    static void listening_failed() {
        std::cerr << "[Error] Listening failed.\n";
        exit(EXIT_FAILURE);
    }

    static void client_accept_failed() {
        std::cerr << "[Error] Failed to accept client connection.\n";
    }
};

// -----------------------------------
// BroadcastMessage Class
// -----------------------------------
class BroadcastMessage {
private:
    std::unordered_map<int, std::string>& clients;
    std::mutex& clients_mutex;
public:
    BroadcastMessage(std::unordered_map<int, std::string>& clients, std::mutex& clients_mutex)
        : clients(clients), clients_mutex(clients_mutex) {}

    void send_broadcast(int sender_socket, const std::string& message) {
        std::lock_guard<std::mutex> lock(clients_mutex);

        // Safely confirm we know the sender
        if (clients.find(sender_socket) == clients.end()) {
            // If for some reason the sender isn't recognized (e.g. disconnected),
            // we can ignore or send an error back:
            std::string err = "[Error] You are not recognized as an active user.\n";
            send(sender_socket, err.c_str(), err.size(), 0);
            return;
        }

        // Get sender's username
        std::string sender_name = clients[sender_socket];
        // Build the broadcast message
        std::string broadcast_msg = "[Broadcast from " + sender_name + "]: " + message + "\n";

        // Send to all connected users except the sender
        for (const auto& [socket, username] : clients) {
            if (socket != sender_socket) {
                send(socket, broadcast_msg.c_str(), broadcast_msg.size(), 0);
            }
        }
    }


    // Utility to let others know who joined/left (optional but typical in chat)
    void announce(const std::string& announcement) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::string msg = announcement + "\n";
        for (const auto& [socket, username] : clients) {
            send(socket, msg.c_str(), msg.size(), 0);
        }
    }
};

// -----------------------------------
// PrivateMessage Class
// -----------------------------------
class PrivateMessage {
private:
    std::unordered_map<int, std::string>& clients;
    std::mutex& clients_mutex;

public:
    PrivateMessage(std::unordered_map<int, std::string>& clients, std::mutex& clients_mutex)
        : clients(clients), clients_mutex(clients_mutex) {}

    void send_private_message(int client_socket, const std::string& recipient, const std::string& message) {
        std::lock_guard<std::mutex> lock(clients_mutex);

        if (clients.find(client_socket) == clients.end()) {
            std::string err = "[Error] You are not recognized as an active user.\n";
            send(client_socket, err.c_str(), err.size(), 0);
            return;
        }

        // Find recipient's socket
        int recipient_socket = -1;
        bool found = false;
        for (const auto& [socket, user] : clients) {
            if (user == recipient) {
                recipient_socket = socket;
                found = true;
                break;
            }
        }
        if (!found) {
            std::string err = "[Error] User not active.\n";
            send(client_socket, err.c_str(), err.size(), 0);
            return;
        }

        std::string sender = clients[client_socket];
        std::string formatted_message = "[Private from " + sender + "]: " + message + "\n";

        // Send to recipient
        send(recipient_socket, formatted_message.c_str(), formatted_message.size(), 0);
    }
};

// -----------------------------------
// GroupManager Class
// -----------------------------------
class GroupManager {
private:
    std::unordered_map<std::string, std::unordered_set<int>> groups;
    std::mutex groups_mutex;

public:
    // Create a group
    void create_group(int client_socket, const std::string& group_name) {
        std::lock_guard<std::mutex> lock(groups_mutex);
        if (groups.find(group_name) == groups.end()) {
            groups[group_name].insert(client_socket);
            std::string msg = "Group " + group_name + " created.\n";
            send(client_socket, msg.c_str(), msg.size(), 0);
        } else {
            ErrorHandler::group_already_exists(client_socket);
        }
    }

    // Join a group
    void join_group(int client_socket,const std::string& username, const std::string& group_name) {
        std::lock_guard<std::mutex> lock(groups_mutex);
        auto it = groups.find(group_name);
        if (it != groups.end()) {
            groups[group_name].insert(client_socket);
            std::string msg = "You joined the group " + group_name + ".\n";
            send(client_socket, msg.c_str(), msg.size(), 0);
                // Build announcement for all group members
            std::string announce_msg = "[Group " + group_name + "] " + username + " has joined.\n";

            for (int sock : it->second) {
                
                if (sock == client_socket) continue; 
                send(sock, announce_msg.c_str(), announce_msg.size(), 0);
            }
        } else {
            ErrorHandler::group_not_exist(client_socket);
        }
    }

    // Leave a group
    void leave_group(int client_socket, const std::string& username,const std::string& group_name) {
        std::lock_guard<std::mutex> lock(groups_mutex);
        auto it = groups.find(group_name);
        if (it != groups.end()) {
            if (it->second.erase(client_socket) > 0) {
                std::string msg = "You left the group " + group_name + ".\n";
                send(client_socket, msg.c_str(), msg.size(), 0);

                // Announce to group members
                std::string announce_msg = "[Group " + group_name + "] " + username + " has left.\n";
                for (int sock : it->second) {
                    send(sock, announce_msg.c_str(), announce_msg.size(), 0);
                }
            } else {
                ErrorHandler::not_in_group(client_socket);
            }
        } else {
            ErrorHandler::not_in_group(client_socket);
        }
    }

    // Send a group message
    void send_group_message(int client_socket, const std::string& sender_username,  const std::string& group_name, const std::string& message) {
        std::lock_guard<std::mutex> lock(groups_mutex);
        auto it = groups.find(group_name);
        if (it == groups.end()) {
            ErrorHandler::group_not_exist(client_socket);
            return;
        }
        // Check membership
        if (!it->second.count(client_socket)) {
            ErrorHandler::not_a_group_member(client_socket);
            return;
        }

        // Relay message to all in group
        std::string group_msg = "[Group " + group_name +"] "+  sender_username + " "  + message + "\n";
        for (int socket : it->second) {
            if (socket != client_socket) {
                send(socket, group_msg.c_str(), group_msg.size(), 0);
            }
        }
    }

    // Remove a socket from ALL groups (for when client disconnects)
    void remove_socket_from_all_groups(int client_socket) {
        std::lock_guard<std::mutex> lock(groups_mutex);
        for (auto& [gname, members] : groups) {
            members.erase(client_socket);
        }
    }
};

// -----------------------------------
// ServerManager Class
// -----------------------------------
class ServerManager {
private:
    std::unordered_map<std::string, std::string> users;   // Valid username->password pairs
    std::unordered_map<int, std::string> clients;         // socket->username
    std::mutex clients_mutex;

    // Single GroupManager shared by all connections
    GroupManager group_manager;

    // Load users from file
    void load_users(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            size_t delimiter = line.find(":");
            if (delimiter != std::string::npos) {
                std::string username = line.substr(0, delimiter);
                std::string password = line.substr(delimiter + 1);

                // Trim possible extra whitespace/newlines
                username.erase(username.find_last_not_of(" \n\r\t") + 1);
                password.erase(password.find_last_not_of(" \n\r\t") + 1);

                users[username] = password;
            }
        }
    }

public:
    void start() {
        load_users("users.txt");

        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            ErrorHandler::socket_creation_failed();
        }

        sockaddr_in server_address{};
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;
        server_address.sin_port = htons(PORT);

        if (bind(server_socket, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
            ErrorHandler::binding_failed();
        }

        if (listen(server_socket, MAX_CLIENTS) < 0) {
            ErrorHandler::listening_failed();
        }

        std::cout << "[Server] Running on port " << PORT << "...\n";

        while (true) {
            sockaddr_in client_address;
            socklen_t client_address_len = sizeof(client_address);
            int client_socket = accept(server_socket, (sockaddr*)&client_address, &client_address_len);

            if (client_socket < 0) {
                ErrorHandler::client_accept_failed();
                continue;
            }

            std::cout << "[Server] New client connected.\n";

            // Handle this client in a dedicated thread
            std::thread(&ServerManager::handle_client, this, client_socket).detach();
        }

        close(server_socket);
    }

    // Handle client: authentication + command loop
    void handle_client(int client_socket) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);

        // Prompt for username
        {
            std::string prompt = "Enter username: ";
            send(client_socket, prompt.c_str(), prompt.size(), 0);
        }
        recv(client_socket, buffer, BUFFER_SIZE, 0);
        std::string username(buffer);
        // trim trailing newlines/spaces
        username.erase(username.find_last_not_of(" \n\r\t") + 1);

        // Prompt for password
        {
            std::string prompt = "Enter password: ";
            send(client_socket, prompt.c_str(), prompt.size(), 0);
        }
        memset(buffer, 0, BUFFER_SIZE);
        recv(client_socket, buffer, BUFFER_SIZE, 0);
        std::string password(buffer);
        password.erase(password.find_last_not_of(" \n\r\t") + 1);

        // Check authentication
        if (users.find(username) == users.end() || users[username] != password) {
            ErrorHandler::authentication_failed(client_socket);
            return;
        }

        // Auth successful
        {
            std::string msg = "Authentication successful!\n";
            send(client_socket, msg.c_str(), msg.size(), 0);
            std::cout << "[Server] User " << username << " authenticated.\n";
        }

        // Add to global clients list
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients[client_socket] = username;
        }

        // Optional: announce to all that <username> joined
        {
            BroadcastMessage broadcast(clients, clients_mutex);
            broadcast.announce(username + " has joined the chat.");
        }

        // Create message-handling helpers
        BroadcastMessage broadcast(clients, clients_mutex);
        PrivateMessage private_msg(clients, clients_mutex);

        // Main receive loop
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);

            if (bytes_received <= 0) {
                // Client disconnected or error
                std::cout << "[Server] Client " << username << " disconnected.\n";

                // Remove from clients
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    clients.erase(client_socket);
                }
                // Remove from groups
                group_manager.remove_socket_from_all_groups(client_socket);

                // Optional: broadcast user leaving
                broadcast.announce(username + " has left the chat.");

                close(client_socket);
                return;
            }

            std::string message(buffer);

            if (message.starts_with("/broadcast ")) {
                // /broadcast <message>
                broadcast.send_broadcast(client_socket, message.substr(11));

            } else if (message.starts_with("/msg ")) {
                // /msg <username> <message>
                size_t space_pos = message.find(' ', 5);
                if (space_pos != std::string::npos) {
                    std::string recipient = message.substr(5, space_pos - 5);
                    std::string pm = message.substr(space_pos + 1);
                    private_msg.send_private_message(client_socket, recipient, pm);
                }

            } else if (message.starts_with("/create_group ")) {
                // /create_group <group_name>
                std::string group_name = message.substr(14);
                group_name.erase(group_name.find_last_not_of(" \n\r\t") + 1);
                group_manager.create_group(client_socket, group_name);

            } else if (message.starts_with("/join_group ")) {
                // /join_group <group_name>
                std::string group_name = message.substr(12);
                group_name.erase(group_name.find_last_not_of(" \n\r\t") + 1);
                group_manager.join_group(client_socket, username, group_name);

            } else if (message.starts_with("/leave_group ")) {
                // /leave_group <group_name>
                std::string group_name = message.substr(13);
                group_name.erase(group_name.find_last_not_of(" \n\r\t") + 1);
                group_manager.leave_group(client_socket, username, group_name);

            } else if (message.starts_with("/group_msg ")) {
                // /group_msg <group_name> <message>
                // e.g. "/group_msg CS425 Hello everyone!"
                size_t space_pos = message.find(' ', 11);
                if (space_pos != std::string::npos) {
                    std::string group_name = message.substr(11, space_pos - 11);
                    std::string group_msg  = message.substr(space_pos + 1);
                    group_manager.send_group_message(client_socket,username, group_name, group_msg);
                }

            } else if (message == "/exit") {
                // Optional: let user type /exit to disconnect gracefully
                std::cout << "[Server] User " << username << " requested /exit.\n";
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    clients.erase(client_socket);
                }
                group_manager.remove_socket_from_all_groups(client_socket);
                broadcast.announce(username + " has left the chat.");
                close(client_socket);
                return;

            } else {
                ErrorHandler::unknown_command(client_socket);
            }
        }
    }
};

int main() {
    ServerManager server;
    server.start();
    return 0;
}
