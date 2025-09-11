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
    void handleLogin(const std::string& args);
    void handleChat(const std::string& args);
    void handlePull(const std::string& args);

    std::unordered_map<std::string, std::function<void(const std::string&)>> commandMap;
};
