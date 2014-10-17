
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

    // ToDo:  Maintain valid auth-code with Authentication Server
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
            if (strncmp(p, title.c_str(), title.length()) == 0) break;
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

bool OneNoteHelper::MemberOf(std::wstring &id, std::list<std::wstring> &set)
{
    for each (std::wstring s in set) {
        if (s.compare(id) == 0) return true;
    }
    return false;
}

char * OneNoteHelper::GetPageIDs(_Inout_ char *buffer, _In_ unsigned long bufsz, std::list<std::wstring> &skipIDs)
{
    if (buffer == NULL || bufsz < 1)
    {
        buffer = _buf;
        bufsz = _countof(_buf);
    }
    *buffer = 0;

    NameValuePair nvp;
    nvp.name = L"Authorization";
    nvp.value = L"Bearer " + _AuthCode;
    std::list<NameValuePair> Headers;
    Headers.push_back(nvp);

    unsigned long nrread;
    MinHttpGP *gp = new MinHttpGP();
    gp->_showlog = _showLog;
    if (_AuthCode.length() < 1) gp->PrtfLog(L"WARN: NO Authorization code specified\n");

    // first enumerate all page IDs in default notebook, to build appropriate '../api/Beta/pages/{ID}/content' URI
    gp->GetRqst(L"https://www.onenote.com/api/beta/pages", &Headers, buffer, bufsz, &nrread);
    if (nrread == 0) return buffer;

    // Get entry matching page
    std::wstring pageId = L"";
    const char *p = buffer;
    for (;;)
    {
        if (p == NULL || *p == 0) return S_OK;
        p = FindTitleId(p, _Page, pageId);
        skipIDs.push_back(pageId);
    }
    return buffer;
}

char * OneNoteHelper::PageRead(_Inout_ char *buffer, _In_ unsigned long bufsz, std::list<std::wstring> &skipIDs)
{
    if (buffer == NULL || bufsz < 1)
    {
        buffer = _buf;
        bufsz = _countof(_buf);
    }
    *buffer = 0;

    NameValuePair nvp;
    nvp.name = L"Authorization"; 
    nvp.value = L"Bearer " + _AuthCode;
    std::list<NameValuePair> Headers;
    Headers.push_back(nvp);

    unsigned long nrread;
    MinHttpGP *gp = new MinHttpGP();
    gp->_showlog = _showLog;
    if (_AuthCode.length() < 1) gp->PrtfLog(L"WARN: NO Authorization code specified\n");

    // first enumerate all page IDs in default notebook, to build appropriate '../api/Beta/pages/{ID}/content' URI
    gp->GetRqst(L"https://www.onenote.com/api/beta/pages", &Headers, buffer, bufsz, &nrread);
    if (nrread == 0) return buffer;

    // Get entry matching page
    std::wstring pageId = L"";
    const char *p = buffer;
    for (;;)
    {
        if (p == NULL || *p == 0) {
            *buffer = 0;
            return buffer;
        }
        p = FindTitleId(p, _Page, pageId);
        if (!MemberOf(pageId, skipIDs)) break;
    }
    _OneNoteUri = L"https://www.onenote.com/api/beta/pages/" + pageId + L"/content";
    skipIDs.push_back(pageId); // add this one to ones read already

    // get and return reply in buffer
    gp->GetRqst((WCHAR*)_OneNoteUri.c_str(), &Headers, buffer, bufsz, &nrread);   

    return buffer;
}

char * OneNoteHelper::PageRead(_Inout_ char *buffer, _In_ unsigned long bufsz)
{
    std::list<std::wstring> skipIDs;
    skipIDs.clear();
    return (PageRead(buffer, bufsz, skipIDs));
}


void OneNoteHelper::StripMarkup(_Inout_ char *buffer, _In_ unsigned long bufsz)
{
    if (buffer == NULL || bufsz < 1) return;

    std::string s;
    s.clear();
    bool skip = false;
    bool skipbin = false;
    char *p = buffer;
    for (unsigned long x = 0; x < bufsz && *p != 0; ++x, ++p)
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
            if (!skip && !skipbin) s += *p;
        }
    }
    s += "\n\n";
    strcpy_s(buffer, bufsz, s.c_str());
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

    unsigned long nrread;
    MinHttpGP *gp = new MinHttpGP();
    gp->_showlog = _showLog;
    if (_AuthCode.length() < 1) gp->PrtfLog(L"WARN: NO Authorization code specified\n");

    _OneNoteUri = L"https://www.onenote.com/api/v1.0/pages"; // creates new page in "Quick Notes" section of default notebook
    gp->PostRqst((WCHAR*)_OneNoteUri.c_str(), &Headers, content, NULL, 0, &nrread);

    return S_OK;
}

