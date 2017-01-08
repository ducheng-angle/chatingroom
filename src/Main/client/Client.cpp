

#include "Client.hpp"
#include "include/Log.hpp"
#include "MsgType.hpp"
#include "include/common.h"

int CRClient::Init()
{
    int err = 0;
    uint64_t connId;

    do {
        err = mNetService.Start();
        if (0 != err) {
            error_log("netservice start failed!");
            break;
        }

        err = mNetService.StartConnectRemote(mIP, mPort, connId);
        if (0 != err) {
            error_log("client connect remote failed! ip: " << mIP 
                    << ", port: " << mPort);
            break;
        }

        {
            OperContext *replyctx = new OperContext(OperContext::OP_SEND);              
            Msg *repmsg = new Msg();                                                    
            (*repmsg) << MsgType::c2s_logon;
            (*repmsg) << mUser; 
            repmsg->SetLen();                                                           
            replyctx->SetMessage(repmsg);                                               
            replyctx->SetConnID(connId);                                                
            mNetService.Enqueue(replyctx);                                              
            OperContext::DecRef(replyctx);
        }

    } while(0);

    return err;
}

void
CRClient::RecvMessage(OperContext *ctx)
{
    Msg *msg = ctx->GetMessage();
    int msgType;
    (*msg) >> msgType;

    trace_log("CRClient RecvMessage! type: " << msgType);

    switch(msgType)
    {
        case MsgType::c2s_logon_res:
            HandleLogonRes(msg);
            break;
    }

    delete msg;
    ctx->SetMessage(NULL);
}

bool
CRClient::Process(OperContext *ctx)
{
    bool processed = true;
    debug_log("CRClient::Process! ctx type: " << ctx->GetType());

    switch (ctx->GetType())
    {
        case OperContext::OP_RECV:
            RecvMessage(ctx);
            break;
        case OperContext::OP_SEND:
            mNetService.Enqueue(ctx);
            break;
        default:
            processed = false;
            break;
    }

    return processed;
}

int 
CRClient::Finit()
{
    return 0;
}

void
CRClient::HandleLogonRes(Msg *msg)
{
    int err = 0;
    std::string errstr;
    (*msg) >> err;
    (*msg) >> errstr;

    debug_log("handle logon " << err
            << ", errstr: " << errstr);
    if (0 == err) {
        ParseRoomAndUserList(msg);
        UpdateWindowUserList();
    } else {
        error_log("logon failed, errno: " << err
                << ", errstr: " << errstr);
        /* TODO exit normally */
        exit(err);
        LogonFailed();
    }
}

void 
CRClient::ParseRoomAndUserList(Msg *msg)
{
    uint32_t count = 0;
    iter_id_user iter = mOnlines.begin();
    /* clear mOnlines*/
    for (; iter != mOnlines.end(); ++iter) {
        delete iter->second;
    }
    mOnlines.clear();

    mCurrentRoom.Decode(msg);
    trace_log("room info: " << mCurrentRoom.DebugString());

    (*msg) >> count;
    debug_log("parse user list count: " << count);
    for (uint32_t i = 0; i < count; ++i) {
        User *user = new User;
        user->Decode(msg);
        debug_log("user info:" << user->DebugString());
        mOnlines.insert(std::make_pair(user->mID, user));
    }
}

void
CRClient::UpdateWindowUserList()
{
    std::vector<std::string> vec;
    std::string title = "<L></B/U/31>NAME  ID   IP   PORT<!31>";
    vec.push_back(title);

    iter_id_user index = mOnlines.begin();
    for (;index != mOnlines.end(); ++index) {
        User *user = index->second;
        std::string temp;
        temp += "<L></B/31>";
        temp += user->GetName() + "  ";
        temp += i2s(user->GetId()) + "  ";
        temp += user->GetIp() + "  ";
        temp += i2s(user->GetPort()) + "  ";
        vec.push_back(temp);
    }

    mLayout.UpdateUserListWithStringVec(vec);
}

void
CRClient::LogonFailed()
{
}






