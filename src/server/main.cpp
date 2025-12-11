#include"chatserver.hpp"
#include<iostream>
using namespace std;

int main()
{
    EventLoop loop;//创建事件循环对象
    InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"ChatServer");//创建服务器对象

    server.start();//启动服务器
    loop.loop();//以阻塞方式等待新用户连接，已连接用户的读写事件

    return 0;
}