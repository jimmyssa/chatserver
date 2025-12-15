#include"chatserver.hpp"
#include<functional>
#include"json.hpp"
#include"chatservice.hpp"
using json=nlohmann::json;
ChatServer::ChatServer(EventLoop* loop,
    const InetAddress& listenAddr,//IP+port
    const string& nameArg)//服务器名字
    : _server(loop, listenAddr, nameArg),
        _loop(loop)
{
    //给服务器注册用户连接的创建和断开回调函数
    _server.setConnectionCallback(
        std::bind(&ChatServer::onConnection, this, placeholders::_1)
    );
    //给服务器注册用户读写事件回调函数
    _server.setMessageCallback(
        std::bind(&ChatServer::onMessage, this,
                    placeholders::_1,
                    placeholders::_2,
                    placeholders::_3)
    );
    //设置服务器端的线程数量,1个IO线程,3个工作线程
    _server.setThreadNum(4);
}
    //开启事件循环
void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    //客户端断开链接
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
    
}
    //专门处理用户读写事件
void ChatServer::onMessage(const TcpConnectionPtr&conn,//连接
                Buffer* buffer,//缓冲区
                Timestamp time)//接收数据的时间信息
{
    string buf=buffer->retrieveAllAsString();
    //数据的反序列化
    json js =json::parse(buf);
    // 达到的目的：完全解耦网络模块的代码和业务模块的代码
    //通过js["msgid"]获取=》业务handler=》conn js time
    auto msgHandler=ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn,js,time);
}
    
