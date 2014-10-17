#pragma once

#include <wrl.h>
#include <string>
#include <list>

#include "MinXHttpRqst.h"

class NameValuePair {
public:
    std::wstring name;
    std::wstring value;
};

class MinHttpGP {
private:
    void TextToFile(const char *txt, int txtsz, wchar_t *fname);
    enum MinHttpGPType { Get, Post };
    void MinHttpGP::MinHttpSendRecv(
        _In_ MinHttpGPType Method,
        _In_ PCWSTR pcwszUrl,
        _Inout_ BOOL *pfRetry,
        _Inout_ DWORD *pdwCertIgnoreFlags,
        _Inout_ DWORD *pcIssuerList,
        _Inout_ const WCHAR ***prgpwszIssuerList,
        _Inout_ char *respbuf,
        _In_ ULONG bufsize,
        _Out_ ULONG *pnRead
        );

    const char * _pcstrContent;
    wchar_t _url[10 * 1024];
    bool _fCoInit;
public:
    std::list<NameValuePair> *_headers;
    bool _showlog;
    void MinHttpGP::PrtfLog(_In_ const wchar_t *ctrlp, ...);
    void MinHttpGP::PrtfLog(_In_ const char *ctrlp, ...);

    MinHttpGP();
    ~MinHttpGP();

    void MinHttpGP::GetRqst(_In_ WCHAR *url, _In_ std::list<NameValuePair> *HeaderVals, _Inout_ char *replybuf, _In_ ULONG bufsize, _Out_ ULONG * nread);
    void MinHttpGP::PostRqst(_In_ WCHAR *url, _In_ std::list<NameValuePair> *HeaderVals, _In_ const char *content, _Inout_ char *replybuf, _In_ ULONG bufsize, _Out_ ULONG * nread);
};
