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
class ChatServer
{
public:
    ChatServer(EventLoop* loop,
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
        void start()
    {
        _server.start();
    }
private:
    //专门处理用户连接创建和断开
    void onConnection(const TcpConnectionPtr& conn)
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
    void onMessage(const TcpConnectionPtr&conn,//连接
                   Buffer* buffer,//缓冲区
                   Timestamp time)//接收数据的时间信息
    {
        string buf=buffer->retrieveAllAsString();
        cout<<"recv data:"<<buf<<" time:"<<time.toString()<<endl;
        conn->send(buf);
    }
    TcpServer _server;
    EventLoop* _loop;
};

int main()
{
    EventLoop loop;//创建事件循环对象
    InetAddress addr("127.0.0.1", 6000); 
    ChatServer server(&loop, addr, "ChatServer");//创建服务器对象
    server.start();//启动服务器
    loop.loop();//以阻塞方式等待新用户连接，已连接用户的读写事件
    return 0;
}