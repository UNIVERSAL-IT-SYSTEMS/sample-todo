// Copyright(c) Microsoft Open Technologies, Inc. All rights reserved. 
// Licensed under the BSD 2 - Clause License. 
// See License.txt in the project root for license information. 

#include <fstream>

#include "MinHttpGP.h"
#include "OneNoteHelper.h"

OneNoteHelper::OneNoteHelper()
{
    _showLog = false;
}

void OneNoteHelper::OpenNotebook(_In_ const wchar_t *Notebook, _In_ const wchar_t * Section, _In_ const wchar_t * Page, _In_ const wchar_t * AuthCode)
{
    CloseNotebook();
    if (Notebook != NULL) _Notebook.assign(Notebook);
    if (Section != NULL) _Section.assign(Section);
    if (Page != NULL) _Page.assign(Page);
    if (AuthCode != NULL) _AuthCode.assign(AuthCode);

    std::wstring AuthStr;
    if (AuthCode == NULL || AuthCode[0] == 0) { // if no arg provided, see if authcode file exists
        ReadAuthFile(L"$auth$.txt", AuthStr);
        if (AuthStr.length() > 0) _AuthCode.assign(AuthStr);
    }
}

void OneNoteHelper::CloseNotebook(void) 
{
    _Notebook.clear();
    _Section.clear();
    _Page.clear();
    _AuthCode.clear();
    _OneNoteUri.clear();
}

void OneNoteHelper::ReadAuthFile(wchar_t *fname, std::wstring &Auth)
{
    Auth.clear();
    if (fname == NULL || fname[0] == 0) return;
    std::ifstream f_inp;
    f_inp.open(fname, std::ios::in | std::ios::binary);
    if (!f_inp.is_open()) return;
    for (;;) {
        wchar_t ch = (unsigned int)f_inp.get();
        if (f_inp.eof()) break;
        if (ch > ' ') Auth += ch;
    }
    f_inp.close();
}

const char * OneNoteHelper::FindTitleId(_In_ const char *Content, _In_ std::wstring &Title, _Out_ std::wstring &Id)
{
    Id.clear();
    const char *p = Content;
    if (Title.length() != 0)
    {
        std::string title(Title.begin(), Title.end());
        title += "\"";
        for (;;) {
            p = strstr(p, "\"title\":\"");
            if (p == NULL) return NULL;
            p += 9;
            if (_strnicmp(p, title.c_str(), title.length()) == 0) break;
        }
    }
    p = strstr(p, "\"id\":\"");
    if (p == NULL) return NULL;
    p += 6;
    while (*p != 0 && *p != '"') {
        Id += (wchar_t)(*p);
        ++p;
    }
    return p;
}

int OneNoteHelper::ParseErrorCode(_Inout_ std::string &respStr)
{

    size_t pos = respStr.find("\"error\":");
    if (pos == std::string::npos) return 0;
    pos += 8;
    
    pos = respStr.find("\"code\":\"", pos);
    if (pos == std::string::npos) return 0;
    pos += 8;

    std::string ecode;
    ecode.clear();
    while (pos < respStr.length()) {
        char ch = respStr[pos++];
        if (ch == '"') break;
        ecode += ch;
    }
    return atol(ecode.c_str());
}

bool OneNoteHelper::MemberOf(std::wstring &id, std::list<std::wstring> &set)
{
    for each (std::wstring s in set) {
        if (s.compare(id) == 0) return true;
    }
    return false;
}

void OneNoteHelper::GetPageIDs(std::list<std::wstring> &pageIDs)
{
    pageIDs.clear();

    NameValuePair nvp;
    nvp.name = L"Authorization";
    nvp.value = L"Bearer " + _AuthCode;
    std::list<NameValuePair> Headers;
    Headers.push_back(nvp);

    MinHttpGP *gp = new MinHttpGP();
    gp->_showlog = _showLog;
    if (_AuthCode.length() < 1) gp->PrtfLog(L"WARN: NO Authorization code specified\n");

    std::string respStr;
    gp->GetRqst(L"https://www.onenote.com/api/beta/pages", &Headers, &respStr);
    if (respStr.length() < 1) return;

    std::wstring pageId = L"";
    const char *p = respStr.c_str();
    for (;;)
    {
        if (p == NULL || *p == 0) break;
        p = FindTitleId(p, _Page, pageId);
        if (pageId.length() != 0) {
            pageIDs.push_back(pageId);
        }
    }
}

void OneNoteHelper::PageRead(_Inout_ std::string &respStr, std::list<std::wstring> &readIDs)
{
    respStr.clear();

    NameValuePair nvp;
    nvp.name = L"Authorization"; 
    nvp.value = L"Bearer " + _AuthCode;
    std::list<NameValuePair> Headers;
    Headers.push_back(nvp);

    MinHttpGP *gp = new MinHttpGP();
    gp->_showlog = _showLog;
    if (_AuthCode.length() < 1) gp->PrtfLog(L"WARN: NO Authorization code specified\n");

    // enumerate all page IDs in default notebook, to build appropriate '../api/Beta/pages/{ID}/content' URI
    gp->GetRqst(L"https://www.onenote.com/api/beta/pages", &Headers, &respStr);
    if (respStr.length() < 1) return;

    // Get entry matching page
    std::wstring pageId = L"";
    const char *p = respStr.c_str();
    for (;;)
    {
        if (p == NULL || *p == 0) {
            respStr.clear();
            return;
        }
        p = FindTitleId(p, _Page, pageId);
        if (!MemberOf(pageId, readIDs)) break;
    }
    _OneNoteUri = L"https://www.onenote.com/api/beta/pages/" + pageId + L"/content";
    readIDs.push_back(pageId); // add this one to ones read already

    // get and return reply in buffer
    int pageErr = 0;
    for (int tries = 0; tries < 5; ++tries)
    {
        gp->GetRqst((WCHAR*)_OneNoteUri.c_str(), &Headers, &respStr);
        pageErr = ParseErrorCode(respStr);
        if (pageErr == 0) break;
        gp->PrtfLog(L"WARN: Error with page request = %u\n", pageErr);
        Sleep(5000); // settle time - give OneNote time to finish with any newly added contents on this page - and retry
    }
}

void OneNoteHelper::PageRead(_Inout_ std::string &respStr)
{
    std::list<std::wstring> skipIDs;
    skipIDs.clear();
    PageRead(respStr, skipIDs);
}


void OneNoteHelper::StripMarkup(_Inout_ std::string &respStr)
{
    std::string s;
    s.clear();
    bool skip = false;
    bool skipbin = false;

    size_t x = 0;
    for (const char * p = respStr.c_str(); *p != 0 && x < respStr.length(); ++p, ++x)
    {
        if (*p == '<')
        {
            skip = true;
            if (strncmp(p, "<p>", 3) == 0) s += "\n";
        }
        else if (*p == '>')
        {
            skip = false;
        } 
        else if (*p < 0x20 || *p > 0x7f)
        {
            skipbin = true;
        }
        else
        {
            if (skipbin) s += "\n";
            skipbin = false;
            if (!skip) s += *p;
        }
    }
    s += "\n\n";
    respStr = s;
}

HRESULT OneNoteHelper::PageWrite(_Inout_ const char *content) 
{

    std::list<NameValuePair> Headers;
    NameValuePair nvp;

    nvp.name = L"Authorization";
    nvp.value = L"Bearer " + _AuthCode;
    Headers.push_back(nvp);

    nvp.name = L"Content-Type";
    nvp.value = L"Text/Html";
    Headers.push_back(nvp);

    MinHttpGP *gp = new MinHttpGP();
    gp->_showlog = _showLog;
    if (_AuthCode.length() < 1) gp->PrtfLog(L"WARN: NO Authorization code specified\n");

    _OneNoteUri = L"https://www.onenote.com/api/v1.0/pages"; // creates new page in "Quick Notes" section of default notebook
    gp->PostRqst((WCHAR*)_OneNoteUri.c_str(), &Headers, content, NULL);

    return S_OK;
}

