#include"db.h"
#include<muduo/base/Logging.h>

// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "20020108";
static string dbname = "chat";


MySQL::MySQL() // 构造函数名更新
{
    _conn = mysql_init(nullptr);
}

MySQL::~MySQL() // 析构函数名更新
{
    if (_conn != nullptr)
        mysql_close(_conn);
}

bool MySQL::connect() 
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
    password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        mysql_query(_conn, "set names gbk");
        LOG_INFO<<"connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql fail! Error: " << mysql_error(_conn);
        LOG_INFO << "Connection params - Server: " << server 
                 << ", User: " << user 
                 << ", DB: " << dbname;
    }
    return p;
}

bool MySQL::update(string sql) // 更新作用域
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
        << sql << "更新失败!";
        return false;
    }
    return true;
}

MYSQL_RES* MySQL::query(string sql) // 更新作用域
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
        << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

MYSQL* MySQL::getConnection()
{
    return _conn;
}