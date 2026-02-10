// CredentialStore.h
#pragma once
#include <string>
#include <unordered_map>

class CredentialStore {
public:
    bool userExists(const std::string& username);
    bool registerUser(const std::string& username, const std::string& password);
    bool authenticate(const std::string& username, const std::string& password);

private:
    std::unordered_map<std::string, std::string> userCredentials;
};