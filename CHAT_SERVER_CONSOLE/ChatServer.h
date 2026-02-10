/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
------------------------- Created by Christopher Wilt -------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/

// ChatServer.h
#pragma once
#include <winsock2.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Client.h"
#include "Logger.h"
#include "CredentialStore.h"
#include "MessageHandler.h"
#include "Broadcaster.h"

#pragma comment(lib, "ws2_32.lib")

class ChatServer {
public:
    ChatServer(int port, int capacity, char commandChar);
    ~ChatServer();
    void run();

    // --- Public Interface for MessageHandler and other components ---
    bool sendPacket(SOCKET sock, const std::string& message);
    void broadcastMessage(SOCKET senderSocket, const std::string& message);
    void disconnectClient(SOCKET clientSocket);

    // --- Getters ---
    Client& getClient(SOCKET clientSocket);
    CredentialStore& getCredentialStore();
    Logger& getLogger();
    char getCommandChar() const;
    std::string getActiveUserList();
    SOCKET findClientSocketByUsername(const std::string& username);


private:
    // --- Server Core ---
    void initializeWinsock();
    void createListenSocket();
    void bindAndListen();
    void acceptNewConnection();
    void handleClientData(SOCKET clientSocket);
    void displayServerInfo();

    // --- Packet Protocol ---
    std::string receivePacket(SOCKET sock, bool& isConnected);
    bool sendAll(SOCKET sock, const char* buffer, int len);
    bool recvAll(SOCKET sock, char* buffer, int len);

    // --- Member Variables ---
    int port;
    int capacity;
    char commandChar;
    SOCKET listenSocket;

    fd_set master_set;
    std::map<SOCKET, Client> clients;

    // --- Component Objects ---
    CredentialStore credentialStore;
    MessageHandler messageHandler;
    Logger logger;
    std::unique_ptr<Broadcaster> broadcaster;
};