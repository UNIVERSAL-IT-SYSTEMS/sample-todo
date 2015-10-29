
#pragma once

#include <string>
#include <list>

using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Web::Http;
using namespace Windows::Web::Http::Filters;
using namespace Windows::Web::Http::Headers;

class NameValuePair {
public:
    std::wstring name;
    std::wstring value;
};


class MinHttpGP {
 private:

     Windows::Web::Http::Filters::HttpBaseProtocolFilter^ filter;
     Windows::Web::Http::HttpClient^ httpClient;
     void MinHttpGP::PrtfLogI(_In_ Platform::String^ str);
     void MinHttpGP::LogPageI(_In_ Platform::String^ str);

 public:
    bool _showlog;
    void MinHttpGP::PrtfLog(_In_ const wchar_t *wstrp);

    MinHttpGP();
    ~MinHttpGP();

    void MinHttpGP::SetHeaderAuthorization(std::wstring *wname, std::wstring *wvalue);
    void MinHttpGP::GetRqst(_In_ const wchar_t *url, _Inout_ std::wstring *respStr);
    void MinHttpGP::PostRqst(_In_ const wchar_t *url, _In_ const wchar_t *content, _In_ const wchar_t *contentType, _Inout_ std::wstring *respStr);

};
