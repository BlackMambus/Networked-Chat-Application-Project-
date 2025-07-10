// chat_server.cpp
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

std::vector<int> clients;
std::mutex clients_mutex;

void broadcast(const std::string& message, int sender_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int client : clients) {
        if (client != sender_fd) {
            send(client, message.c_str(), message.size(), 0);
        }
    }
}

void handle_client(int client_socket) {
    char buffer[1024];
    while (true) {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) break;
        std::string message(buffer, bytes_received);
        broadcast(message, client_socket);
    }

    close(client_socket);
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr = {AF_INET, htons(5555), INADDR_ANY};

    bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);
    std::cout << "🟢 Server started on port 5555\n";

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (sockaddr*)&client_addr, &client_len);

        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(client_socket);
        std::thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    return 0;
}
// chat_client.cpp
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void receive_messages(int socket_fd) {
    char buffer[1024];
    while (true) {
        ssize_t bytes = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        std::cout << "\r📨 " << std::string(buffer, bytes) << "\n> ";
        std::cout.flush();
    }
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr = {AF_INET, htons(5555)};
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "❌ Connection failed.\n";
        return 1;
    }

    std::cout << "✅ Connected to chat server.\n";
    std::thread(receive_messages, sock).detach();

    std::string message;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, message);
        if (message == "/quit") break;
        send(sock, message.c_str(), message.size(), 0);
    }

    close(sock);
    return 0;
}
