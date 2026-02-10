// Client.h
#pragma once
#include <string>
#include <winsock2.h>

// Holds state information for each connected client.
struct Client {
    std::string username;
    bool isLoggedIn = false;

    // Constructor to initialize a client with its socket
    Client() : isLoggedIn(false) {}
};