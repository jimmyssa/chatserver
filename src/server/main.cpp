#include"chatserver.hpp"
#include<iostream>
#include"chatservice.hpp"
#include<signal.h>
using namespace std;


//处理服务器ctrl+c 结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}
int main()
{
    signal(SIGINT,resetHandler);

    EventLoop loop;//创建事件循环对象
    InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"ChatServer");//创建服务器对象

    server.start();//启动服务器
    loop.loop();//以阻塞方式等待新用户连接，已连接用户的读写事件

    return 0;
}