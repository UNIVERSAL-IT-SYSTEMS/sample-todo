// Copyright(c) Microsoft Open Technologies, Inc. All rights reserved. 
// Licensed under the BSD 2 - Clause License. 
// See License.txt in the project root for license information. 

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

struct CertSaves {
    DWORD CertIgnoreFlags;
    DWORD cIssuerList;
    const WCHAR ** IssuerList;
};

class MinHttpGP {
private:
    const wchar_t * MinHttpGP::GetLastErrorToString(DWORD err);
    void TextToFile(const char *txt, int txtsz, wchar_t *fname);
    enum MinHttpGPType { Get, Post };
    void MinHttpGP::MinHttpSendRecv(
        _In_ MinHttpGPType Method,
        _In_ PCWSTR pcwszUrl,
        _Inout_ BOOL *pfRetry,
        _Inout_ struct CertSaves *pCertSaves,
        _Inout_ std::string *respStr
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

    void MinHttpGP::GetRqst(_In_ WCHAR *url, _In_ std::list<NameValuePair> *HeaderVals, _Inout_ std::string *respStr);
    void MinHttpGP::PostRqst(_In_ WCHAR *url, _In_ std::list<NameValuePair> *HeaderVals, _In_ const char *content, _Inout_ std::string *respStr);
};
