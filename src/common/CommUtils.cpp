#include "CommUtils.h"

namespace CommUtils {
    unsigned short readUShort(const void* buf)
    {
        const unsigned char* usBuf = (unsigned char*)buf;
        return (((unsigned short)usBuf[0]) << 8) + usBuf[1];
    }

    unsigned int readUInt(const void* buf)
    {
        const unsigned char* usBuf = (unsigned char*)buf;
        unsigned int iRetValue = ((unsigned int)usBuf[0]) << 24;
        iRetValue += ((unsigned int)usBuf[1]) << 16;
        iRetValue += ((unsigned int)usBuf[2]) << 8;
        iRetValue += ((unsigned int)usBuf[3]);
        return iRetValue;
    }

    unsigned int readUInt(const void* buf, const unsigned char n)
    {
        unsigned int iRetValue = 0;
        const unsigned char* usBuf = (unsigned char*)buf;
        for (unsigned int i = 0; i < n && i < 4; ++i) {
            iRetValue += (iRetValue << 8) + ((unsigned char)usBuf[i]);
        }
        return iRetValue;
    }

    void writeUShort(void* buf, const unsigned short usVal)
    {
        unsigned char* usBuf = (unsigned char*)buf;
        usBuf[0] = ((usVal & 0xff00) >> 8);
        usBuf[1] = (usVal & 0xff);
    }

    void writeUInt(void* buf, const unsigned int uiVal)
    {
        unsigned char* usBuf = (unsigned char*)buf;
        usBuf[0] = ((uiVal & 0xff000000) >> 24);
        usBuf[1] = ((uiVal & 0xff0000) >> 16);
        usBuf[2] = ((uiVal & 0xff00) >> 8);
        usBuf[3] = (uiVal & 0xff);
    }

    void char2Hex(const char* szBuf, const unsigned int uiLen, char* usHex)
    {
        for (unsigned int i = 0; i < uiLen / 2; ++i) {
            const int n = i << 1;
            usHex[i] = (char2Hex(szBuf[n]) << 4) + char2Hex(szBuf[n + 1]);
        }
    }

    void hex2Char(const char* szHex, const unsigned int uiLen, char* szBuf)
    {
        for (unsigned int i = 0; i < uiLen; ++i) {
            const int n = i << 1;
            szBuf[n] = hex2Char(((unsigned char)szHex[i]) >> 4);
            szBuf[n + 1] = hex2Char(((unsigned char)szHex[i]) & 0x0f);
        }
    }

    void readNetHeader(const void* buf, NetPkgHeader& stHeader)
    {
        const unsigned char* usBuf = (unsigned char*)buf;
        stHeader.uiPkgLen = CommUtils::readUInt(usBuf);
        stHeader.usHdrLen = CommUtils::readUShort(usBuf + 4);
        stHeader.usVer = CommUtils::readUShort(usBuf + 6);
        stHeader.uiCmdId = CommUtils::readUInt(usBuf + 8);
        stHeader.uiSeqId = CommUtils::readUInt(usBuf + 12);
    }

    void writeNetHeader(void* buf, const NetPkgHeader& stHeader)
    {
        unsigned char* usBuf = (unsigned char*)buf;
        CommUtils::writeUInt(usBuf, stHeader.uiPkgLen);
        CommUtils::writeUShort(usBuf + 4, stHeader.usHdrLen);
        CommUtils::writeUShort(usBuf + 6, stHeader.usVer);
        CommUtils::writeUInt(usBuf + 8, stHeader.uiCmdId);
        CommUtils::writeUInt(usBuf + 12, stHeader.uiSeqId);
    }

    //////////////////////////////////////////////////////////////
    unsigned short readUShort2(const void* buf)
    {
        const unsigned char* usBuf = (unsigned char*)buf;
        return (((unsigned short)usBuf[1]) << 8) + usBuf[0];
    }

    unsigned int readUInt2(const void* buf)
    {
        const unsigned char* usBuf = (unsigned char*)buf;
        unsigned int iRetValue = ((unsigned int)usBuf[3]) << 24;
        iRetValue += ((unsigned int)usBuf[2]) << 16;
        iRetValue += ((unsigned int)usBuf[1]) << 8;
        iRetValue += ((unsigned int)usBuf[0]);
        return iRetValue;
    }
    
    void writeUShort2(void* buf, const unsigned short usVal)
    {
        unsigned char* usBuf = (unsigned char*)buf;
        usBuf[1] = ((usVal & 0xff00) >> 8);
        usBuf[0] = (usVal & 0xff);
    }

    void writeUInt2(void* buf, const unsigned int uiVal)
    {
        unsigned char* usBuf = (unsigned char*)buf;
        usBuf[3] = ((uiVal & 0xff000000) >> 24);
        usBuf[2] = ((uiVal & 0xff0000) >> 16);
        usBuf[1] = ((uiVal & 0xff00) >> 8);
        usBuf[0] = (uiVal & 0xff);
    }

    void readNetHeader2(const void* buf, NetPkgHeader& stHeader)
    {
        const unsigned char* usBuf = (unsigned char*)buf;
        stHeader.uiPkgLen = CommUtils::readUInt2(usBuf);
        stHeader.usHdrLen = CommUtils::readUShort2(usBuf + 4);
        stHeader.usVer = CommUtils::readUShort2(usBuf + 6);
        stHeader.uiCmdId = CommUtils::readUInt2(usBuf + 8);
        stHeader.uiSeqId = CommUtils::readUInt2(usBuf + 12);
    }

    void writeNetHeader2(void* buf, const NetPkgHeader& stHeader)
    {
        unsigned char* usBuf = (unsigned char*)buf;
        CommUtils::writeUInt2(usBuf, stHeader.uiPkgLen);
        CommUtils::writeUShort2(usBuf + 4, stHeader.usHdrLen);
        CommUtils::writeUShort2(usBuf + 6, stHeader.usVer);
        CommUtils::writeUInt2(usBuf + 8, stHeader.uiCmdId);
        CommUtils::writeUInt2(usBuf + 12, stHeader.uiSeqId);
    }
}//end namespace
