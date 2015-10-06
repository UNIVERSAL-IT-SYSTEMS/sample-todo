#include "MinHttpGP.h"

MinHttpGP::MinHttpGP()
{
    _showlog = true;

    // create httpClient object
    filter = ref new HttpBaseProtocolFilter();
    filter->CacheControl->WriteBehavior = HttpCacheWriteBehavior::NoCache;
    filter->CacheControl->ReadBehavior = HttpCacheReadBehavior::MostRecent;
    httpClient = ref new HttpClient(filter);
}

MinHttpGP::~MinHttpGP()
{
}

void MinHttpGP::PrtfLog(_In_ const wchar_t *wstrp)
{
    if (!_showlog) return;
    if (IsDebuggerPresent()) {
        OutputDebugStringW(wstrp);
    }
}

void MinHttpGP::PrtfLogI(_In_ Platform::String^ str)
{
    if (!_showlog) return;
    if (IsDebuggerPresent()) {
        OutputDebugStringW(str->Data());
    }
}

void MinHttpGP::LogPageI(_In_ Platform::String^ str)
{
    if (!_showlog) return;
    if (IsDebuggerPresent()) {
        OutputDebugStringW(L"--------------------\n");
        wchar_t tstr[2];
        for (const wchar_t *p = str->Data(); p != nullptr && *p != 0; ++p) {
            tstr[0] = *p;
            tstr[1] = 0;
            OutputDebugStringW(tstr);
        }
        OutputDebugStringW(L"\n");
    }
}

void MinHttpGP::SetHeaderAuthorization(std::wstring *wname, std::wstring *wvalue)
{
    if (wname == nullptr || wvalue == nullptr) return;
    try
    {
        Platform::String^ name = ref new Platform::String(wname->c_str());
        Platform::String^ value = ref new Platform::String(wvalue->c_str());
        httpClient->DefaultRequestHeaders->Authorization = ref new Headers::HttpCredentialsHeaderValue(name, value);
    }
    catch (Exception^ ex)
    {
        PrtfLogI("SetHeaderAuthorization() Exception: " + ex->Message + "\n");
    }
}

void MinHttpGP::GetRqst(_In_ const wchar_t *url, _Inout_ std::wstring *respStr)
{
    // asyn sequence to send http request, wait for response, gather data..
    try
    {
        Platform::String ^ urlstr = ref new Platform::String(url);
        if (urlstr == nullptr) return;
        Windows::Foundation::Uri^ uri = ref new Windows::Foundation::Uri(urlstr);
        create_task(httpClient->GetAsync(uri)).then([this,respStr](HttpResponseMessage^ response)
        {
                task<Platform::String^> readAsStringTask(response->Content->ReadAsStringAsync());
                readAsStringTask.then([this,respStr](Platform::String^ responseBodyAsText) {

                    LogPageI(responseBodyAsText);
                    if (respStr != nullptr) respStr->assign(responseBodyAsText->Data());

                }, task_continuation_context::use_current()).wait();

        }).wait(); // wait for reply - designed to block calling task - not meant for UI threads

    }
    catch (Exception^ ex)
    {
        PrtfLogI("GetRqst() Exception: " + ex->Message + "\n");
    }
}

void MinHttpGP::PostRqst(_In_ const wchar_t *url, _In_ const wchar_t *content, _In_ const wchar_t *contenttype, _Inout_ std::wstring *respStr)
{
    // asyn sequence to send http request, wait for response, gather data..
    try
    {
        Platform::String ^ urlstr = ref new Platform::String(url);
        if (urlstr == nullptr) return;
        Windows::Foundation::Uri^ uri = ref new Windows::Foundation::Uri(urlstr);

        Platform::String^ contentType = ref new Platform::String(contenttype);
        Platform::String^ contentStr = ref new Platform::String(content);
        Windows::Web::Http::IHttpContent^ httpContent = ref new HttpStringContent(contentStr, Windows::Storage::Streams::UnicodeEncoding::Utf8, contentType);
        
        create_task(httpClient->PostAsync(uri,httpContent)).then([this, respStr](HttpResponseMessage^ response)
        {
            task<Platform::String^> readAsStringTask(response->Content->ReadAsStringAsync());
            readAsStringTask.then([this, respStr](Platform::String^ responseBodyAsText) {
                if (responseBodyAsText->Length())
                {
                    PrtfLogI("Post response...");
                    LogPageI(responseBodyAsText);
                }
                if (respStr != nullptr) respStr->assign(responseBodyAsText->Data());
            }, task_continuation_context::use_current()).wait();
        }).wait(); // wait for reply - designed to block calling task - not meant for UI threads

    }
    catch (Exception^ ex)
    {
        PrtfLogI("PostRqst() Exception: " + ex->Message);
    }
}

