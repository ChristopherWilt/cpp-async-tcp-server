/*
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
------------------------- Created by Christopher Wilt -------------------------
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
*/

// main.cpp
#include <iostream>
#include "ChatServer.h"

int main() {
    // --- Server Configuration ---
    int port;
    int capacity;
    char commandChar;

    std::cout << "\n---------------------------------------------------\n";
    std::cout << "--------------- Chat Server Console ---------------\n";
    std::cout << "--------------- By Christopher Wilt ---------------";
    std::cout << "\n---------------------------------------------------\n\n";

    std::cout << " --- 1. SERVER CONFIGURATION ---\n";
    std::cout << "Enter port to listen on: ";
    std::cin >> port;
    std::cout << "Enter maximum number of clients: ";
    std::cin >> capacity;
    std::cout << "Enter the command character (e.g., ~): ";
    std::cin >> commandChar;

    // --- Start Server ---
    try {
        ChatServer server(port, capacity, commandChar);
        server.run();
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Fatal Server Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}