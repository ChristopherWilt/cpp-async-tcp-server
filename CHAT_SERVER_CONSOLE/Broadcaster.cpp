// Broadcaster.cpp
#include "Broadcaster.h"
#include <ws2tcpip.h>
#include <iostream>
#include <chrono>

Broadcaster::Broadcaster(int tcpPort)
    : tcpPort(tcpPort), broadcastSocket(INVALID_SOCKET), isRunning(false) {
}

Broadcaster::~Broadcaster()
{
    stop();
    if (broadcastSocket != INVALID_SOCKET) {
        closesocket(broadcastSocket);
    }
}

void Broadcaster::start()
{
	if (isRunning) return;

	// Create UDP socket
	broadcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (broadcastSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create broadcast socket. Error: " << WSAGetLastError() << std::endl;
        return;
	}

	// Set socket options to enable broadcast
    char broadcastOption = '1';
    if (setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, &broadcastOption, sizeof(broadcastOption)) == SOCKET_ERROR) {
        std::cerr << "Failed to set socket options. Error: " << WSAGetLastError() << std::endl;
        closesocket(broadcastSocket);
        broadcastSocket = INVALID_SOCKET;
        return;
	}

    isRunning = true;
    broadcastThread = std::thread(&Broadcaster::broadcastLoop, this);
    std::cout << "UDP broadcaster started. Sending to port 31337." << std::endl;
}

void Broadcaster::stop()
{
    isRunning = false;
    if (broadcastThread.joinable()) {
        broadcastThread.join();
    }
}

void Broadcaster::broadcastLoop()
{
    // Construct broadcast address structure
    sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(31337); // The port SpaghettiRelay listens on for broadcasts
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    // Compose the broadcast message
    std::string message = "127.0.0.1:" + std::to_string(tcpPort);

    while (isRunning) {
        // Regularly dispatch the message
        if (sendto(broadcastSocket, message.c_str(), message.length(), 0, (sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) == SOCKET_ERROR) {
            std::cerr << "sendto failed: " << WSAGetLastError() << std::endl;
        }

        // Wait for a few seconds before sending the next broadcast
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}
