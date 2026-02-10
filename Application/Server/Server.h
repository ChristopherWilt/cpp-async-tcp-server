#pragma once
#include "../platform.h"
#include "../definitions.h"

class Server
{
public:
	Server();

	int init(uint16_t port);
	int readMessage(char* buffer, int32_t size);
	int sendMessage(char* data, int32_t length);
	void stop();

private:
	SOCKET listenSocket;
	SOCKET clientSocket;
	bool isShuttingDown;

	// Helper functions
	int recvAll(char* buffer, int len);
	int sendAll(char* data, int len);
};