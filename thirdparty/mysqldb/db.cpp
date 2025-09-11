#include "db.h"
#include <string>
#include<mysql/mysql.h>
#include <muduo/base/Logging.h>

using namespace std;
// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

// 初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn);
    }
}
// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        // C和C++代码默认的编码是ASCII, 如果不设置， 从MYSQL上拉下来的中文显示？
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!";
    }
    else{
        LOG_INFO << "connect mysql fail!";
    }

    // // 设置字符集为 utf8mb4
    // if (mysql_query(_conn, "set names utf8mb4"))
    // {
    //     LOG_INFO << "Failed to set character set: " << mysql_error(_conn);
    //     return false;
    // }

    return p;
}
// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "更新失败!错误原因："<< mysql_error(_conn);
        return false;
    }
    return true;
}
// 查询操作
MYSQL_RES* MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

// 获取连接
unsigned long MySQL::escape(const std::string& in, std::string& out){
	if(!_conn) return 0;
	out.reserve(in.size()*2+1);
	unsigned long n = mysql_real_escape_string(_conn,
											   &out[0],
											   in.data(),
											   static_cast<unsigned long>(in.size()));
	out.resize(n);
	return n;
}