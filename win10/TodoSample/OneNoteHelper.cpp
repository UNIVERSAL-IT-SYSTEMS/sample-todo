#include <fstream>

#include "OneNoteHelper.h"

OneNoteHelper::OneNoteHelper()
{
    _showLog = false;
    _wait_event = CreateEventEx( NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS );
}

void OneNoteHelper::OpenNotebook( _In_ const wchar_t *Notebook, _In_ const wchar_t * Section, _In_ const wchar_t * Page, _In_ const wchar_t * AuthCode )
{
    CloseNotebook();
    if( Notebook != NULL ) _Notebook.assign( Notebook );
    if( Section != NULL ) _Section.assign( Section );
    if( Page != NULL ) _Page.assign( Page );
    if( AuthCode != NULL ) _AuthCode.assign( AuthCode );

    std::wstring AuthStr;
    if( AuthCode == NULL || AuthCode[0] == 0 ) { // if no arg provided, see if authcode file exists
        ReadAuthFile( L"$auth$.txt", AuthStr );
        if( AuthStr.length() > 0 ) _AuthCode.assign( AuthStr );
    }
}

void OneNoteHelper::CloseNotebook( void )
{
    _Notebook.clear();
    _Section.clear();
    _Page.clear();
    _AuthCode.clear();
    _OneNoteUri.clear();
}

void OneNoteHelper::ReadAuthFile( wchar_t *fname, std::wstring &Auth )
{
    Auth.clear();
    if( fname == NULL || fname[0] == 0 ) return;
    std::ifstream f_inp;
    f_inp.open( fname, std::ios::in | std::ios::binary );
    if( !f_inp.is_open() ) return;
    for( ;;) {
        wchar_t ch = (unsigned int)f_inp.get();
        if( f_inp.eof() ) break;
        if( ch > ' ' ) Auth += ch;
    }
    f_inp.close();
}

const wchar_t * OneNoteHelper::FindTitleId( _In_ const wchar_t *Content, _In_ std::wstring &Title, _Out_ std::wstring &Id )
{
    Id.clear();
    const wchar_t *p = Content;
    if( Title.length() != 0 )
    {
        std::wstring title( Title.begin(), Title.end() );
        title += L"\"";
        for( ;;) {
            p = wcsstr( p, L"\"title\":\"" );
            if( p == NULL ) return NULL;
            p += 9;
            if( _wcsnicmp( p, title.c_str(), title.length() ) == 0 ) break;
        }
    }
    p = wcsstr( p, L"\"id\":\"" );
    if( p == NULL ) return NULL;
    p += 6;
    while( *p != 0 && *p != '"' ) {
        Id += (wchar_t)( *p );
        ++p;
    }
    return p;
}

int OneNoteHelper::ParseErrorCode( _Inout_ std::wstring &respStr )
{

    size_t pos = respStr.find( L"\"error\":" );
    if( pos == std::string::npos ) return 0;
    pos += 8;

    pos = respStr.find( L"\"code\":\"", pos );
    if( pos == std::string::npos ) return 0;
    pos += 8;

    std::wstring ecode;
    ecode.clear();
    while( pos < respStr.length() ) {
        wchar_t ch = respStr[pos++];
        if( ch == '"' ) break;
        ecode += ch;
    }
    return _wtol( ecode.c_str() );
}

bool OneNoteHelper::MemberOf( std::wstring &id, std::list<std::wstring> &set )
{
    for each ( std::wstring s in set ) {
        if( s.compare( id ) == 0 ) return true;
    }
    return false;
}

void OneNoteHelper::GetPageIDs( std::list<std::wstring> &pageIDs )
{
    pageIDs.clear();

    MinHttpGP *gp = new MinHttpGP();
    gp->_showlog = _showLog;
    std::wstring auth_name = L"Bearer";
    gp->SetHeaderAuthorization( &auth_name, &_AuthCode );
    if( _AuthCode.length() < 1 ) gp->PrtfLog( L"WARN: NO Authorization code specified\n" );

    std::wstring respStr;
    _OneNoteUri = L"https://www.onenote.com/api/beta/pages";
    gp->GetRqst( _OneNoteUri.c_str(), &respStr );
    if( respStr.length() < 1 ) return;

    std::wstring pageId = L"";
    const wchar_t *p = respStr.c_str();
    for( ;;)
    {
        if( p == NULL || *p == 0 ) break;
        p = FindTitleId( p, _Page, pageId );
        if( pageId.length() != 0 ) {
            pageIDs.push_back( pageId );
        }
    }
}

static void WaitTime( int ms )
{

}

void OneNoteHelper::PageRead( _Inout_ std::wstring &respStr, std::list<std::wstring> &readIDs )
{
    respStr.clear();

    MinHttpGP *gp = new MinHttpGP();
    gp->_showlog = _showLog;
    std::wstring auth_name = L"Bearer";
    gp->SetHeaderAuthorization( &auth_name, &_AuthCode );
    if( _AuthCode.length() < 1 ) gp->PrtfLog( L"WARN: NO Authorization code specified\n" );

    // enumerate all page IDs in default notebook, to build appropriate '../api/Beta/pages/{ID}/content' URI
    _OneNoteUri = L"https://www.onenote.com/api/beta/pages";
    gp->GetRqst( _OneNoteUri.c_str(), &respStr );
    if( respStr.length() < 1 ) return;

    // Get entry matching page
    std::wstring pageId = L"";
    const wchar_t *p = respStr.c_str();
    for( ;;)
    {
        if( p == NULL || *p == 0 ) {
            respStr.clear();
            return;
        }
        p = FindTitleId( p, _Page, pageId );
        if( !MemberOf( pageId, readIDs ) ) break;
    }
    _OneNoteUri = L"https://www.onenote.com/api/beta/pages/" + pageId + L"/content";
    readIDs.push_back( pageId ); // add this one to ones read already

                                 // get and return reply in buffer
    int pageErr = 0;
    for( int tries = 0; tries < 5; ++tries )
    {
        gp->GetRqst( _OneNoteUri.c_str(), &respStr );
        pageErr = ParseErrorCode( respStr );
        if( pageErr == 0 ) break;
        gp->PrtfLog( L"WARN: Error with page request = TODO:\n" );
        WaitForSingleObjectEx( _wait_event, 5000, false ); // settle time - give OneNote time to finish with any newly added contents on this page - and retry
    }
}

void OneNoteHelper::PageRead( _Inout_ std::wstring &respStr )
{
    std::list<std::wstring> skipIDs;
    skipIDs.clear();
    PageRead( respStr, skipIDs );
}


void OneNoteHelper::StripMarkup( _Inout_ std::wstring &respStr )
{
    std::wstring s;
    s.clear();
    bool skip = false;
    bool skipbin = false;

    size_t x = 0;
    for( const wchar_t * p = respStr.c_str(); *p != 0 && x < respStr.length(); ++p, ++x )
    {
        if( *p == '<' )
        {
            skip = true;
            if( wcsncmp( p, L"<p>", 3 ) == 0 ) s += L"\n";
        }
        else if( *p == '>' )
        {
            skip = false;
        }
        else if( *p < 0x20 || *p > 0x7f )
        {
            skipbin = true;
        }
        else
        {
            if( skipbin ) s += L"\n";
            skipbin = false;
            if( !skip ) s += *p;
        }
    }
    s += L"\n\n";
    respStr = s;
}

void OneNoteHelper::PageWrite( _Inout_ std::wstring &content )
{
    MinHttpGP *gp = new MinHttpGP();
    gp->_showlog = _showLog;
    std::wstring auth_name = L"Bearer";
    gp->SetHeaderAuthorization( &auth_name, &_AuthCode );
    if( _AuthCode.length() < 1 ) gp->PrtfLog( L"WARN: NO Authorization code specified\n" );

    _OneNoteUri = L"https://www.onenote.com/api/v1.0/pages"; // creates new page in "Quick Notes" section of default notebook
    gp->PostRqst( _OneNoteUri.c_str(), content.c_str(), L"Text/Html", NULL );

}

