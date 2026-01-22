// 首先包含系统头文件
#include <muduo/base/Logging.h>
#include <string>
#include<vector>
#include<map>
// 然后包含项目头文件
#include "chatservice.hpp"
#include "public.hpp"
#include"friendmodel.hpp"
// 使用命名空间（放在所有头文件包含之后）
using namespace std::placeholders;
using namespace muduo;
using namespace muduo::net;
using namespace std;

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
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
}

//服务器异常，业务重置方法
void ChatService::reset()
{
    //把online状态用户，设置成offline
    _userModel.resetState();
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
            response["errmsg"]="this account is using,input another!";
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

            //查询该用户是否有离线消息
            vector<string>vec = _offlineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"]=vec;
                //读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }
            //查询该用户的好友信息
            vector<User> userVec=_friendModel.query(id);
            if(!userVec.empty())
            {
                vector<string> vec2;
                for(User &user : userVec)
                {
                    json js;
                    js["id"]=user.getId();
                    js["name"]=user.getName();
                    js["state"]=user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"]=vec2;
            }

            vector<Group> groups = _groupModel.queryGroups(id);
            if (!groups.empty()) 
            {
                vector<string> groupsJson;
                for (Group &group : groups) 
                {
                    json grpjs;
                    grpjs["id"] = group.getId();
                    grpjs["groupname"] = group.getName();
                    grpjs["groupdesc"] = group.getDesc();
                    
                    // ========== 关键修改：添加群组成员信息 ==========
                    // 查询该群组的成员
                    vector<GroupUser> groupUsers = _groupModel.queryGroupUsers(group.getId());
                    
                    if (!groupUsers.empty()) 
                    {
                        vector<string> usersJson;
                        for (GroupUser &guser : groupUsers) 
                        {
                            json userjs;
                            userjs["id"] = guser.getId();
                            userjs["name"] = guser.getName();
                            userjs["state"] = guser.getState();
                            userjs["role"] = guser.getRole();
                            usersJson.push_back(userjs.dump());
                        }
                        grpjs["users"] = usersJson;  // 注意：这里是字符串数组
                    }
                    else
                    {
                        // 如果没有成员，返回空数组
                        grpjs["users"] = vector<string>();
                    }
                    
                    groupsJson.push_back(grpjs.dump());  // 将整个群组对象转为字符串
                }
                response["groups"] = groupsJson;  // 字符串数组
            }

            
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
//处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            //从map表删除用户的连接信息
            _userConnMap.erase(it);
        }
    }

    //更新用户的状态信息
    User user(userid,"","", "offline");
    _userModel.updateState(user);
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

void ChatService::oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int toid = js["to"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            //toid 在线，转发消息 服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }


        // toid 不在线，存储离线消息
    _offlineMsgModel.insert(toid,js.dump());
}

//添加好友业务 msg id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //存储好友信息
    _friendModel.insert(userid,friendid);
}


//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    //存储新创建的群组信息
    Group group(-1,name,desc);
    if(_groupModel.createGroup(group))
    {
        //存储群组创建人信息
        _groupModel.addGroup(userid,group.getId(),"creator");
    }

}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
   int userid=js["id"].get<int>();
   int groupid = js["groupid"].get<int>();
   _groupModel.addGroup(userid,groupid,"normal"); 
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryOtherUsersInGroup(userid,groupid);
    lock_guard<mutex> lock(_connMutex);
    for(int id: useridVec)
    {
        auto it = _userConnMap.find(id);
        if(it!=_userConnMap.end())
        {
            //转发群消息
            it->second->send(js.dump());
        }
        else
        {
            //存储离线群消息
            _offlineMsgModel.insert(id,js.dump());
        }
    }
}