// MessageHandler.h
#pragma once
#include <winsock2.h>
#include <string>
#include <sstream> 

// Forward declarations to avoid circular includes
class ChatServer;
struct Client;

class MessageHandler {
public:
    void handle(SOCKET clientSocket, const std::string& message, ChatServer& server);

private:
    void handleCommand(SOCKET clientSocket, const std::string& command, std::stringstream& ss, ChatServer& server);
    void handlePublicMessage(SOCKET clientSocket, const std::string& message, ChatServer& server);
};
