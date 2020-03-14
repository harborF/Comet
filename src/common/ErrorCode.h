#ifndef _ERROR_CODE_H__
#define _ERROR_CODE_H__

namespace ConstUtils
{
    const int kSeccuss      = 0;//ok
    const int kErrParam     = -3001;//参数错误
    const int kErrInner     = -3002;//服务器内部错误
    const int kErrVersion   = -3003;//版本错误
    const int kErrLength    = -3004;//数据包长度错误
    const int kErrCmdId     = -3005;//命令ID错误
    const int kErrSeqId     = -3006;//序号错误
    const int kErrBodyParse = -3007;//包解析错误
    const int kErrNoLogin   = -3008;//未登录
    const int kErrSign      = -3009;//签名错误
    const int kErrLogout    = -3010;//服务器中断连接
    const int kErrReLogin   = -3011;//重复登陆中断连接

}

#endif // !_ERROR_CODE_H__
