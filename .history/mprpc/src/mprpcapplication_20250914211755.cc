#include "mprpcapplication.h"
#include "logger/logger.h"
#include <iostream>
#include <unistd.h>
#include <string>

MprpcConfig MprpcApplication::m_config;

void ShowArgsHelp()
{
	LOG_INFO << "format: command -i <configfile>";
}

void MprpcApplication::Init(int argc, char **argv)
{
	if (argc < 2)
	{
		ShowArgsHelp();
		exit(EXIT_FAILURE);
	}

	int c = 0;
	std::string config_file;
	while((c = getopt(argc, argv, "i:")) != -1)
	{
		switch (c)
		{
		case 'i':
			config_file = optarg;
			break;
		case '?':
			ShowArgsHelp();
			exit(EXIT_FAILURE);
		case ':':
			ShowArgsHelp();
			exit(EXIT_FAILURE);
		default:
			break;
		}
	}

	// 开始加载配置文件了 rpcserver_ip=		rpcserver_port  zookeeper_ip= 	zookepper_port=
	m_config.LoadConfigFile(config_file.c_str());

	// std::cout << "rpcserverip:" << m_config.Load("rpcserverip") << std::endl;
	// std::cout << "rpcserverport:" << m_config.Load("rpcserverport") << std::endl;
	// std::cout << "zookeeperip:" << m_config.Load("zookeeperip") << std::endl;
	// std::cout << "zookeeperport:" << m_config.Load("zookeeperport") << std::endl;
	// LOG_INFO("mprpcservice start up!");
	// LOG_INFO("rpcserverip:%s", m_config.Load("rpcserverip").c_str());
	// LOG_INFO("rpcserverport:%s", m_config.Load("rpcserverport").c_str());
	// LOG_INFO("zookeeperip:%s", m_config.Load("zookeeperip").c_str());
	// LOG_INFO("zookeeperport:%s", m_config.Load("zookeeperport").c_str());
}
MprpcApplication& MprpcApplication::GetInstance()
{
	static MprpcApplication app; // 单例模式
	return app;
}

MprpcConfig& MprpcApplication::GetConfig()
{
	return m_config;
}