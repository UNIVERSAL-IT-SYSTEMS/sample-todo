
#pragma once

#include <msxml6.h>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Details;

#define MAX_BUFFER_LENGTH (50*1024)

class MinXHttpRqstCallback :
    public Microsoft::WRL::RuntimeClass<RuntimeClassFlags<ClassicCom>, IXMLHTTPRequest3Callback>
{
private:
    HANDLE m_hComplete;
    HRESULT m_hr;
    DWORD m_dwStatus;
    BOOL m_fRetry;
    DWORD m_dwCertIgnoreFlags;
    DWORD m_cIssuerList;
    const WCHAR **m_rgpwszIssuerList;

    MinXHttpRqstCallback();
    ~MinXHttpRqstCallback();
    STDMETHODIMP RuntimeClassInitialize();
    friend HRESULT Microsoft::WRL::Details::MakeAndInitialize<MinXHttpRqstCallback,MinXHttpRqstCallback>(MinXHttpRqstCallback **);
    STDMETHODIMP DuplicateIssuerList( _In_ DWORD cIssuerList, _In_reads_(cIssuerList) const WCHAR **rgpwszIssuerList, _Out_ const WCHAR ***prgpwszDuplicateIssuerList );
    STDMETHODIMP FreeIssuerList( _In_ DWORD cIssuerList, _Frees_ptr_ const WCHAR **rgpwszIssuerList );
    STDMETHODIMP CompareIssuer( _In_ VOID *pvCertContext, _In_ DWORD cIssuerList, _In_reads_(cIssuerList) const WCHAR **rgpwszIssuerList, _Out_ BOOL *pfMatch );

public:
    char m_resptxt[MAX_BUFFER_LENGTH];
    ULONG m_resptxtlen;
    wchar_t m_hdrtxt[MAX_BUFFER_LENGTH];
    ULONG m_hdrtxtlen;

    STDMETHODIMP OnRedirect( __RPC__in_opt IXMLHTTPRequest2 *pXHR,  __RPC__in_string const WCHAR *pwszRedirectUrl );
    STDMETHODIMP OnHeadersAvailable( __RPC__in_opt IXMLHTTPRequest2 *pXHR, DWORD dwStatus,  __RPC__in_string const WCHAR *pwszStatus );
    STDMETHODIMP OnDataAvailable( __RPC__in_opt IXMLHTTPRequest2 *pXHR,  __RPC__in_opt ISequentialStream *pResponseStream  );
    STDMETHODIMP OnResponseReceived( __RPC__in_opt IXMLHTTPRequest2 *pXHR,  __RPC__in_opt ISequentialStream *pResponseStream );
    STDMETHODIMP OnError(  __RPC__in_opt IXMLHTTPRequest2 *pXHR,  HRESULT hrError  );
    STDMETHODIMP OnServerCertificateReceived( __RPC__in_opt IXMLHTTPRequest3 *pXHR, DWORD dwCertErrors, DWORD cServerCertChain, __RPC__in_ecount_full_opt(cServerCertChain) const XHR_CERT *rgServerCertChain );
    STDMETHODIMP OnClientCertificateRequested( __RPC__in_opt IXMLHTTPRequest3 *pXHR, DWORD cIssuerList,  __RPC__in_ecount_full_opt(cIssuerList) const WCHAR **rgpwszIssuerList );
    STDMETHODIMP SelectCert( _In_ DWORD cIssuerList, _Frees_ptr_ const WCHAR **rgpwszIssuerList, _Inout_ DWORD *pcbCertHash, _Inout_updates_(*pcbCertHash) BYTE *pbCertHash );
    STDMETHODIMP GetCertResult( _Out_ BOOL *pfRetry, _Out_ DWORD *pdwCertIgnoreFlags, _Out_ DWORD *pcIssuerList, _Out_ const WCHAR ***prgpwszIssuerList  );
    STDMETHODIMP ReadFromStream( _In_opt_ ISequentialStream *pStream );
    STDMETHODIMP WaitForComplete( _Out_ PDWORD pdwStatus );
};

class MinXHttpRqstPostStream : public Microsoft::WRL::RuntimeClass<RuntimeClassFlags<ClassicCom>, ISequentialStream>
{
private:
    HANDLE m_hFile;
    MinXHttpRqstPostStream();
    ~MinXHttpRqstPostStream();
    friend Microsoft::WRL::ComPtr<MinXHttpRqstPostStream> Microsoft::WRL::Details::Make<MinXHttpRqstPostStream>();
public:
    STDMETHODIMP  Open(_In_opt_ PCWSTR pcwszFileName);
    STDMETHODIMP  Read(_Out_writes_bytes_to_(cb, *pcbRead)  void *pv, ULONG cb, _Out_opt_  ULONG *pcbRead);
    STDMETHODIMP  Write(_In_reads_bytes_(cb)  const void *pv, ULONG cb, _Out_opt_  ULONG *pcbWritten);
    STDMETHODIMP  GetSize(_Out_ ULONGLONG *pullSize);
};

