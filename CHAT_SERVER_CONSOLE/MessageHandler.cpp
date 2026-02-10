// MessageHandler.cpp
#include "MessageHandler.h"
#include "ChatServer.h"
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iostream>

void MessageHandler::handle(SOCKET clientSocket, const std::string& message, ChatServer& server) {
    if (message.empty()) return;

    if (message[0] == server.getCommandChar()) {

		// Log the command with client info
        Client& client = server.getClient(clientSocket);
        std::string logEntry = "Socket " + std::to_string(clientSocket) + (client.isLoggedIn ? " (" + client.username + ")" : "") + ": " + message;
        server.getLogger().logCommand(logEntry);

		// Parse command and arguments
        std::stringstream ss(message.substr(1));
        std::string command;
        ss >> command;

        // Clean up command string
        command.erase(std::remove(command.begin(), command.end(), '\0'), command.end());
        size_t lastChar = command.find_last_not_of(" \r\n\t");
        if (std::string::npos != lastChar) {
            command = command.substr(0, lastChar + 1);
        }
        for (char& c : command) { c = tolower(c); }

        std::cout << "DEBUG: Parsed command: '" << command << "'" << std::endl;
        handleCommand(clientSocket, command, ss, server);
    }
    else {
        handlePublicMessage(clientSocket, message, server);
    }
}

void MessageHandler::handleCommand(SOCKET clientSocket, const std::string& command, std::stringstream& ss, ChatServer& server) {
    Client& client = server.getClient(clientSocket);

    if (command == "help") {
        std::string helpMsg1 = "--- Available Commands ---\n"
            "help | Shows all commands available\n"
            "register <username> <password> | Used to register an account\n"
            "login <username> <password> | Used to login to an account";
        std::string helpMsg2 = "logout | Used to logout of an account from the server\n"
            "getlist | Used to get a List of all active users\n"
            "getlog | Used to get the message log from the public server chat\n"
            "send <recipient> <message> | Used to Direct Message an Existing User";
        server.sendPacket(clientSocket, helpMsg1);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        server.sendPacket(clientSocket, helpMsg2);
    }
    else if (command == "register") {
        std::string username, password;
        ss >> username >> password;
        if (username.empty() || password.empty()) {
            server.sendPacket(clientSocket, "Usage: " + std::string(1, server.getCommandChar()) + "register <username> <password>");
        }
        else if (server.getCredentialStore().registerUser(username, password)) {
            std::cout << "New user registered: " << username << std::endl;
            server.sendPacket(clientSocket, "Success: Registration complete. You may now log in.");
        }
        else {
            server.sendPacket(clientSocket, "Failure: That username is already taken.");
        }
    }
    else if (command == "login") {
        if (client.isLoggedIn) {
            server.sendPacket(clientSocket, "Error: You are already logged in.");
            return;
        }
        std::string username, password;
        ss >> username >> password;
        if (username.empty() || password.empty()) {
            server.sendPacket(clientSocket, "Usage: " + std::string(1, server.getCommandChar()) + "login <username> <password>");
        }
        else if (!server.getCredentialStore().userExists(username)) {
            server.sendPacket(clientSocket, "Failure: User not found.");
        }
        else if (!server.getCredentialStore().authenticate(username, password)) {
            server.sendPacket(clientSocket, "Failure: Incorrect password.");
        }
        else {
            client.isLoggedIn = true;
            client.username = username;
            std::cout << "User '" << username << "' logged in on socket " << clientSocket << std::endl;
            server.sendPacket(clientSocket, "Success: You are now logged in.");
        }
    }
    else if (command == "send") {
        if (!client.isLoggedIn) {
            server.sendPacket(clientSocket, "Error: You must be logged in to send messages.");
            return;
        }
        std::string recipient, message;
        ss >> recipient;
        std::getline(ss, message);
        if (!message.empty() && message[0] == ' ') { message = message.substr(1); }

        SOCKET recipientSocket = server.findClientSocketByUsername(recipient);
        if (recipientSocket != INVALID_SOCKET) {
            std::string dm = "(DM from " + client.username + "): " + message;
            server.sendPacket(recipientSocket, dm);
        }
        else {
            server.sendPacket(clientSocket, "Error: User '" + recipient + "' not found or is not online.");
        }
    }
    else if (command == "getlist") {
        if (!client.isLoggedIn) {
            server.sendPacket(clientSocket, "Error: You must be logged in to use this command.");
            return;
        }
        server.sendPacket(clientSocket, server.getActiveUserList());
    }
    else if (command == "getlog") {
        if (!client.isLoggedIn) {
            server.sendPacket(clientSocket, "Error: You must be logged in to use this command.");
            return;
        }
        server.getLogger().sendPublicLog(clientSocket, server);
    }
    else if (command == "logout") {
        if (!client.isLoggedIn) {
            server.sendPacket(clientSocket, "Error: You are not logged in.");
        }
        else {
            std::cout << "User '" << client.username << "' logged out." << std::endl;
            server.sendPacket(clientSocket, "You have been logged out.");
            server.disconnectClient(clientSocket);
        }
    }
    else {
        server.sendPacket(clientSocket, "Error: Unknown command. Type " + std::string(1, server.getCommandChar()) + "help for a list of commands.");
    }
}

void MessageHandler::handlePublicMessage(SOCKET clientSocket, const std::string& message, ChatServer& server) {
    Client& client = server.getClient(clientSocket);
    if (!client.isLoggedIn) {
        server.sendPacket(clientSocket, "Error: You must be logged in to send messages.");
    }
    else {
        std::string broadcastMsg = client.username + ": " + message;
        server.broadcastMessage(clientSocket, broadcastMsg);
    }
}