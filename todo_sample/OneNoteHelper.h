#pragma once

#include <list>
#include <string>

class OneNoteHelper 
{
private:
    std::wstring _AuthCode;
    std::wstring _Notebook;
    std::wstring _Section;
    std::wstring _Page;

    std::wstring _OneNoteUri;

    bool MemberOf(std::wstring &id, std::list<std::wstring> &set);
    const char * FindTitleId(_In_ const char *Content, _In_ std::wstring &Title, _Out_ std::wstring &Id);
    void ReadAuthFile(wchar_t *fname, std::wstring &Auth);

public:
    bool _showLog;
    char _buf[50 * 1024];

    OneNoteHelper();
    void OpenNotebook(_In_ const wchar_t *Notebook, _In_ const wchar_t * Section, _In_ const wchar_t * Page, _In_ const wchar_t * AuthCode);
    void CloseNotebook(void);
    char * PageRead(_Inout_ char *buffer, _In_ unsigned long bufsz);
    char * PageRead(_Inout_ char *buffer, _In_ unsigned long bufsz, std::list<std::wstring> &skipIDs);
    char * GetPageIDs(_Inout_ char *buffer, _In_ unsigned long bufsz, std::list<std::wstring> &skipIDs);
    HRESULT PageWrite(_Inout_ const char *content);
    void StripMarkup(_Inout_ char *buffer, _In_ unsigned long bufsz);
};