#pragma once
#ifndef CHATSERVER_H
#define CHATSERVER_H

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include<functional>
#include<string>
using namespace std;
using namespace muduo;
using namespace muduo::net;

/*// 基于muduo网络库开发服务器程序
// 1.组合TcpServer对象
// 2.创建EventLoop事件循环对象的指针
// 3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.在构造函数中注册用户连接和读写事件的回调函数
5.设置合适的服务器端线程数量,muduo库会自己分配IO线程和工作线程
*/
//聊天服务器的主类
class ChatServer
{
public:
    ChatServer(EventLoop* loop,
               const InetAddress& listenAddr,//IP+port
               const string& nameArg);//服务器名字
       
    
    //开启事件循环
        void start();
    
private:
    //专门处理用户连接创建和断开
    void onConnection(const TcpConnectionPtr& conn);
   
    //专门处理用户读写事件
    void onMessage(const TcpConnectionPtr&conn,//连接
                   Buffer* buffer,//缓冲区
                   Timestamp time);//接收数据的时间信息
   
    TcpServer _server;//组合的muduo库的服务器对象
    EventLoop* _loop;//事件循环对象的指针
};
#endif //CHATSERVER_H