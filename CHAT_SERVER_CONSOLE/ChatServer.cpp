// ChatServer.cpp
#include "ChatServer.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <ws2tcpip.h>

// --- CONSTRUCTOR & DESTRUCTOR ---

ChatServer::ChatServer(int port, int capacity, char commandChar)
    : port(port), capacity(capacity), commandChar(commandChar), listenSocket(INVALID_SOCKET),
    logger("commands.log", "public_messages.log") {

    initializeWinsock();
    displayServerInfo();
    createListenSocket();
    bindAndListen();

    FD_ZERO(&master_set);
    FD_SET(listenSocket, &master_set);

    std::cout << "Server configured with port " << port << ", capacity " << capacity << ", and command character '" << commandChar << "'" << std::endl;
    
    broadcaster = std::make_unique<Broadcaster>(port);
    broadcaster->start();
}

ChatServer::~ChatServer() {

    broadcaster->stop();

    for (u_int i = 0; i < master_set.fd_count; i++) {
        closesocket(master_set.fd_array[i]);
    }
    WSACleanup();
}

// --- SERVER CORE ---

void ChatServer::run() {
    std::cout << "Server is listening on port " << port << "..." << std::endl;

    while (true) {
        fd_set read_set = master_set;
        if (select(0, &read_set, NULL, NULL, NULL) == SOCKET_ERROR) {
            std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
            break;
        }

        if (FD_ISSET(listenSocket, &read_set)) {
            acceptNewConnection();
        }

        for (u_int i = 0; i < master_set.fd_count; i++) {
            SOCKET clientSocket = master_set.fd_array[i];
            if (clientSocket != listenSocket && FD_ISSET(clientSocket, &read_set)) {
                handleClientData(clientSocket);
            }
        }
    }
}

void ChatServer::initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed.");
    }
}

void ChatServer::createListenSocket() {
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Socket creation failed.");
    }
}

void ChatServer::bindAndListen() {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(listenSocket);
        WSACleanup();
        throw std::runtime_error("Bind failed.");
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSocket);
        WSACleanup();
        throw std::runtime_error("Listen failed.");
    }
}

void ChatServer::acceptNewConnection() {
    SOCKET newClientSocket = accept(listenSocket, nullptr, nullptr);
    if (newClientSocket == INVALID_SOCKET) {
        return; // No new connection
    }

    if (clients.size() >= capacity) {
        std::string fullMsg = "Server is at full capacity. Try again later.";
        sendPacket(newClientSocket, fullMsg);
        closesocket(newClientSocket);
        std::cout << "Rejected new client: server at capacity." << std::endl;
    }
    else {
        FD_SET(newClientSocket, &master_set);
        clients[newClientSocket] = Client(); // Add new client to the map
        std::cout << "New client connected on socket " << newClientSocket << std::endl;
        std::string welcomeMsg = "Welcome! Command character is " + std::string(1, commandChar);
        sendPacket(newClientSocket, welcomeMsg);
    }
}

void ChatServer::handleClientData(SOCKET clientSocket) {
    bool isConnected = true;
    std::string receivedMsg = receivePacket(clientSocket, isConnected);

    if (!isConnected) {
        disconnectClient(clientSocket);
    }
    else {
        std::cout << "Received from socket " << clientSocket << ": " << receivedMsg << std::endl;
        messageHandler.handle(clientSocket, receivedMsg, *this);
    }
}

void ChatServer::displayServerInfo()
{
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cerr << "Error getting hostname: " << WSAGetLastError() << std::endl;
        return;
    }
    std::cout << "\n--- Server Host Information ---" << std::endl;
    std::cout << "Hostname: " << hostname << std::endl;

    addrinfo hints = { 0 }, * result;
    hints.ai_family = AF_UNSPEC; // Get both IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, NULL, &hints, &result) != 0) {
        std::cerr << "getaddrinfo failed." << std::endl;
        return;
    }

    for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        char ipStr[INET6_ADDRSTRLEN];
        if (ptr->ai_family == AF_INET) { // IPv4
            sockaddr_in* sockaddr_ipv4 = (sockaddr_in*)ptr->ai_addr;
            inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ipStr, sizeof(ipStr));
            std::cout << "IPv4 Address: " << ipStr << std::endl;
        }
        else if (ptr->ai_family == AF_INET6) { // IPv6
            sockaddr_in6* sockaddr_ipv6 = (sockaddr_in6*)ptr->ai_addr;
            inet_ntop(AF_INET6, &sockaddr_ipv6->sin6_addr, ipStr, sizeof(ipStr));
            std::cout << "IPv6 Address: " << ipStr << std::endl;
        }
    }
    std::cout << "-----------------------------" << std::endl;
    freeaddrinfo(result);
}

void ChatServer::disconnectClient(SOCKET clientSocket) {
    if (clients.count(clientSocket)) {
        std::cout << "Client on socket " << clientSocket << " (" << clients[clientSocket].username << ") disconnected." << std::endl;
        clients.erase(clientSocket);
    }
    closesocket(clientSocket);
    FD_CLR(clientSocket, &master_set);
}

// --- PUBLIC INTERFACE & HELPERS ---

void ChatServer::broadcastMessage(SOCKET senderSocket, const std::string& message) {
    logger.logPublicMessage(message);
    std::cout << "Broadcasting: " << message << std::endl;

    for (auto const& [sock, client] : clients) {
        if (client.isLoggedIn && sock != senderSocket) {
            sendPacket(sock, message);
        }
    }
}

Client& ChatServer::getClient(SOCKET clientSocket) {
    return clients.at(clientSocket);
}

CredentialStore& ChatServer::getCredentialStore() {
    return credentialStore;
}

Logger& ChatServer::getLogger() {
    return logger;
}

char ChatServer::getCommandChar() const {
    return commandChar;
}

std::string ChatServer::getActiveUserList() {
    std::string userList = "Active Users:\n";
    for (auto const& [sock, info] : clients) {
        if (info.isLoggedIn) {
            userList += "- " + info.username + "\n";
        }
    }
    return userList;
}

SOCKET ChatServer::findClientSocketByUsername(const std::string& username) {
    for (auto const& [sock, info] : clients) {
        if (info.isLoggedIn && info.username == username) {
            return sock;
        }
    }
    return INVALID_SOCKET;
}


// --- PACKET PROTOCOL IMPLEMENTATION ---

bool ChatServer::sendPacket(SOCKET sock, const std::string& message) {
    std::cout << "--> Sending to socket " << sock << ": \"" << message << "\"" << std::endl;

    if (message.length() > 255) {
        std::cerr << "Message too long to send (max 255 bytes)." << std::endl;
        return false;
    }

    int packetSize = 1 + message.length();
    char* packet = new char[packetSize];

    packet[0] = static_cast<char>(message.length());
    memcpy(packet + 1, message.c_str(), message.length());

    bool result = sendAll(sock, packet, packetSize);
    delete[] packet;
    return result;
}

std::string ChatServer::receivePacket(SOCKET sock, bool& isConnected) {
    char lengthByte;
    if (!recvAll(sock, &lengthByte, 1)) {
        isConnected = false;
        return "";
    }

    int messageLength = static_cast<unsigned char>(lengthByte);
    if (messageLength == 0) {
        isConnected = true;
        return "";
    }

    char* buffer = new char[messageLength];
    if (!recvAll(sock, buffer, messageLength)) {
        isConnected = false;
        delete[] buffer;
        return "";
    }

    std::string message(buffer, messageLength);
    delete[] buffer;
    isConnected = true;
    return message;
}

bool ChatServer::sendAll(SOCKET sock, const char* buffer, int len) {
    int totalSent = 0;
    while (totalSent < len) {
        int sent = send(sock, buffer + totalSent, len - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            return false;
        }
        totalSent += sent;
    }
    return true;
}

bool ChatServer::recvAll(SOCKET sock, char* buffer, int len) {
    int totalReceived = 0;
    while (totalReceived < len) {
        int received = recv(sock, buffer + totalReceived, len - totalReceived, 0);
        if (received == 0 || received == SOCKET_ERROR) {
            return false;
        }
        totalReceived += received;
    }
    return true;
}