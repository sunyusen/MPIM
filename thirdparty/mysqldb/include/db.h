#pragma once
#include <mysql/mysql.h>
#include <string>

// 数据库操作类
class MySQL {
public:
    //初始化数据库连接
    MySQL();
    // 释放数据库连接资源
    ~MySQL();
    // 连接数据库
    bool connect();
    // 更新操作
    bool update(std::string sql);
    //查询操作
    MYSQL_RES* query(std::string sql);
    // 获取连接
    MYSQL* getConnection();

	// 把转义封装到类里，外部不见MYSQL*
	// 返回转义后字符串长度
	unsigned long escape(const std::string& in, std::string& out);
private:
    MYSQL *_conn; //数据库连接句柄
};
