#include "chatserver.hpp"
#include <iostream>
#include "chatservice.hpp"
#include <signal.h>
#include <cstdlib>  // 添加 atoi 函数支持
using namespace std;

// 处理服务器ctrl+c 结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char** argv)  // ✅ 添加命令行参数
{
    // ✅ 检查命令行参数
    if (argc < 3) {
        cerr << "使用方法: " << argv[0] << " <IP地址> <端口号>" << endl;
        cerr << "示例: " << argv[0] << " 127.0.0.1 6000" << endl;
        cerr << "示例: " << argv[0] << " 127.0.0.1 6002" << endl;
        return -1;
    }
    
    // ✅ 使用命令行参数
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    
    cout << "启动服务器: IP=" << ip << ", 端口=" << port << endl;
    
    signal(SIGINT, resetHandler);

    EventLoop loop;  // 创建事件循环对象
    InetAddress addr(ip, port);  // ✅ 使用命令行参数，而不是硬编码
    ChatServer server(&loop, addr, "ChatServer");  // 创建服务器对象

    server.start();  // 启动服务器
    loop.loop();     // 以阻塞方式等待新用户连接，已连接用户的读写事件

    return 0;
}