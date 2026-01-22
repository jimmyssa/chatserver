#include "json.hpp"
#include<iostream>
#include<thread>
#include<string>
#include<vector>
#include<chrono>
#include<ctime>
using namespace std;
using json = nlohmann::json;

#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include"group.hpp"
#include"user.hpp"
#include"public.hpp"

//记录当前系统登陆的用户信息
User g_currentUser;
//记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
//记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
//显示当前登录成功用户的基本信息
void showCurrentUserData();


//接受线程
void readTaskHandler(int clientfd);
//获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
//控制主菜单页面程序
bool isMainMenuRunning = false;
//主聊天页面程序
void mainMenu(int clientfd);

//聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example:./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    for (;;)
    {
        cout << "===================================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "===================================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get();

        switch (choice)
        {
        case 1: // login
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get();
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), request.length() + 1, 0);
            if (-1 == len)
            {
                cerr << "send login msg error:" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv login response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>())
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else
                    {
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        if (responsejs.contains("friends"))
                        {
                            g_currentUserFriendList.clear();
                            vector<string> vec = responsejs["friends"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }
                        


                        if (responsejs.contains("groups"))
                        {
                            g_currentUserGroupList.clear();
                            vector<string> vec1 = responsejs["groups"];
                            for (string &groupstr : vec1)
                            {
                                json grpjs = json::parse(groupstr);
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);

                                vector<string> vec2 = grpjs["users"];
                                for (string &userstr : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }
                                g_currentUserGroupList.push_back(group);
                            }
                        }

                        showCurrentUserData();
                        
                        //显示当前用户的离线消息
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                int msgtype = js["msgid"].get<int>();
                                if(ONE_CHAT_MSG == msgtype)
                                {
                                    cout<<js["time"].get<string>()<<"["<<js["id"]<<"]"<<js["name"].get<string>()
                                    <<"said:"<<js["msg"].get<string>()<<endl;
                                }
                                else if(GROUP_CHAT_MSG == msgtype)  // 明确判断群聊消息
                                {
                                    cout << "群消息{" << js["groupid"] << "}:" << js["time"].get<string>() 
                                        << "[" << js["id"] << "]" << js["name"].get<string>()
                                        << " said: " << js["msg"].get<string>() << endl;
                                }
                                else
                                {
                                    cout << "未知类型离线消息: " << js.dump() << endl;
                                }
                            }
                        }
                        
                        //登录成功 启动接收线程负责接收数据,该线程只启动一次
                        static int readthreadnumber=0;
                        if(readthreadnumber==0)
                        {
                            std::thread readTask(readTaskHandler, clientfd);//pthread_create
                            readTask.detach();//pthread_detach
                            readthreadnumber++;
                        }
                        isMainMenuRunning = true;
                        mainMenu(clientfd);  // 进入主菜单
                    }
                }
            }
        }
        break;

        case 2: // register
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), request.length() + 1, 0);
            if (-1 == len)
            {
                cerr << "send reg msg error: " << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>())
                    {
                        cerr << name << " is already exist, register error!" << endl;
                    }
                    else
                    {
                        cout << name << " register success, userid is " << responsejs["id"] 
                             << ", do not forget it!" << endl;
                    }
                }
            }
        }
        break;

        case 3: // quit
            close(clientfd);
            cout << "client quit" << endl;
            exit(0);
            break;

        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }

    return 0;  // 这行可能永远不会执行，但为了完整性保留
}

//接受线程
void readTaskHandler(int clientfd)
{
    for(;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd,buffer,1024,0);
        if(-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }
        //接受ChatSever转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if(ONE_CHAT_MSG == msgtype)
        {
            cout<<js["time"].get<string>()<<"["<<js["id"]<<"]"<<js["name"].get<string>()
            <<"said:"<<js["msg"].get<string>()<<endl;
            continue;
        }
        else if(GROUP_CHAT_MSG == msgtype)
        {
            cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() 
                 << "[" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
        }
        else
        {
            // 添加默认处理，避免消息丢失
            cout << "收到系统消息[type=" << msgtype << "]: " << js.dump() << endl;
        }
    }
}



//显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "=====================登录user=====================" << endl;
    cout << "current login user >> id: " << g_currentUser.getId() << " name: " << g_currentUser.getName() << endl;
    
    cout << "=====================好友列表=====================" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    
    cout << "=====================Group list=====================" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    
    cout << "=====================获取系统时间(聊天信息需要添加时间信息)=====================" << endl;
}



// 命令处理函数声明
void help(int fd = 0,string str = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}
};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}
};

void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning) {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        
        int idx = commandbuf.find(":");
        if (idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }
        
        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需修改此函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}

void help(int , string )  //
{
    cout << "show command list" << endl;
    for (auto& p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

void addfriend(int clientfd,string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len == -1)
    {
        cerr<<"send addfriend msg error"<<buffer<<endl;
    }
}

void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }
    
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
    
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    
    string buffer = js.dump();
    
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat msg error -> " << buffer << endl;
    }
}

void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr<<"creategroup command invalid!"<<endl;
        return;
    }

    string groupname = str.substr(0,idx);
    string groupdesc = str.substr(idx+1, str.size()-idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len == -1)
    {
        cerr<<"send creatgroup error ->"<<buffer<<endl;
    }
}
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len == -1)
    {
        cerr<<"send addgroup msg error"<<buffer<<endl;
    }
}
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr<<"groupchat command invalid!"<<endl;
        return;
    }

    int groupid = atoi(str.substr(0,idx).c_str());
    string message = str.substr(idx+1, str.size()-idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len == -1)
    {
        cerr<<"send groupchat error ->"<<buffer<<endl;
    }
}
void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len == -1)
    {
        cerr<<"send loginout msg error"<<buffer<<endl;
    }
    else
    {
        isMainMenuRunning = false; // 停止主菜单循环
    }
}

string getCurrentTime()
{
    // 获取当前时间的实现代码
    auto tt = std::chrono::system_clock::to_time_t(chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60]={0};
    sprintf(date,"%d-%02d-%02d %02d:%02d:%02d",
    (int)ptm->tm_year +1900,(int)ptm->tm_mon+1,(int)ptm->tm_mday,
    (int)ptm->tm_hour,(int)ptm->tm_min,(int)ptm->tm_sec);
    return string(date);
}