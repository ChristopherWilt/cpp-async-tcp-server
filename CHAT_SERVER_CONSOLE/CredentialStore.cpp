// CredentialStore.cpp
#include "CredentialStore.h"

bool CredentialStore::userExists(const std::string& username) {
    return userCredentials.count(username) > 0;
}

bool CredentialStore::registerUser(const std::string& username, const std::string& password) {
    if (userExists(username)) {
        return false; // User already exists
    }
    userCredentials[username] = password;
    return true;
}

bool CredentialStore::authenticate(const std::string& username, const std::string& password) {
    if (!userExists(username)) {
        return false; // User not found
    }
    return userCredentials[username] == password;
}