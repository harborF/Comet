#ifndef _CMD_DEFINE_H__
#define _CMD_DEFINE_H__

namespace ConstUtils {

    const unsigned int kCmdAuthReq = 101;
    const unsigned int kCmdAuthAck = 102;

    const unsigned int kCmdHeartReq = 103;
    const unsigned int kCmdHeartAck = 104;

    const unsigned int kCmdPushMsgReq = 105;
    const unsigned int kCmdPushMsgAck = 106;

    const unsigned int kCmdKillConnReq = 107;

    /**************************************/
    const unsigned int kLoginBegin = 200;
    const unsigned int kLoginEnd = 299;

    const unsigned int kCmdLogin = 201;

    /**************************************/
    const unsigned int kCmdPushMsg = 1001;

}

#endif // !_CMD_DEFINE_H__
