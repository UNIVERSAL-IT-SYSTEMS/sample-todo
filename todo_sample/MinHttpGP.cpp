
#include <tchar.h>
#include <msxml6.h>
#include <wrl.h>

#include "MinHttpGP.h"

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <ostream>

using namespace Microsoft::WRL;

MinHttpGP::MinHttpGP()
{
    _headers = nullptr;
    _showlog = false;

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) _fCoInit = true;
}

MinHttpGP::~MinHttpGP()
{
    if (_fCoInit) CoUninitialize();
}

void MinHttpGP::TextToFile(const char *txt, int txtsz, wchar_t *fname) 
{
    std::ofstream f_tmp(fname);
    if (!f_tmp) {
        PrtfLog(L"MinHttpGP ERROR: Cannot create %s\n", fname);
        return;
    }
    f_tmp.write(txt, txtsz);
    f_tmp.close();
}

void MinHttpGP::MinHttpSendRecv(_In_ MinHttpGPType Method, _In_ PCWSTR pcwszUrl, _Inout_ BOOL *pfRetry, _Inout_ struct CertSaves *pCertSaves, _Inout_ std::string *respStr)
{
    HRESULT hr = S_OK;
    DWORD dwStatus = 0;
    BOOL fAbort = TRUE;
    ComPtr<IXMLHTTPRequest3> spXHR = nullptr;
    ComPtr<MinXHttpRqstCallback> spXhrCallback = nullptr;

    BOOL fRetry = *pfRetry;
    DWORD dwCertIgnoreFlags = pCertSaves->CertIgnoreFlags;
    DWORD cIssuerList = pCertSaves->cIssuerList;
    const WCHAR **rgpwszIssuerList = pCertSaves->IssuerList;

    if (respStr != NULL) respStr->clear();

    hr = CoCreateInstance(CLSID_FreeThreadedXMLHTTP60, NULL, CLSCTX_INPROC_SERVER,  IID_PPV_ARGS(&spXHR));
    if (FAILED(hr)) goto Exit;

    hr = MakeAndInitialize<MinXHttpRqstCallback>(&spXhrCallback);
    if (FAILED(hr)) goto Exit;

    hr = spXHR->Open( (Method == Get ? L"GET" : L"POST") , pcwszUrl, spXhrCallback.Get(), NULL, NULL, NULL, NULL);
    if (FAILED(hr)) goto Exit;

    if (fRetry && dwCertIgnoreFlags != 0)
    {
        hr = spXHR->SetProperty(XHR_PROP_IGNORE_CERT_ERRORS, dwCertIgnoreFlags);
    }
    if (FAILED(hr)) goto Exit;

    if (_headers != nullptr) {
        for each (NameValuePair nvp in *_headers) {
            hr = spXHR->SetRequestHeader(nvp.name.c_str(), nvp.value.c_str());
            if (FAILED(hr)) goto Exit;
        }
    }

    // Send GET or PUT request
    if (Method == Get) {
        hr = spXHR->Send(NULL, 0);

    } else { 
        ComPtr<MinXHttpRqstPostStream> spXhrPostStream;
        spXhrPostStream = Make<MinXHttpRqstPostStream>();
        if (spXhrPostStream == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        if (_pcstrContent != NULL) {
            TextToFile(_pcstrContent, strlen(_pcstrContent), L"$minhttp$.txt");
        } else {
            TextToFile(" ", 1, L"$minhttp$.txt");
        } 
        hr = spXhrPostStream->Open(L"$minhttp$.txt");
        if (FAILED(hr)) goto Exit;

        ULONGLONG ullContentSize = 0;
        hr = spXhrPostStream->GetSize(&ullContentSize);
        if (FAILED(hr)) goto Exit;

        hr = spXHR->Send(spXhrPostStream.Get(), ullContentSize);
    }

    if (FAILED(hr)) {
        PrtfLog(L"MinHttpGP ERROR: spXHR->Send failed. %u", hr);
        goto Exit;
    }
        
    hr = spXhrCallback->WaitForComplete(&dwStatus);
    if (FAILED(hr)) goto Exit;

    if (respStr != NULL) respStr->assign(spXhrCallback->_respStr);
    fAbort = FALSE;

  Exit:
    //return cert values for potential retries
    if (!fRetry && SUCCEEDED(spXhrCallback->GetCertResult(&fRetry,  &dwCertIgnoreFlags, &cIssuerList, &rgpwszIssuerList)))
    {
        *pfRetry = fRetry;
        pCertSaves->CertIgnoreFlags = dwCertIgnoreFlags;
        pCertSaves->cIssuerList = cIssuerList;
        pCertSaves->IssuerList = rgpwszIssuerList;
    }

    if (FAILED(hr))
    {
        PrtfLog(L"MinHttpGP %s FAILED, Error code = 0x%08x.\n", (Method == Get ? L"GET" : L"POST"), hr);
        if (spXhrCallback != nullptr) PrtfLog(L"%s\n", spXhrCallback->_hdrStr.c_str());
    }
        
    if (spXhrCallback != nullptr && (spXhrCallback->_respStr.length() > 0))
    {
        PrtfLog(L"--------------------------------------------------\n");
        PrtfLog(L"%s %s\n", (Method == Get ? L"GET" : L"POST"), pcwszUrl);
        PrtfLog(L"Status Code = %u.\n", dwStatus);
        PrtfLog(L"%s\n", spXhrCallback->_hdrStr.c_str());
        PrtfLog(L"Response text len = %u\n", spXhrCallback->_respStr.length());
        if (_showlog && spXhrCallback->_respStr.length() > 0) {
            OutputDebugStringA(spXhrCallback->_respStr.c_str());
        }
    }

    if (fAbort) spXHR->Abort();
 }

void MinHttpGP::PrtfLog(_In_ const wchar_t *ctrlp, ...)
{
    if (!_showlog) return;

    va_list marker;

    va_start(marker, ctrlp);
    int len = _vscwprintf(ctrlp, marker) + 1;
    wchar_t * buffer = new wchar_t[len];
    if (buffer != NULL)
    {
        len = vswprintf_s(buffer, len, ctrlp, marker);
        if (IsDebuggerPresent()) OutputDebugStringW(buffer);
        delete[](buffer);
    }
    else {
        vwprintf(ctrlp, marker);
    }
    va_end(marker);
}


void MinHttpGP::GetRqst(_In_ WCHAR *url, _In_ std::list<NameValuePair> *HeaderVals, _Inout_ std::string *respStr)
{
    HRESULT hr = S_OK;
    BOOL fRetry = FALSE;

    struct CertSaves cs;
    memset(&cs, 0, sizeof(cs));

    wcscpy_s(_url, _countof(_url), url);
    _headers = HeaderVals;
    _pcstrContent = NULL;
    MinHttpSendRecv(Get, _url, &fRetry, &cs, respStr);
    if (fRetry) MinHttpSendRecv(Get, _url, &fRetry, &cs, respStr);
}

void MinHttpGP::PostRqst(_In_ WCHAR *url, _In_ std::list<NameValuePair> *HeaderVals, _In_ const char *content, _Inout_ std::string *respStr)
{
    HRESULT hr = S_OK;
    BOOL fRetry = FALSE;

    struct CertSaves cs;
    memset(&cs, 0, sizeof(cs));

    wcscpy_s(_url, _countof(_url), url);
    _headers = HeaderVals;
    _pcstrContent = content;
    MinHttpSendRecv(Post, _url, &fRetry, &cs, respStr);
    if (fRetry) MinHttpSendRecv(Post, _url, &fRetry, &cs, respStr);
}

