// commandHandler.cpp
#include "commandHandler.hpp"
#include <iostream>
#include <sstream>

CommandHandler::CommandHandler(Client *client) : client(client)
{
	commandMap["help"] = [this](const std::string &)
	{ showHelp(); };
	commandMap["login"] = [this](const std::string &args)
	{ handleLogin(args); };
	commandMap["chat"] = [this](const std::string &args)
	{ handleChat(args); };
	commandMap["pull"] = [this](const std::string &args)
	{ handlePull(args); };
}

// 命令格式：cmd:arg1:arg2:...
void CommandHandler::handleCommand(const std::string &line) {
    auto trim = [](std::string s){
        size_t i = s.find_first_not_of(" \t\r\n");
        if (i == std::string::npos) return std::string();
        size_t j = s.find_last_not_of(" \t\r\n");
        return s.substr(i, j - i + 1);
    };
    auto starts_with = [](const std::string& s, const char* p){
        return s.rfind(p, 0) == 0;
    };

    std::string s = trim(line);
    if (s.empty()) return;

    // 单词命令先处理
    if (s == "help") { showHelp(); return; }
    if (s == "pull") { client->pullOfflineMessages(); return; }
    if (s == "quit") {
        client->stop();
        std::cout << "再见 👋\n";
        client->gracefulExit(0);	//优雅的退出
    }

    // login:alice:123456 或 login alice 123456
    if (starts_with(s, "login:") || starts_with(s, "login ")) {
        std::string user, pwd;
        if (s[5] == ':') {
            size_t p1 = s.find(':', 6);
            if (p1 == std::string::npos) { std::cout << "用法：login:<user>:<pwd>\n"; return; }
            user = s.substr(6, p1 - 6);
            pwd  = s.substr(p1 + 1);
        } else {
            // login alice 123456
            std::istringstream iss(s);
            std::string cmd; iss >> cmd >> user >> pwd;
            if (user.empty() || pwd.empty()) { std::cout << "用法：login <user> <pwd>\n"; return; }
        }
        client->login(user, pwd);
        return;
    }

    // chat:@24:hello / chat:24:hello / chat @24 hello / chat 24 hello
    if (starts_with(s, "chat:") || starts_with(s, "chat ")) {
        std::string to, text;
        if (s[4] == ':') {
            size_t p1 = s.find(':', 5);
            if (p1 == std::string::npos) { std::cout << "用法：chat:<uid|@uid>:<text>\n"; return; }
            to   = s.substr(5, p1 - 5);
            text = s.substr(p1 + 1);
        } else {
            std::istringstream iss(s);
            std::string cmd; iss >> cmd >> to;
            std::getline(iss, text); // 其余全是文本
            if (!text.empty() && text[0] == ' ') text.erase(0,1);
        }
        if (!to.empty() && to[0] == '@') to.erase(0,1);
        if (to.empty() || text.empty()) { std::cout << "用法：chat:<uid|@uid>:<text>\n"; return; }
        client->sendMessage(to, text);
        return;
    }

    std::cout << "Invalid command. 输入 help 查看可用命令。\n";
}

void CommandHandler::showHelp()
{
	std::cout << "Commands:\n"
			  << "help                  : Show this help\n"
			  << "login:user:pwd        : Login\n"
			  << "chat:toUid:message    : Send message\n"
			  << "pull                  : Pull offline messages\n";
}

void CommandHandler::handleLogin(const std::string &args)
{
	size_t pos = args.find(':');
	if (pos == std::string::npos)
	{
		std::cerr << "Invalid login command format!" << std::endl;
		return;
	}
	std::string user = args.substr(0, pos);
	std::string pwd = args.substr(pos + 1);
	// 解析出命令参数，并调用 Client 的登录方法
	client->login(user, pwd);
}

void CommandHandler::handleChat(const std::string &args)
{
	size_t pos = args.find(':');
	if (pos == std::string::npos)
	{
		std::cerr << "Invalid chat command format!" << std::endl;
		return;
	}
	std::string toUid = args.substr(0, pos);
	std::string msg = args.substr(pos + 1);
	if (!toUid.empty() && toUid[0] == '@') toUid.erase(0,1);
    if (toUid.empty() || msg.empty()) {
        std::cerr << "usage：chat:<uid|@uid>:<text>\n";
        return;
    }
	client->sendMessage(toUid, msg);
}

void CommandHandler::handlePull(const std::string &)
{
	client->pullOfflineMessages();
}
