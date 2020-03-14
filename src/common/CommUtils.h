#ifndef _COMM_UTILS_H__
#define _COMM_UTILS_H__

#pragma pack(1)
struct NetPkgHeader {
    unsigned int uiPkgLen;
    unsigned short usHdrLen;
    unsigned short usVer;
    unsigned int uiCmdId;
    unsigned int uiSeqId;
};
#pragma pack()

namespace CommUtils {

    unsigned short readUShort(const void* buf);
    unsigned int readUInt(const void* buf);
    unsigned int readUInt(const void* buf, const unsigned char n);

    void writeUShort(void* usBuf, const unsigned short);
    void writeUInt(void* usBuf, const unsigned int);

    inline unsigned char char2Hex(const char);
    inline char hex2Char(const unsigned char);
    void char2Hex(const char* szBuf, const unsigned int uiLen, char* usHex);
    void hex2Char(const char* szHex, const unsigned int uiLen, char* szBuf);

    void readNetHeader(const void* buf, NetPkgHeader&);
    void writeNetHeader(void* szBuf, const NetPkgHeader&);

    unsigned short readUShort2(const void* buf);
    unsigned int readUInt2(const void* buf);

    void writeUShort2(void* usBuf, const unsigned short);
    void writeUInt2(void* usBuf, const unsigned int);

    void readNetHeader2(const void* buf, NetPkgHeader&);
    void writeNetHeader2(void* szBuf, const NetPkgHeader&);
}

inline unsigned char CommUtils::char2Hex(const char ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else if (ch <= 'f' && ch >= 'a')
        return ch - 'a' + 10;
    else if (ch <= 'F' && ch >= 'A')
        return ch - 'A' + 10;
    return 0;
}

inline char CommUtils::hex2Char(const unsigned char ch) {
    const char szHex[] = "0123456789abcdef";
    if (ch > sizeof(szHex) / sizeof(szHex[0])) {
        return szHex[0];
    }
    return szHex[ch];
}

#endif
