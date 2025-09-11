#pragma once

#include "mprpcconfig.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

// mprpc框架的基础类
class MprpcApplication
{
public:
	static void Init(int argc, char **argv);
	static MprpcApplication &GetInstance();
	static MprpcConfig& GetConfig();
private:
	MprpcApplication() = default;
	MprpcApplication(const MprpcApplication &) = delete;
	MprpcApplication(MprpcApplication &&) = delete;
private:
	static MprpcConfig m_config;
};