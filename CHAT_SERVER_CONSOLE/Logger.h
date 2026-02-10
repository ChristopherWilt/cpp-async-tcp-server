// Logger.h
#pragma once
#include <string>
#include <fstream>
#include <winsock2.h>

class Logger {
public:
    Logger(const std::string& commandLogFile, const std::string& publicLogFile);
    ~Logger();

    void logPublicMessage(const std::string& message);
    void logCommand(const std::string& command);
    void sendPublicLog(SOCKET clientSocket, class ChatServer& server);

private:
    std::ofstream commandLog;
    std::ofstream publicLog;
};

