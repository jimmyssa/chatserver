#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include"group.hpp"
#include<string>
#include<vector>
using namespace std;

//维护群组信息的操作接口方法
class GroupModel
{
public:
    //创建群组
    bool createGroup(Group &group);
    //加入群组
    void addGroup(int userid,int groupid,string role);
    //查询用户所在群组信息
    vector<Group> queryGroups(int userid);
    //查询群组用户id列表,主要用户群聊业务给群组其它成员群发消息
    vector<GroupUser> queryGroupUsers(int groupid);
     // 函数2：查询群组中除指定用户外的其他成员
    vector<int> queryOtherUsersInGroup(int groupid, int excludeUserid);
};


#endif