// Logger.cpp
#include "Logger.h"
#include "ChatServer.h" // For sendPacket
#include <iostream>

Logger::Logger(const std::string& commandLogFile, const std::string& publicLogFile) {
    commandLog.open(commandLogFile, std::ios::app);
    publicLog.open(publicLogFile, std::ios::app);

    if (!commandLog.is_open() || !publicLog.is_open()) {
        std::cerr << "Error: Could not open log files!" << std::endl;
    }
}

Logger::~Logger() {
    if (commandLog.is_open()) {
        commandLog.close();
    }
    if (publicLog.is_open()) {
        publicLog.close();
    }
}

void Logger::logPublicMessage(const std::string& message) {
    if (publicLog.is_open()) {
        publicLog << message << std::endl;
        publicLog.flush();
    }
}

void Logger::logCommand(const std::string& command)
{
    if (commandLog.is_open()) {
        commandLog << command << std::endl;
        commandLog.flush();
    }
}

void Logger::sendPublicLog(SOCKET clientSocket, ChatServer& server) {
    server.sendPacket(clientSocket, "--- Public Log ---");
    std::ifstream logFile("public_messages.log");
    if (logFile) {
        std::string line;
        while (std::getline(logFile, line)) {
            server.sendPacket(clientSocket, line);
        }
        logFile.close();
    }
    else {
        server.sendPacket(clientSocket, "No public log found.");
    }
}