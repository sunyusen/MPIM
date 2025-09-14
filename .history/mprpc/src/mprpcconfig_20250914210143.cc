#include "mprpcconfig.h"

#include <iostream>
#include <string>
#include "logger.h"

// 去掉字符串前后的空格
void MprpcConfig::Trim(std::string &src)
{
	// 去掉字符串前面多余的空格
	
	int idx = src.find_first_not_of(' ');
	if (idx != -1)
	{
		// 说明字符串前面有空格
		src = src.substr(idx, src.size() - idx);
	}
	// 去掉字符串后面多余的空格
	idx = src.find_last_not_of(' ');
	if (idx != -1)
	{
		// 说明字符串后面有空格
		src = src.substr(0, idx + 1);
	}
}

// 负责解析加载配置文件
void MprpcConfig::LoadConfigFile(const char *config_file)
{
	FILE *fp = fopen(config_file, "r");
	if (nullptr == fp)
	{
		LOG_ERROR << config_file << " is not exist!";
		exit(EXIT_FAILURE);
	}

	// 1,注释	2，正确的配置项		3，去掉开头多余的空格
	while (!feof(fp))
	{
		char buf[512] = {0};
		fgets(buf, sizeof(buf), fp);

		std::string read_buf(buf);

		Trim(read_buf);

		// 判断#的注释
		if (read_buf[0] == '#' || read_buf.empty())
		{
			continue;
		}

		// 判断配置项
		int idx = read_buf.find('=');
		if (idx == -1)
		{
			// 配置项不合法
			continue;
		}

		std::string key;
		std::string value;
		key = read_buf.substr(0, idx);
		Trim(key);
		int endidx = read_buf.find('\n', idx);
		endidx = endidx == std::string::npos ? read_buf.size() : endidx;
		value = read_buf.substr(idx + 1, endidx - idx-1);
		Trim(value);
		LOG_DEBUG << "Config key: " << key << " value: " << value;
		m_configMap.insert({key, value});
	}
}
// 查询配置项信息
std::string MprpcConfig::Load(const std::string &key)
{
	// m_configMap[key] 方式，当 key 不存在时，会自动插入一个默认值，所以不能使用这种方式
	auto it = m_configMap.find(key);
	if (it == m_configMap.end())
	{
		return "";
	}
	return it->second;
}