// Copyright(c) Microsoft Open Technologies, Inc. All rights reserved. 
// Licensed under the BSD 2 - Clause License. 
// See License.txt in the project root for license information. 

#include <windows.h>
#include <wincrypt.h>

#include "MinXHttpRqst.h"

MinXHttpRqstCallback::MinXHttpRqstCallback()
{
    _hComplete = NULL;
    _respStr.clear();
    _hdrStr.clear();
    _hResult = S_OK;
    _dwStatus = 0;
    _fRetry = FALSE;
    _dwCertIgnoreFlags = 0;
    _cIssuerList = 0;
}

MinXHttpRqstCallback::~MinXHttpRqstCallback()
{
}

STDMETHODIMP MinXHttpRqstCallback::RuntimeClassInitialize() {
    HANDLE hEvent = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if (hEvent == NULL) return HRESULT_FROM_WIN32(GetLastError());
    _hComplete = hEvent;
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::ReadStream (_In_opt_ ISequentialStream *pStream)
{
    if (pStream == NULL) return E_INVALIDARG;
    for (;;)
    {
        size_t bufsz = 100 * 1024;
        char *bufp = new char[bufsz];
        if (bufp == NULL) return E_OUTOFMEMORY;
        DWORD dwread = 0;
        HRESULT hresult = pStream->Read(bufp, bufsz - 1, &dwread);
        if (FAILED(hresult) || dwread == 0) return hresult;
        if (dwread < bufsz) bufp[dwread] = 0; else bufp[bufsz - 1] = 0;
        _respStr.append(bufp);
        delete[] bufp;
    }
}

STDMETHODIMP MinXHttpRqstCallback::OnResponseReceived(__RPC__in_opt IXMLHTTPRequest2 *pXHR,	__RPC__in_opt ISequentialStream *pResponseStream	)
{
    UNREFERENCED_PARAMETER(pXHR);
    _respStr.clear();
    HRESULT hresult = ReadStream(pResponseStream);
    SetEvent(_hComplete);
    return hresult;
}

STDMETHODIMP MinXHttpRqstCallback::OnError(__RPC__in_opt IXMLHTTPRequest2 *pXHR, HRESULT hrError)
{
    UNREFERENCED_PARAMETER(pXHR);
    _hResult = hrError;
    SetEvent(_hComplete);
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::WaitForComplete(_Out_ PDWORD pdwStatus)
{
    if (pdwStatus == NULL) return E_INVALIDARG;
    DWORD dw = WaitForSingleObject(_hComplete, INFINITE);
    if (dw == WAIT_FAILED) return HRESULT_FROM_WIN32(GetLastError());
    if (dw != WAIT_OBJECT_0) return E_ABORT;
    if (FAILED(_hResult)) return _hResult;
    *pdwStatus = dw;
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::OnHeadersAvailable(__RPC__in_opt IXMLHTTPRequest2 *pXHR, DWORD dwStatus, __RPC__in_string const WCHAR *pwszStatus )
{
    UNREFERENCED_PARAMETER(pwszStatus);
    _dwStatus = dwStatus;
    if (pXHR == NULL) return E_INVALIDARG;

    PWSTR pwszHeaders = NULL;
    HRESULT hresult = pXHR->GetAllResponseHeaders(&pwszHeaders);
    if (FAILED(hresult)) return hresult;
    _hdrStr.assign(pwszHeaders);
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::OnClientCertificateRequested(__RPC__in_opt IXMLHTTPRequest3 *pXHR, DWORD cIssuerList, __RPC__in_ecount_full_opt(cIssuerList) const WCHAR **rgpwszIssuerList)
{
    UNREFERENCED_PARAMETER(pXHR);
    if (cIssuerList == 0 || rgpwszIssuerList == NULL) return S_OK;

    HRESULT hresult = DuplicateIssuerList(cIssuerList, rgpwszIssuerList, &_rgpwszIssuerList);
    if (FAILED(hresult)) return hresult;

    _cIssuerList = cIssuerList;
    _fRetry = TRUE;

    return hresult;
}

STDMETHODIMP MinXHttpRqstCallback::OnServerCertificateReceived(__RPC__in_opt IXMLHTTPRequest3 *pXHR, DWORD dwCertErrors, DWORD cServerCertChain, __RPC__in_ecount_full_opt(cServerCertChain) const XHR_CERT *rgServerCertChain)
{
    UNREFERENCED_PARAMETER(pXHR);
    if (cServerCertChain == 0 || rgServerCertChain == NULL) return S_OK;

    for (DWORD ix = 0; ix < cServerCertChain; ix++)
    {
        BYTE *pbCert = rgServerCertChain[ix].pbCert;
        DWORD cbCert = rgServerCertChain[ix].cbCert;
        PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, pbCert, cbCert);
        if (pCertContext) CertFreeCertificateContext(pCertContext);
        pCertContext = NULL;
    }

    DWORD dwErrs = 0;
    dwErrs |= dwCertErrors & XHR_CERT_ERROR_REVOCATION_FAILED;
    dwErrs |= dwCertErrors & XHR_CERT_ERROR_UNKNOWN_CA;
    dwErrs |= dwCertErrors & XHR_CERT_ERROR_CERT_CN_INVALID;
    dwErrs |= dwCertErrors & XHR_CERT_ERROR_CERT_DATE_INVALID;
    if (dwErrs != 0) {
        _dwCertIgnoreFlags = dwErrs;
        _fRetry = TRUE;
    }
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::GetCertResult(_Out_ BOOL *pfRetry, _Out_ DWORD *pdwCertIgnoreFlags, _Out_ DWORD *pcIssuerList, _Out_ const WCHAR ***prgpwszIssuerList)
{
    *pfRetry = _fRetry;
    *pdwCertIgnoreFlags = _dwCertIgnoreFlags;
    if (_cIssuerList != 0 && _rgpwszIssuerList != NULL)
    {
        *pcIssuerList = _cIssuerList;
        return DuplicateIssuerList(_cIssuerList, _rgpwszIssuerList, prgpwszIssuerList);
    }
    *pcIssuerList = 0;
    *prgpwszIssuerList = NULL;
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::DuplicateIssuerList(_In_ DWORD cIssuerList, _In_reads_(cIssuerList) const WCHAR **rgpwszIssuerList, _Out_ const WCHAR ***prgpwszDuplicateIssuerList)
{
    HRESULT hresult = S_OK;
    DWORD ix = 0;
    const WCHAR **rgpwszDuplicateIssuerList = NULL;

    if (cIssuerList == 0 || rgpwszIssuerList == NULL)
    {
        hresult = E_INVALIDARG;
        goto Exit;
    }

    rgpwszDuplicateIssuerList = (const WCHAR**) new WCHAR*[cIssuerList];
    if (rgpwszDuplicateIssuerList == NULL)
    {
        hresult = E_OUTOFMEMORY;
        goto Exit;
    }

    memset(rgpwszDuplicateIssuerList, 0, sizeof(WCHAR*)* cIssuerList);

    for (ix = 0; ix < cIssuerList && rgpwszIssuerList[ix] != NULL; ix++)
    {
        DWORD cchIssuer = (DWORD)wcslen(rgpwszIssuerList[ix]);
        WCHAR * pwszIssuer = new WCHAR[cchIssuer + 1];
        if (pwszIssuer == NULL)
        {
            hresult = E_OUTOFMEMORY;
            goto Exit;
        }

        wcscpy_s(pwszIssuer, cchIssuer + 1, rgpwszIssuerList[ix]);
        (rgpwszDuplicateIssuerList)[ix] = pwszIssuer;
    }

Exit:

    if (FAILED(hresult))
    {
        FreeIssuerList(ix, rgpwszDuplicateIssuerList);
        rgpwszDuplicateIssuerList = NULL;
    }

    *prgpwszDuplicateIssuerList = rgpwszDuplicateIssuerList;
    return hresult;
}

STDMETHODIMP MinXHttpRqstCallback::FreeIssuerList(_In_ DWORD cIssuerList, _Frees_ptr_ const WCHAR **rgpwszIssuerList )
{
    if (cIssuerList == 0 || rgpwszIssuerList == NULL) return S_OK;
    for (DWORD ix = 0; ix < cIssuerList; ix++)
    {
        delete[] rgpwszIssuerList[ix];
        rgpwszIssuerList[ix] = NULL;
    }
    delete[] rgpwszIssuerList;
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::OnDataAvailable(__RPC__in_opt IXMLHTTPRequest2 *pXHR, __RPC__in_opt ISequentialStream *pResponseStream)
{
    UNREFERENCED_PARAMETER(pXHR);
    UNREFERENCED_PARAMETER(pResponseStream);
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::OnRedirect(__RPC__in_opt IXMLHTTPRequest2 *pXHR, __RPC__in_string const WCHAR *pwszRedirectUrl)
{
    UNREFERENCED_PARAMETER(pXHR);
    UNREFERENCED_PARAMETER(pwszRedirectUrl);
    return S_OK;
}


MinXHttpRqstPostStream::MinXHttpRqstPostStream()
{
    _hFile = INVALID_HANDLE_VALUE;
}

MinXHttpRqstPostStream::~MinXHttpRqstPostStream()
{
    if (_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hFile);
        _hFile = INVALID_HANDLE_VALUE;
    }
}

STDMETHODIMP MinXHttpRqstPostStream::Open(_In_opt_ PCWSTR pcwszFileName)
{
    if (pcwszFileName == NULL || *pcwszFileName == L'\0') return E_INVALIDARG;
    HANDLE hFile = CreateFile(pcwszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());
    _hFile = hFile;
    return S_OK;
}

STDMETHODIMP MinXHttpRqstPostStream::Read(_Out_writes_bytes_to_(cb, *pcbRead) void *pv, ULONG cb, _Out_opt_  ULONG *pcbRead)
{
    if (pcbRead != NULL) *pcbRead = 0;
    if (pv == NULL || cb == 0) return E_INVALIDARG;

    DWORD cbRead = 0;
    BOOL fSuccess = ReadFile(_hFile, pv, cb, &cbRead, NULL);
    if (!fSuccess)
    {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_HANDLE_EOF) return HRESULT_FROM_WIN32(dwError);
    }

    HRESULT hresult = S_OK;
    if (cbRead < cb) hresult = S_FALSE;
    if (pcbRead != NULL) *pcbRead = cbRead;
    return hresult;
}

STDMETHODIMP MinXHttpRqstPostStream::Write(_In_reads_bytes_(cb)  const void *pv, ULONG cb, _Out_opt_  ULONG *pcbWritten)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(cb);
    UNREFERENCED_PARAMETER(pcbWritten);
    return STG_E_ACCESSDENIED;
}

STDMETHODIMP MinXHttpRqstPostStream::GetSize(_Out_ ULONGLONG *pullSize)
{
    if (pullSize == NULL) return E_INVALIDARG;
    *pullSize = 0;
    LARGE_INTEGER liFileSize = {};
    BOOL fSuccess = GetFileSizeEx(_hFile, &liFileSize);
    if (!fSuccess) return HRESULT_FROM_WIN32(GetLastError());
    *pullSize = (ULONGLONG)liFileSize.QuadPart;
    return S_OK;
}
