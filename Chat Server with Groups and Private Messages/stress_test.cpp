#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <random>
#include <chrono>

// -------------------------------------------------------------------
// Configuration
// -------------------------------------------------------------------
static const char* SERVER_HOST = "127.0.0.1";
static const int   SERVER_PORT = 12345;
static const int   NUM_CLIENTS = 1000; // number of simulated clients
static const int   BUFFER_SIZE = 1024;

// A set of valid users from users.txt (adjust as needed)
static const std::vector<std::pair<std::string,std::string>> TEST_USERS = {
    {"alice",   "password123"},
    {"bob",     "qwerty456"},
    {"charlie", "secure789"},
    {"david",   "helloWorld!"},
    {"eve",     "trustno1"},
    {"frank",   "letmein"},
    {"grace",   "passw0rd"}
};

// A few random messages to send in broadcast or private
static const std::vector<std::string> RANDOM_MESSAGES = {
    "Hello world!",
    "CS425 is awesome",
    "Testing the server",
    "How's everyone?",
    "Network labs are fun",
    "Lorem ipsum dolor sit amet"
};

// Some random group names
static const std::vector<std::string> GROUP_NAMES = {
    "CS425", "TestGroup", "Networkers", "CoolGroup", "FridayFun"
};

// -------------------------------------------------------------------
// Utility: connect to server, read/write lines
// -------------------------------------------------------------------
int connect_to_server(const char* host, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "[Error] Failed to create socket.\n";
        return -1;
    }

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[Error] Failed to connect to server.\n";
        close(sockfd);
        return -1;
    }
    return sockfd;
}

// -------------------------------------------------------------------
// Utility: receive a line (up to BUFFER_SIZE) from socket
//          This is a simplistic approach; may read partial messages
// -------------------------------------------------------------------
std::string recv_line(int sockfd) {
    char buffer[BUFFER_SIZE];
    std::memset(buffer, 0, BUFFER_SIZE);
    int n = recv(sockfd, buffer, BUFFER_SIZE-1, 0);
    if (n <= 0) {
        return ""; // indicates closed or error
    }
    return std::string(buffer);
}

// -------------------------------------------------------------------
// Utility: send a line with newline
// -------------------------------------------------------------------
void send_line(int sockfd, const std::string &line) {
    // You can decide whether to append a newline
    // The server code typically just reads up to buffer length.
    // We'll send exactly the line as is, no extra newline unless you want it.
    send(sockfd, line.c_str(), line.size(), 0);
}

// -------------------------------------------------------------------
// Worker function: each thread simulates a single client
// -------------------------------------------------------------------
void simulate_client(int index) {
    int sockfd = connect_to_server(SERVER_HOST, SERVER_PORT);
    if (sockfd < 0) {
        std::cerr << "[Client " << index << "] Unable to connect.\n";
        return;
    }

    // Select user credentials (cycling or random)
    auto user_cred = TEST_USERS[ index % TEST_USERS.size() ];
    const std::string& username = user_cred.first;
    const std::string& password = user_cred.second;

    // Print which user we’re simulating
    std::cout << "[Client " << index << "] Using credentials: ("
              << username << ", " << password << ")\n";

    // Read "Enter username:" prompt
    std::string prompt1 = recv_line(sockfd);
    if (prompt1.empty()) {
        std::cerr << "[Client " << index << "] Server closed immediately.\n";
        close(sockfd);
        return;
    }
    // Send username
    send_line(sockfd, username);

    // Read "Enter password:"
    std::string prompt2 = recv_line(sockfd);
    if (prompt2.empty()) {
        std::cerr << "[Client " << index << "] No password prompt.\n";
        close(sockfd);
        return;
    }
    // Send password
    send_line(sockfd, password);

    // Read auth response
    std::string auth_resp = recv_line(sockfd);
    if (auth_resp.find("failed") != std::string::npos ||
        auth_resp.find("Error")  != std::string::npos)
    {
        std::cerr << "[Client " << index << "] Authentication failed for "
                  << username << ".\n";
        close(sockfd);
        return;
    }
    // Otherwise, auth is successful
    std::cout << "[Client " << index << "] Authenticated successfully.\n";

    // Random engine and distributions
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist_action(0, 5);   // pick an action
    std::uniform_int_distribution<> dist_sleep(500, 1500); // ms
    std::uniform_int_distribution<> dist_msgs(0, RANDOM_MESSAGES.size()-1);
    std::uniform_int_distribution<> dist_groups(0, GROUP_NAMES.size()-1);
    std::uniform_int_distribution<> dist_users(0, TEST_USERS.size()-1);

    // Send 5 random commands
    for (int i = 0; i < 5; ++i) {
        int action = dist_action(gen);
        std::string cmd;

        switch (action) {
            case 0: {
                // /broadcast <message>
                int msg_idx = dist_msgs(gen);
                cmd = "/broadcast " + RANDOM_MESSAGES[msg_idx];
            } break;
            case 1: {
                // /group_msg <group_name> <message>
                int g_idx = dist_groups(gen);
                int msg_idx = dist_msgs(gen);
                cmd = "/group_msg " + GROUP_NAMES[g_idx] + " " + RANDOM_MESSAGES[msg_idx];
            } break;
            case 2: {
                // /msg <username> <message>
                int usr_idx = dist_users(gen);
                int msg_idx = dist_msgs(gen);
                cmd = "/msg " + TEST_USERS[usr_idx].first + " " + RANDOM_MESSAGES[msg_idx];
            } break;
            case 3: {
                // /create_group <group_name>
                int g_idx = dist_groups(gen);
                cmd = "/create_group " + GROUP_NAMES[g_idx];
            } break;
            case 4: {
                // /join_group <group_name>
                int g_idx = dist_groups(gen);
                cmd = "/join_group " + GROUP_NAMES[g_idx];
            } break;
            case 5: {
                // /leave_group <group_name>
                int g_idx = dist_groups(gen);
                cmd = "/leave_group " + GROUP_NAMES[g_idx];
            } break;
        }

        // **Print the command** for visibility
        std::cout << "[Client " << index << "] Sending command: " << cmd << "\n";

        // Send it
        send_line(sockfd, cmd);

        // Sleep 0.5 to 1.5 seconds
        int ms = dist_sleep(gen);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    // Disconnect
    std::string exit_cmd = "/exit";
    std::cout << "[Client " << index << "] Sending command: " << exit_cmd << "\n";
    send_line(sockfd, exit_cmd);

    close(sockfd);
    std::cout << "[Client " << index << "] Disconnected.\n";
}

// -------------------------------------------------------------------
// Main: spawn multiple client threads
// -------------------------------------------------------------------
int main() {
    std::vector<std::thread> threads;
    threads.reserve(NUM_CLIENTS);

    for (int i = 0; i < NUM_CLIENTS; i++) {
        threads.emplace_back(simulate_client, i);
        // Optionally stagger starts slightly
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Wait for all
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Stress test complete." << std::endl;
    return 0;
}
