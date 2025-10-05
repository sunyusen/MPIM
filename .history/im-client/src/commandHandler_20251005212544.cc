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
	commandMap["register"] = [this](const std::string &args)
	{ handleRegister(args); };
	commandMap["chat"] = [this](const std::string &args)
	{ handleChat(args); };
	commandMap["pull"] = [this](const std::string &args)
	{ handlePull(args); };
	commandMap["logout"] = [this](const std::string &args)
	{ handleLogout(args); };
	commandMap["addfriend"] = [this](const std::string &args)
	{ handleAddFriend(args); };
	commandMap["getfriends"] = [this](const std::string &args)
	{ handleGetFriends(args); };
	commandMap["creategroup"] = [this](const std::string &args)
	{ handleCreateGroup(args); };
	commandMap["joingroup"] = [this](const std::string &args)
	{ handleJoinGroup(args); };
	commandMap["sendgroup"] = [this](const std::string &args)
	{ handleSendGroup(args); };
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
    if (s == "logout") { client->logout(); return; }
    if (s == "getfriends") { client->getFriends(); return; }
    if (s == "quit") {
        client->stop();
        std::cout << "再见 👋\n";
        client->gracefulExit(0);	//优雅的退出
    }

    // register:alice:123456 或 register alice 123456
    if (starts_with(s, "register:") || starts_with(s, "register ")) {
        std::string user, pwd;
        if (s[8] == ':') {
            size_t p1 = s.find(':', 9);
            if (p1 == std::string::npos) { std::cout << "用法：register:<user>:<pwd>\n"; return; }
            user = s.substr(9, p1 - 9);
            pwd  = s.substr(p1 + 1);
        } else {
            std::istringstream iss(s);
            std::string cmd; iss >> cmd >> user >> pwd;
            if (user.empty() || pwd.empty()) { std::cout << "用法：register <user> <pwd>\n"; return; }
        }
        // 直接发送 REGISTER 行协议
        client->enqueueRaw("REGISTER " + user + " " + pwd);
        return;
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

    // addfriend:123 或 addfriend 123
    if (starts_with(s, "addfriend:") || starts_with(s, "addfriend ")) {
        std::string friendId;
        if (s[9] == ':') {
            friendId = s.substr(10);
        } else {
            std::istringstream iss(s);
            std::string cmd; iss >> cmd >> friendId;
        }
        if (friendId.empty()) { std::cout << "用法：addfriend:<friend_id>\n"; return; }
        client->addFriend(friendId);
        return;
    }

    // creategroup:name:desc 或 creategroup name desc
    if (starts_with(s, "creategroup:") || starts_with(s, "creategroup ")) {
        std::string name, desc;
        if (s[11] == ':') {
            size_t p1 = s.find(':', 12);
            if (p1 == std::string::npos) { std::cout << "用法：creategroup:<name>:<desc>\n"; return; }
            name = s.substr(12, p1 - 12);
            desc = s.substr(p1 + 1);
        } else {
            std::istringstream iss(s);
            std::string cmd; iss >> cmd >> name;
            std::getline(iss, desc);
            if (!desc.empty() && desc[0] == ' ') desc.erase(0,1);
        }
        if (name.empty()) { std::cout << "用法：creategroup:<name>:<desc>\n"; return; }
        client->createGroup(name, desc);
        return;
    }

    // joingroup:123 或 joingroup 123
    if (starts_with(s, "joingroup:") || starts_with(s, "joingroup ")) {
        std::string groupId;
        if (s[9] == ':') {
            groupId = s.substr(10);
        } else {
            std::istringstream iss(s);
            std::string cmd; iss >> cmd >> groupId;
        }
        if (groupId.empty()) { std::cout << "用法：joingroup:<group_id>\n"; return; }
        client->joinGroup(groupId);
        return;
    }

    // sendgroup:123:hello 或 sendgroup 123 hello
    if (starts_with(s, "sendgroup:") || starts_with(s, "sendgroup ")) {
        std::string groupId, message;
        if (s[9] == ':') {
            size_t p1 = s.find(':', 10);
            if (p1 == std::string::npos) { std::cout << "用法：sendgroup:<group_id>:<message>\n"; return; }
            groupId = s.substr(10, p1 - 10);
            message = s.substr(p1 + 1);
        } else {
            std::istringstream iss(s);
            std::string cmd; iss >> cmd >> groupId;
            std::getline(iss, message);
            if (!message.empty() && message[0] == ' ') message.erase(0,1);
        }
        if (groupId.empty() || message.empty()) { std::cout << "用法：sendgroup:<group_id>:<message>\n"; return; }
        client->sendGroupMessage(groupId, message);
        return;
    }

    std::cout << "Invalid command. 输入 help 查看可用命令。\n";
}

void CommandHandler::showHelp()
{
	std::cout << "Commands:\n"
			  << "help                  : Show this help\n"
			  << "register:user:pwd     : Register new user\n"
			  << "login:user:pwd        : Login\n"
			  << "logout                : Logout\n"
			  << "chat:toUid:message    : Send message\n"
			  << "pull                  : Pull offline messages\n"
			  << "addfriend:friendId    : Add friend\n"
			  << "getfriends            : Get friends list\n"
			  << "creategroup:name:desc : Create group\n"
			  << "joingroup:groupId     : Join group\n"
			  << "sendgroup:groupId:msg : Send group message\n";
}

void CommandHandler::handleRegister(const std::string &args)
{
	size_t pos = args.find(':');
	if (pos == std::string::npos)
	{
		std::cerr << "Invalid register command format!" << std::endl;
		return;
	}
	std::string user = args.substr(0, pos);
	std::string pwd = args.substr(pos + 1);
	client->enqueueRaw("REGISTER " + user + " " + pwd);
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

void CommandHandler::handleLogout(const std::string &)
{
	client->logout();
}

void CommandHandler::handleAddFriend(const std::string &args)
{
	client->addFriend(args);
}

void CommandHandler::handleGetFriends(const std::string &)
{
	client->getFriends();
}

void CommandHandler::handleCreateGroup(const std::string &args)
{
	size_t pos = args.find(':');
	if (pos == std::string::npos)
	{
		std::cerr << "Invalid creategroup command format!" << std::endl;
		return;
	}
	std::string name = args.substr(0, pos);
	std::string desc = args.substr(pos + 1);
	client->createGroup(name, desc);
}

void CommandHandler::handleJoinGroup(const std::string &args)
{
	client->joinGroup(args);
}

void CommandHandler::handleSendGroup(const std::string &args)
{
	size_t pos = args.find(':');
	if (pos == std::string::npos)
	{
		std::cerr << "Invalid sendgroup command format!" << std::endl;
		return;
	}
	std::string groupId = args.substr(0, pos);
	std::string message = args.substr(pos + 1);
	client->sendGroupMessage(groupId, message);
}
