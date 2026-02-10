#include "Server.h"
#include <Ws2tcpip.h>


Server::Server() :
	listenSocket(INVALID_SOCKET),
	clientSocket(INVALID_SOCKET),
	isShuttingDown(false)
{}

int Server::init(uint16_t port)
{

	// Create a socket for listening
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		return SETUP_ERROR;
	}
	char reuseAddr = 1;
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));
	// Bind the socket to the specified port
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces

	if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closesocket(listenSocket);
		return BIND_ERROR;
	}

	// Start listening queue for incoming connections
	if (listen(listenSocket, 1) == SOCKET_ERROR) {
		closesocket(listenSocket);
		return SETUP_ERROR;
	}

	// Wait for a client to connect
	clientSocket = accept(listenSocket, NULL, NULL);

	// Close the listening socket as we only handle one client at a time
	closesocket(listenSocket);
	listenSocket = INVALID_SOCKET; // Marks as closed!

	if (clientSocket == INVALID_SOCKET) {
		// Checks if we are shutting down or if there was an error
		return isShuttingDown ? SHUTDOWN : CONNECT_ERROR;
	}

	// If we reach here, the connection was successful
	return SUCCESS;
}

int Server::readMessage(char* buffer, int32_t size)
{
	// Read the 1-byte length prefix
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

int Server::sendMessage(char* data, int32_t length)
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

void Server::stop()
{
	// Set the shutdown flag to true
	// This tells our blocking functions (recv, accept) that the error is intentional
	isShuttingDown = true;

	// Close the listening socket if it's still open (e.g., if stop is called before accept)
	if (listenSocket != INVALID_SOCKET)
	{
		closesocket(listenSocket);
		listenSocket = INVALID_SOCKET;
	}

	// Shutdown and close the client connection socket
	if (clientSocket != INVALID_SOCKET)
	{
		shutdown(clientSocket, SD_BOTH); // Gracefully shut down send/receive
		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
	}
}

// Helper to loop recv() until 'len' bytes are received
int Server::recvAll(char* buf, int len)
{
	int totalReceived = 0;
	while (totalReceived < len)
	{
		int bytesReceived = recv(clientSocket, buf + totalReceived, len - totalReceived, 0);

		if (bytesReceived == 0) // Graceful disconnect from the client
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
int Server::sendAll(char* data, int len)
{
	int totalSent = 0;
	while (totalSent < len)
	{
		int bytesSent = send(clientSocket, data + totalSent, len - totalSent, 0);

		if (bytesSent == SOCKET_ERROR)
		{
			// Check if the error was caused by our own 'stop()' call
			return isShuttingDown ? SHUTDOWN : DISCONNECT;
		}

		totalSent += bytesSent;
	}
	return SUCCESS;
}
