
#include <windows.h>
#include <wincrypt.h>
#include <cryptuiapi.h>
#include "MinXHttpRqst.h"

MinXHttpRqstCallback::MinXHttpRqstCallback()
{
    m_hr = S_OK;
    m_dwStatus = 0;
    m_hComplete = NULL;

    m_fRetry = FALSE;
    m_dwCertIgnoreFlags = 0;
    m_cIssuerList = 0;
    m_rgpwszIssuerList = NULL;

    m_resptxtlen = 0;
    m_resptxt[0] = 0;

    m_hdrtxtlen = 0;
    m_hdrtxt[0] = 0;
}

MinXHttpRqstCallback::~MinXHttpRqstCallback()
{
    if (m_hComplete)
    {
        CloseHandle(m_hComplete);
        m_hComplete = NULL;
    }

    if (m_rgpwszIssuerList)
    {
        FreeIssuerList(m_cIssuerList, m_rgpwszIssuerList);
        m_cIssuerList = 0;
        m_rgpwszIssuerList = NULL;
    }
}

STDMETHODIMP MinXHttpRqstCallback::RuntimeClassInitialize()
{
    HRESULT hr = S_OK;
    HANDLE hEvent = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if (hEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    m_hComplete = hEvent;
    hEvent = NULL;

Exit:

    if (hEvent)
    {
        CloseHandle(hEvent);
        hEvent = NULL;
    }

    return hr;
}

STDMETHODIMP MinXHttpRqstCallback::OnRedirect(
    __RPC__in_opt IXMLHTTPRequest2 *pXHR,
    __RPC__in_string const WCHAR *pwszRedirectUrl
)
{
    UNREFERENCED_PARAMETER(pXHR);
    UNREFERENCED_PARAMETER(pwszRedirectUrl);

    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::OnHeadersAvailable(
    __RPC__in_opt IXMLHTTPRequest2 *pXHR,
    DWORD dwStatus,
    __RPC__in_string const WCHAR *pwszStatus
)
{
    UNREFERENCED_PARAMETER(pwszStatus);

    HRESULT hr = S_OK;

    PWSTR pwszHeaders = NULL;
    PWSTR pwszSingleHeader = NULL;

    if (pXHR == NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = pXHR->GetAllResponseHeaders(&pwszHeaders);
    if (FAILED(hr)) goto Exit;
    wcscpy_s(m_hdrtxt, _countof(m_hdrtxt), pwszHeaders);
    m_hdrtxtlen = wcslen(m_hdrtxt);

    hr = S_OK;

Exit:

    if (pwszHeaders != NULL)
    {
        CoTaskMemFree(pwszHeaders);
        pwszHeaders = NULL;
    }

    if (pwszSingleHeader != NULL)
    {
        CoTaskMemFree(pwszSingleHeader);
        pwszSingleHeader = NULL;
    }

    m_dwStatus = dwStatus;
    return hr;
}

STDMETHODIMP MinXHttpRqstCallback::ReadFromStream(
    _In_opt_ ISequentialStream *pStream
)
{
    HRESULT hr = S_OK;
    DWORD cbRead = 0;

    if (pStream == NULL) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    for(;;)
    {
        hr = pStream->Read(m_resptxt, MAX_BUFFER_LENGTH - 1, &cbRead);

        if (FAILED(hr) || cbRead == 0) goto Exit;

        if ((m_resptxtlen + cbRead) < (_countof(m_resptxt) - 1)) {
            m_resptxtlen += cbRead;
            m_resptxt[m_resptxtlen] = 0;
        }
    }

Exit:
    return hr;
}

STDMETHODIMP MinXHttpRqstCallback::OnDataAvailable(
    __RPC__in_opt IXMLHTTPRequest2 *pXHR,
    __RPC__in_opt ISequentialStream *pResponseStream
)
{
    UNREFERENCED_PARAMETER(pXHR);
    UNREFERENCED_PARAMETER(pResponseStream);
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::OnResponseReceived(
    __RPC__in_opt IXMLHTTPRequest2 *pXHR,
    __RPC__in_opt ISequentialStream *pResponseStream
)
{
    UNREFERENCED_PARAMETER(pXHR);
    HRESULT hr = ReadFromStream(pResponseStream);
    SetEvent(m_hComplete);
    return hr;
}

STDMETHODIMP MinXHttpRqstCallback::OnError(
    __RPC__in_opt IXMLHTTPRequest2 *pXHR,
    HRESULT hrError
)
{
    UNREFERENCED_PARAMETER(pXHR);

    m_hr = hrError;
    SetEvent(m_hComplete);
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::OnServerCertificateReceived( 
    __RPC__in_opt IXMLHTTPRequest3 *pXHR,
    DWORD dwCertErrors,
    DWORD cServerCertChain,
    __RPC__in_ecount_full_opt(cServerCertChain) const XHR_CERT *rgServerCertChain
)
{
    UNREFERENCED_PARAMETER(pXHR);

    PCCERT_CONTEXT pCertContext = NULL;
    BYTE *pbCertEncoded = NULL;
    DWORD cbCertEncoded = 0;
    DWORD i = 0;

    if (cServerCertChain == 0 || rgServerCertChain == NULL) goto Exit;

    for (i = 0; i < cServerCertChain; i++)
    {
        pbCertEncoded = rgServerCertChain[i].pbCert;
        cbCertEncoded = rgServerCertChain[i].cbCert;

        pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, pbCertEncoded, cbCertEncoded);
        if (pCertContext)
        {
            CertFreeCertificateContext(pCertContext);
            pCertContext = NULL;
        }
    }

    if ((dwCertErrors & XHR_CERT_ERROR_REVOCATION_FAILED) != 0)
    {
        m_dwCertIgnoreFlags |= XHR_CERT_IGNORE_REVOCATION_FAILED;
        m_fRetry = TRUE;
    }

    if ((dwCertErrors & XHR_CERT_ERROR_UNKNOWN_CA) != 0)
    {
        m_dwCertIgnoreFlags |= XHR_CERT_IGNORE_UNKNOWN_CA;
        m_fRetry = TRUE;
    }

    if ((dwCertErrors & XHR_CERT_ERROR_CERT_CN_INVALID) != 0)
    {
        m_dwCertIgnoreFlags |= XHR_CERT_IGNORE_CERT_CN_INVALID;
        m_fRetry = TRUE;
    }

    if ((dwCertErrors & XHR_CERT_ERROR_CERT_DATE_INVALID) != 0)
    {
        m_dwCertIgnoreFlags |= XHR_CERT_IGNORE_CERT_DATE_INVALID;
        m_fRetry = TRUE;
    }

Exit:
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::OnClientCertificateRequested( 
    __RPC__in_opt IXMLHTTPRequest3 *pXHR,
    DWORD cIssuerList,
    __RPC__in_ecount_full_opt(cIssuerList) const WCHAR **rgpwszIssuerList
)
{
    UNREFERENCED_PARAMETER(pXHR);

    HRESULT hr = S_OK;

    if (cIssuerList == 0 || rgpwszIssuerList == NULL) goto Exit;

    hr = DuplicateIssuerList(cIssuerList, rgpwszIssuerList, &m_rgpwszIssuerList);

    if (FAILED(hr)) goto Exit;

    m_cIssuerList = cIssuerList;
    m_fRetry = TRUE;

Exit:
    return hr;
}

STDMETHODIMP MinXHttpRqstCallback::DuplicateIssuerList( 
    _In_ DWORD cIssuerList,
    _In_reads_(cIssuerList) const WCHAR **rgpwszIssuerList,
    _Out_ const WCHAR ***prgpwszDuplicateIssuerList
)
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    DWORD cchIssuer = 0;
    WCHAR *pwszIssuer = NULL;
    const WCHAR **rgpwszDuplicateIssuerList = NULL;

    if (cIssuerList == 0 || rgpwszIssuerList == NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    rgpwszDuplicateIssuerList = (const WCHAR**) new WCHAR*[cIssuerList];

    if (rgpwszDuplicateIssuerList == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    memset(rgpwszDuplicateIssuerList, 0, sizeof(WCHAR*) * cIssuerList);

    for (i = 0; i < cIssuerList && rgpwszIssuerList[i] != NULL; i++)
    {
        cchIssuer = (DWORD) wcslen(rgpwszIssuerList[i]);
        pwszIssuer = new WCHAR[cchIssuer + 1];

        if (pwszIssuer == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        wcscpy_s(pwszIssuer, cchIssuer + 1, rgpwszIssuerList[i]);
        (rgpwszDuplicateIssuerList)[i] = pwszIssuer;
    }

Exit:

    if (FAILED(hr))
    {
        FreeIssuerList(i, rgpwszDuplicateIssuerList);
        rgpwszDuplicateIssuerList = NULL;
    }

    *prgpwszDuplicateIssuerList = rgpwszDuplicateIssuerList;

    return hr;
}

STDMETHODIMP MinXHttpRqstCallback::FreeIssuerList(
    _In_ DWORD cIssuerList,
    _Frees_ptr_ const WCHAR **rgpwszIssuerList
)
{
    DWORD i = 0;

    if (cIssuerList == 0 || rgpwszIssuerList == NULL)
    {
        goto Exit;
    }

    for (i = 0; i < cIssuerList; i++)
    {
        delete[] rgpwszIssuerList[i];
        rgpwszIssuerList[i] = NULL;
    }

    delete[] rgpwszIssuerList;

Exit:
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::CompareIssuer(
    _In_ VOID *pvCertContext,
    _In_ DWORD cIssuerList,
    _In_reads_(cIssuerList) const WCHAR **rgpwszIssuerList,
    _Out_ BOOL *pfMatch
)
{
    DWORD i = 0;
    WCHAR *pwszName = NULL;
    DWORD cchName = 0;
    PCCERT_CONTEXT pCertContext = (PCCERT_CONTEXT) pvCertContext;

    *pfMatch = FALSE;

    cchName = CertNameToStr(X509_ASN_ENCODING,
                            &pCertContext->pCertInfo->Issuer,
                            CERT_SIMPLE_NAME_STR | CERT_NAME_STR_CRLF_FLAG | CERT_NAME_STR_NO_PLUS_FLAG,
                            NULL,
                            0);

    pwszName = new WCHAR[cchName];
    if (pwszName == NULL) goto Exit;

    CertNameToStr(X509_ASN_ENCODING,
                  &pCertContext->pCertInfo->Issuer,
                  CERT_SIMPLE_NAME_STR | CERT_NAME_STR_CRLF_FLAG | CERT_NAME_STR_NO_PLUS_FLAG,
                  pwszName,
                  cchName);

    for (i = 0; i < cIssuerList; i++)
    {
        if (wcsncmp(pwszName, rgpwszIssuerList[i], cchName) == 0)
        {
            *pfMatch = TRUE;
            break;
        }
    }

Exit:
    if (pwszName != NULL) delete[] pwszName;
    return S_OK;
}

STDMETHODIMP MinXHttpRqstCallback::SelectCert(
    _In_ DWORD cIssuerList,
    _Frees_ptr_ const WCHAR **rgpwszIssuerList,
    _Inout_ DWORD *pcbCertHash,
    _Inout_updates_(*pcbCertHash) BYTE *pbCertHash
)
{
    HRESULT hr = S_OK;
    PCCERT_CONTEXT pCertContext = NULL;
    PCCERT_CONTEXT pSelectedContext = NULL;
    HCERTSTORE hMyCertStore = NULL;
    HCERTSTORE hMemStore = NULL;
    BOOL fMatch = FALSE;

    hMyCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,  X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,  NULL,  CERT_SYSTEM_STORE_CURRENT_USER,  L"MY");

    if (hMyCertStore == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    hMemStore = CertOpenStore(CERT_STORE_PROV_MEMORY,
                              X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                              NULL,
                              CERT_STORE_CREATE_NEW_FLAG,
                              NULL);

    if (hMemStore == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    for (pCertContext = CertEnumCertificatesInStore(hMyCertStore, pCertContext);
         pCertContext != NULL;
         pCertContext = CertEnumCertificatesInStore(hMyCertStore, pCertContext))
    {
        hr = CompareIssuer((VOID*) pCertContext, cIssuerList, rgpwszIssuerList, &fMatch);

        if (FAILED(hr) || !fMatch)
        {
            continue;
        }

        if (!CertAddCertificateContextToStore(hMemStore, pCertContext,  CERT_STORE_ADD_ALWAYS, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    }

    // (headless device, so unable to dialog with) pSelectedContext = CryptUIDlgSelectCertificateFromStore(hMemStore, NULL, NULL,  NULL,  0,  0, NULL);
    if (pSelectedContext == NULL)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (!CertGetCertificateContextProperty(pSelectedContext, CERT_HASH_PROP_ID, pbCertHash, pcbCertHash))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:

    if (pSelectedContext != NULL) CertFreeCertificateContext(pSelectedContext);
    if (pCertContext != NULL) CertFreeCertificateContext(pCertContext);
    if (hMemStore != NULL) CertCloseStore(hMemStore, 0);
    if (hMyCertStore != NULL) CertCloseStore(hMyCertStore, 0);
    FreeIssuerList(cIssuerList, rgpwszIssuerList);
    return hr;
}

STDMETHODIMP MinXHttpRqstCallback::GetCertResult(
    _Out_ BOOL *pfRetry,
    _Out_ DWORD *pdwCertIgnoreFlags,
    _Out_ DWORD *pcIssuerList,
    _Out_ const WCHAR ***prgpwszIssuerList
)
{
    HRESULT hr = S_OK;

    *pfRetry = m_fRetry;
    *pdwCertIgnoreFlags = m_dwCertIgnoreFlags;

    if (m_cIssuerList != 0 && m_rgpwszIssuerList != NULL)
    {
        *pcIssuerList = m_cIssuerList;
        hr = DuplicateIssuerList(m_cIssuerList, m_rgpwszIssuerList, prgpwszIssuerList);
    }
    else
    {
        *pcIssuerList = 0;
        *prgpwszIssuerList = NULL;
    }
    return hr;
}

STDMETHODIMP MinXHttpRqstCallback::WaitForComplete(
    _Out_ PDWORD pdwStatus
)
{
    DWORD dwError = ERROR_SUCCESS;
    HRESULT hr = S_OK;

    if (pdwStatus == NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwError = WaitForSingleObject(m_hComplete, INFINITE);

    if (dwError == WAIT_FAILED)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    else if (dwError != WAIT_OBJECT_0)
    {
        hr = E_ABORT;
        goto Exit;
    }

    if (FAILED(m_hr))
    {
        hr = m_hr;
        goto Exit;
    }

    hr = S_OK;
    *pdwStatus = m_dwStatus;

Exit:
    return hr;
}


MinXHttpRqstPostStream::MinXHttpRqstPostStream()
{
    m_hFile = INVALID_HANDLE_VALUE;
}

MinXHttpRqstPostStream::~MinXHttpRqstPostStream() 
{ 
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
}

STDMETHODIMP MinXHttpRqstPostStream::Open(_In_opt_ PCWSTR pcwszFileName)
{
    HRESULT hr = S_OK;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    if (pcwszFileName == NULL ||
        *pcwszFileName == L'\0')
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hFile = CreateFile(pcwszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0,  NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    m_hFile = hFile;
    hFile = INVALID_HANDLE_VALUE;

Exit:
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }
    return hr;
}

STDMETHODIMP MinXHttpRqstPostStream::Read(_Out_writes_bytes_to_(cb, *pcbRead)  void *pv, ULONG cb, _Out_opt_  ULONG *pcbRead)
{
    HRESULT hr = S_OK;
    BOOL fSuccess = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    DWORD cbRead = 0;

    if (pcbRead != NULL)
    {
        *pcbRead = 0;
    }

    if (pv == NULL || cb == 0)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    fSuccess = ReadFile(m_hFile, pv, cb, &cbRead, NULL);
    if (!fSuccess)
    {
        dwError = GetLastError();
        if (dwError != ERROR_HANDLE_EOF)
        {
            hr = HRESULT_FROM_WIN32(dwError);
            goto Exit;
        }
    }

    if (cbRead < cb) hr = S_FALSE;
    if (pcbRead != NULL) *pcbRead = cbRead;

Exit:
    return hr;
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
    HRESULT hr = S_OK;
    BOOL fSuccess = FALSE;
    LARGE_INTEGER liFileSize = {};

    if (pullSize == NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *pullSize = 0;

    fSuccess = GetFileSizeEx(m_hFile, &liFileSize);
    if (!fSuccess)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    *pullSize = (ULONGLONG)liFileSize.QuadPart;

Exit:
    return hr;
}
