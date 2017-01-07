
#include "assert.h"
#include "Err.hpp"
#include "include/Log.hpp"
#include "RoomKeeper.hpp"

int
RoomKeeper::HandleLogon(const std::string& name, const std::string& ip,
        const unsigned short& port, 
        const uint64_t& userId, std::string& errstr)
{
    int err = 0;
    iter_id_user iter;
    User *user = NULL;

    do {
        /* find if this conn already logoned */
        iter = mIdUser.find(userId);
        if (mIdUser.end() != iter) {
            warn_log("user has logoned before!");
            err = Err::user_name_alredy_logoned;
            errstr = "user has logoned before!";
            break;
        }

        /* find if someone has used this name */
        if (mUserNames.find(name) != mUserNames.end()) {
            warn_log("user with same name exists!");
            err = Err::user_same_name_exist;
            errstr = "user with sanme name exists on server!";
            break;
        }

        /* create User and insert */
        user = new User(name, ip, port, userId);
        mIdUser.insert(std::make_pair(userId, user));
        mUserNames.insert(std::make_pair(name, userId));

        /* join hall */
        err = Join(HALL_NAME, userId, "", errstr);
        assert(err == 0);

    } while(0);

    return err;
}

int 
RoomKeeper::Join(const std::string& roomName, const uint64_t& userId,
        const std::string& passwd, std::string errstr)
{
    int err = 0;
    std::map<std::string, uint32_t>::iterator room_name_id_iter;
    uint32_t roomId;
    iter_id_room id_room_iter;
    Room *room = NULL;

    do {
        /* check if room exists*/
        room_name_id_iter = mRoomNames.find(roomName);
        if (room_name_id_iter == mRoomNames.end()) {
            warn_log("room" << roomName << " not exist!");
            err = Err::room_not_exist;
            errstr = "room not exists!";
            break;
        }

        roomId = room_name_id_iter->second;
        id_room_iter = mIdRoom.find(roomId);
        assert(id_room_iter != mIdRoom.end());
        room = id_room_iter->second;

        if (!room->IsPasswdOkay(passwd)) {
            warn_log("passwd not match ");
            err = Err::room_passwd_not_match;
            errstr = "room passwd not match, try again!";
            break;
        }
        
        /* check if user already exist in this room */
        if (room->IsUserIn(userId)) {
            warn_log("user already in this room");
            err = Err::user_already_in_this_room;
            errstr = "user already in this room";
            break;
        }

        /* add user to this room */
        room->Add(userId);

        /* set room id */
        SetUserRoomId(userId, roomId);

    } while(0);

    return err;
}

void
RoomKeeper::SetUserRoomId(uint64_t userId, uint32_t roomId)
{
    iter_id_user iter = mIdUser.find(userId);
    assert(iter != mIdUser.end());
    User *user = iter->second;
    user->SetRoomId(roomId);
}

void
RoomKeeper::GetRoomUserListMsg(uint32_t roomId, Msg *msg)
{
    std::set<uint64_t> users;
    std::set<uint64_t>::iterator iter_users;
    uint32_t count = 0;
    User *user = NULL;
    iter_id_user id_user_iter;

    /* get all user of this room*/
    iter_id_room iter = mIdRoom.find(roomId);
    assert(iter != mIdRoom.end());
    Room *room = iter->second; 
    users = room->GetAllUsers();

    count = users.size();
    (*msg) << count;

    /* encode msg with user info */
    iter_users = users.begin();
    for (;iter_users != users.end(); ++iter_users) {
        id_user_iter = mIdUser.find(*iter_users);
        assert(id_user_iter != mIdUser.end());
        user = id_user_iter->second;
        user->Encode(msg);
        debug_log("user info " << user->DebugString());
    }
}

int
RoomKeeper::CreateRoom(const std::string& name, const std::string& passwd,
        std::string& errstr)
{
    int err = 0;
    std::map<std::string, uint32_t>::iterator found;
    Room *room = NULL;

    do {
        found = mRoomNames.find(name);
        if (found != mRoomNames.end()) {
            debug_log("room " << name << " already created");
            err = Err::room_with_same_name_already_exists;
            errstr = "room with same name already exists";
            break;
        }

        room = new Room(mRoomId, name, passwd);
        mRoomNames.insert(std::make_pair(name, mRoomId));
        mIdRoom.insert(std::make_pair(mRoomId, room));

        ++mRoomId;
    } while(0);

    return err;
}

void
RoomKeeper::HandleDrop(uint64_t userId)
{
    iter_id_user found;
    iter_id_room roomFound;
    User *user = NULL;
    Room *room = NULL;
    uint32_t roomId;

    do {
        found = mIdUser.find(userId);
        if (found == mIdUser.end()) {
            warn_log("HandleDrop but userid not found");
            break;
        }

        user = found->second;
        roomId = user->GetRoomId();

        roomFound = mIdRoom.find(roomId);
        assert(roomFound != mIdRoom.end());
        room = roomFound->second;

        /* delete from room 
         * inform roomers that he dropped 
         * */
        room->HandleUserLeave(userId);

        /* delete from mIdUser */
        mUserNames.erase(user->GetName());
        mIdUser.erase(userId);

        delete user;
        user = NULL;

    } while(0);

    return;
}





