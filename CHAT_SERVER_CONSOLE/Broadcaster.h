// Broadcaster.h
#pragma once
#include <winsock2.h>
#include <string>
#include <thread>
#include <atomic>

class Broadcaster {
public:
    Broadcaster(int tcpPort);
    ~Broadcaster();
    void start();
    void stop();

private:
    void broadcastLoop();

    SOCKET broadcastSocket;
    int tcpPort;
    std::thread broadcastThread;
    std::atomic<bool> isRunning;
};
