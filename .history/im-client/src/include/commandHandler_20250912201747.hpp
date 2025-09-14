// CommandHandler.hpp
#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include "client.hpp"

class CommandHandler {
public:
    CommandHandler(Client* client);
    void handleCommand(const std::string& command);

private:
    Client* client;
    void showHelp();
    void handleRegister(const std::string& args);
    void handleLogin(const std::string& args);
    void handleChat(const std::string& args);
    void handlePull(const std::string& args);
    void handleLogout(const std::string& args);
    void handleAddFriend(const std::string& args);
    void handleGetFriends(const std::string& args);
    void handleCreateGroup(const std::string& args);
    void handleJoinGroup(const std::string& args);
    void handleSendGroup(const std::string& args);

    std::unordered_map<std::string, std::function<void(const std::string&)>> commandMap;
};
