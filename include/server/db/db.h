// db.h
#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
using namespace std;

class MySQL // 将类名从 MYSQL 改为 MySQLConn
{
public:
    MySQL(); // 构造函数相应改名
    ~MySQL();
    bool connect();
    bool update(string sql);
    MYSQL_RES* query(string sql);
    //获取连接
    MYSQL *getConnection();
private:
    MYSQL* _conn; // 这个成员变量类型是MySQL库的MYSQL结构体，保留不变
};
#endif