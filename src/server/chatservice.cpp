// 首先包含系统头文件
#include <muduo/base/Logging.h>
#include <string>

// 然后包含项目头文件
#include "chatservice.hpp"
#include "public.hpp"

// 使用命名空间（放在所有头文件包含之后）
using namespace std::placeholders;
using namespace muduo;
using namespace muduo::net;

//获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service; 
}

//注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    
    if (it == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        return it->second;
    }
}

//处理登录业务 ORM 业务层操作的都是对象
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id =js["id"].get<int>();
    string pwd=js["password"];

    User user = _userModel.query(id);
    if(user.getId()==id && user.getPwd()==pwd)
    {
        if(user.getState()=="online")
        {
            //该用户已登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"]=2;
            response["errmsg"]="该账号已经登录";
            conn->send(response.dump());
        }
        else
        {
            //登录成功，记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id,conn});
            }
            

            //登录成功,更新用户状态信息 state offline=>online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"]=0;
            response["id"]=user.getId();
            response["name"]=user.getName();
            conn->send(response.dump());
        }
    }
    else
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"]=1;
        response["errmsg"]="用户名或密码错误";
        conn->send(response.dump());
    }
}

//处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name=js["name"];
    string pwd=js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    LOG_INFO << name<<"   do login service";
    bool state=_userModel.insert(user);
    if(state)
    {
        //注册成功
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=0;
        response["id"]=user.getId();
        conn->send(response.dump());
    }
    else
    {
        //注册失败
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=1;
        conn->send(response.dump());
    }
}

//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        
        for(auto it=_userConnMap.begin();it!=_userConnMap.end();++it)
        {
            if(it->second==conn)
            {
                //从map表删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //更新用户的状态信息
    if(user.getId()!=-1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }

}