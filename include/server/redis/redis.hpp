#ifndef REDIS_H
#define REDIS_H

#include<hiredis/hiredis.h>
#include<thread>
#include<functional>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    //连接redis服务器
    bool connect();

    //向redis指定通道channel发布消息
    bool publish(int channel, string message);

    //向redis指定通道subscribe订阅消息
    bool subscribe(int channel);

    //向redis指定通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);

    //在独立线程中接收订阅的消息
    void observer_channel_message();

    //回调方法，当接收到订阅的消息后，调用该方法
    void init_notify_handler(function<void(int, string)> fn);

private:
    //hiredis同步上下文对象
    redisContext *_publish_context;

    //hiredis同步上下文对象
    redisContext *_subscribe_context;

    //回调操作，收到订阅的消息后，调用该函数
    function<void(int, string)> _notify_message_handler;

};

#endif