#include "Client.h"
#include <Ws2tcpip.h>

// Constructor: Initialize socket to INVALID_SOCKET and shutdown flag to false
Client::Client() :
	serverSocket(INVALID_SOCKET),
	isShuttingDown(false)
{
}

int Client::init(uint16_t port, char* address)
{
	// 1. Create the sockaddr structure first.
	// We need this structure to pass to inet_pton.
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	// 2. Convert the address string using the modern inet_pton function.
	// This function converts the string and checks its format in one step.
	int result = inet_pton(AF_INET, address, &serverAddr.sin_addr);

	if (result == 0)
	{
		// A result of 0 means the address string was not in a valid format.
		return ADDRESS_ERROR;
	}
	else if (result == -1)
	{
		// A result of -1 means a system error occurred (e.g., AF_INET not supported)
		return SETUP_ERROR;
	}
	// A result of 1 means success.

	// 3. Create a socket
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		return SETUP_ERROR;
	}

	// 4. Connect the socket to the server
	if (connect(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		closesocket(serverSocket); // (Using the fix from the previous step)
		return isShuttingDown ? SHUTDOWN : CONNECT_ERROR;
	}

	return SUCCESS;
}

int Client::readMessage(char* buffer, int32_t size)
{
	// First, read the 1-byte length prefix
	char lengthByte;
	int result = recvAll(&lengthByte, 1);

	// If recvAll failed, return its error code (SHUTDOWN or DISCONNECT)
	if (result != SUCCESS)
	{
		return result;
	}

	// Convert the length byte to an unsigned integer (0-255)
	uint8_t messageLength = (uint8_t)lengthByte;

	// Check if the message length exceeds the buffer capacity
	if (messageLength > size)
	{
		return PARAMETER_ERROR;
	}

	// If length is 0, we are done. Return success.
	if (messageLength == 0)
	{
		return SUCCESS;
	}

	// Now, read exactly 'messageLength' bytes into the buffer
	return recvAll(buffer, messageLength);
}

int Client::sendMessage(char* data, int32_t length)
{
	// Check for invalid length parameter
	if (length < 0 || length > 255)
	{
		return PARAMETER_ERROR;
	}

	// Create the 1-byte length prefix
	char lengthByte = (char)length;

	// First, send the length prefix
	int result = sendAll(&lengthByte, 1);
	if (result != SUCCESS)
	{
		return result; // Will be SHUTDOWN or DISCONNECT
	}

	// If the message has data (length > 0), send the actual data
	if (length > 0)
	{
		return sendAll(data, length);
	}

	return SUCCESS; // Success if length was 0
}

void Client::stop()
{
	// Set the shutdown flag to true
	isShuttingDown = true;

	// Shutdown and close the server connection socket
	if (serverSocket != INVALID_SOCKET)
	{
		shutdown(serverSocket, SD_BOTH); // Gracefully shut down send/receive
		closesocket(serverSocket);
		serverSocket = INVALID_SOCKET;
	}
}

// --- Private Helper Functions ---

// Helper to loop recv() until 'len' bytes are received
int Client::recvAll(char* buf, int len)
{
	int totalReceived = 0;
	while (totalReceived < len)
	{
		int bytesReceived = recv(serverSocket, buf + totalReceived, len - totalReceived, 0);

		if (bytesReceived == 0) // Graceful disconnect from the server
		{
			return isShuttingDown ? SHUTDOWN : DISCONNECT;
		}

		if (bytesReceived == SOCKET_ERROR)
		{
			// Check if the error was caused by our own 'stop()' call
			return isShuttingDown ? SHUTDOWN : DISCONNECT;
		}

		totalReceived += bytesReceived;
	}
	return SUCCESS;
}

// Helper to loop send() until 'len' bytes are sent
int Client::sendAll(char* data, int len)
{
	int totalSent = 0;
	while (totalSent < len)
	{
		int bytesSent = send(serverSocket, data + totalSent, len - totalSent, 0);

		if (bytesSent == SOCKET_ERROR)
		{
			// Check if the error was caused by our own 'stop()' call
			return isShuttingDown ? SHUTDOWN : DISCONNECT;
		}

		totalSent += bytesSent;
	}
	return SUCCESS;
}