#ifndef _ERROR_CODE_H__
#define _ERROR_CODE_H__

namespace ConstUtils
{
    const int kSeccuss      = 0;//ok
    const int kErrParam     = -3001;//��������
    const int kErrInner     = -3002;//�������ڲ�����
    const int kErrVersion   = -3003;//�汾����
    const int kErrLength    = -3004;//���ݰ����ȴ���
    const int kErrCmdId     = -3005;//����ID����
    const int kErrSeqId     = -3006;//��Ŵ���
    const int kErrBodyParse = -3007;//����������
    const int kErrNoLogin   = -3008;//δ��¼
    const int kErrSign      = -3009;//ǩ������
    const int kErrLogout    = -3010;//�������ж�����
    const int kErrReLogin   = -3011;//�ظ���½�ж�����

}

#endif // !_ERROR_CODE_H__
