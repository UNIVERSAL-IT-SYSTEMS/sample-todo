// Copyright(c) Microsoft Open Technologies, Inc. All rights reserved. 
// Licensed under the BSD 2 - Clause License. 
// See License.txt in the project root for license information. 

#pragma once

#include <list>
#include <string>

#include "MinHttpGP.h"

class OneNoteHelper
{
private:
    std::wstring _AuthCode;
    std::wstring _Notebook;
    std::wstring _Section;
    std::wstring _Page;

    std::wstring _OneNoteUri;

    HANDLE _wait_event;

    bool MemberOf( std::wstring &id, std::list<std::wstring> &set );
    const wchar_t * FindTitleId( _In_ const wchar_t *Content, _In_ std::wstring &Title, _Out_ std::wstring &Id );
    void ReadAuthFile( wchar_t *fname, std::wstring &Auth );

public:
    bool _showLog;

    OneNoteHelper();
    void OpenNotebook( _In_ const wchar_t *Notebook, _In_ const wchar_t * Section, _In_ const wchar_t * Page, _In_ const wchar_t * AuthCode );
    void CloseNotebook( void );
    void PageRead( _Inout_ std::wstring &respStr );
    void PageRead( _Inout_ std::wstring &respStr, std::list<std::wstring> &readIDs );
    void GetPageIDs( std::list<std::wstring> &pageIDs );
    void PageWrite( _Inout_ std::wstring &content );
    void StripMarkup( _Inout_ std::wstring &respStr );
    int ParseErrorCode( _Inout_ std::wstring &respStr );

};