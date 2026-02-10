#pragma once
#include "../platform.h"
#include "../definitions.h"

class Client
{
public:
	// Constructor to initialize variables
	Client();

	int init(uint16_t port, char* address);
	int readMessage(char* buffer, int32_t size);
	int sendMessage(char* data, int32_t length);
	void stop();

private:
	SOCKET serverSocket;
	bool isShuttingDown;

	// Helper functions for full message transmission/receipt
	int recvAll(char* buf, int len);
	int sendAll(char* data, int len);
};