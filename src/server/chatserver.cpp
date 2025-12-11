#include"chatserver.hpp"
#include<functional>


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
    if(conn->connected())//新连接
    {
        cout << conn->peerAddress().toIpPort() << " -> "
                << conn->localAddress().toIpPort() << " state:online"
                << endl;
    }
    else//断开连接
    {
        cout << conn->peerAddress().toIpPort() << " -> "
                << conn->localAddress().toIpPort() << " state:offline"
                << endl;
        conn->shutdown();//关闭连接
    }
}
    //专门处理用户读写事件
void ChatServer::onMessage(const TcpConnectionPtr&conn,//连接
                Buffer* buffer,//缓冲区
                Timestamp time)//接收数据的时间信息
{
    string buf=buffer->retrieveAllAsString();
    cout<<"recv data:"<<buf<<" time:"<<time.toString()<<endl;
    conn->send(buf);
}
    
