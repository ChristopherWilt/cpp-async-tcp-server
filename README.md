# Async TCP Chat Framework (C++)

## 📖 Project Overview
A custom-built, lightweight TCP networking framework written in C++. This project demonstrates low-level networking concepts by implementing a raw **Client-Server architecture** using Windows Sockets (Winsock2).

Unlike standard tutorials that use high-level wrappers, this project manages the raw byte streams, memory serialization, and socket handshakes manually to ensure maximum control over the data transmission.

**Core Technologies:**
* **Language:** C++ (Standard 11/14)
* **Networking:** `Winsock2` (Raw Sockets)
* **Protocol:** Custom TCP Binary Protocol (Length-Prefixed Packets)
* **Architecture:** Authoritative Server with Thin Clients

---

## 📂 Repository Structure

### 1. Server Core (`/CHAT_SERVER_CONSOLE`)
The brain of the operation. This console application acts as the authoritative host.
* **Listener Logic:** Binds to a specific port (default `31337`) and awaits incoming connections.
* **Socket Management:** Handles multiple concurrent client sockets using `select()` or blocking loops.
* **Command Parsing:** Interprets raw string commands from clients (e.g., `-login`, `-register`).
* **Broadcasting:** Routes messages to all connected clients or specific targets (Direct Messaging).

### 2. Client Application (`/Application`)
The user-facing client.
* **Connection Handling:** Establishes the TCP handshake with the server IP/Port.
* **Packet Serialization:** wraps user input into the custom binary packet format before sending.
* **State Management:** Handles local state (Logged In / Logged Out) based on Server responses.

---

## ⚙️ Features Implemented

### Networking & Protocol
* **Binary Header System:** Uses a 1-byte length prefix to handle TCP stream fragmentation, ensuring messages are read exactly as sent.
* **Robust Error Handling:** Detects disconnects (`FD_CLOSE`), connection resets, and invalid address formats.

### Interactive Command System
The server parses specific command strings to manage user sessions:
* `register <user> <pass>`: Creates a new user credential in memory.
* `login <user> <pass>`: Authenticates an existing session.
* `logout`: Gracefully terminates the session.
* `getlist`: Requests a list of all currently active users.
* `send <user> <msg>`: Private Direct Message routing.

---

## 💻 Code Highlight: Binary Packet Reading
One of the critical challenges in TCP is that data arrives as a "stream" of bytes, not distinct messages. I solved this by implementing a **Length-Prefix Protocol**.

```cpp
// Located in Client.cpp
int Client::readMessage(char* buffer, int32_t size)
{
    // 1. Read the Header (1 Byte) to know how much data is coming
    char lengthByte;
    int result = recvAll(&lengthByte, 1);

    if (result != SUCCESS) return result;

    uint8_t messageLength = (uint8_t)lengthByte;
    
    // 2. Read exactly 'messageLength' bytes into the buffer
    // This prevents "reading into" the next message in the stream.
    return recvAll(buffer, messageLength);
}
