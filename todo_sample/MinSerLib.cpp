// Copyright(c) Microsoft Open Technologies, Inc. All rights reserved. 
// Licensed under the BSD 2 - Clause License. 
// See License.txt in the project root for license information. 

#include "stdafx.h"

#include "MinSerLib.h"
#include <stdlib.h>
#include <setupapi.h>
#include <limits.h>

MinSerClass::MinSerClass()
{
    _showlog = true;
    _HFile = INVALID_HANDLE_VALUE;

    _workthread_active = false;
    _ThreadWaitObj = CreateEvent(
        NULL,      // security attributes
        FALSE,     // WaitForSingleObject() resets this when it completes
        FALSE,     // not set
        NULL);     // name
    CreateThread(NULL, 0, WorkThread, this, 0, NULL);

    memset(&_RWOlapEvent, 0, sizeof(_RWOlapEvent));
    _RWOlapEvent.hEvent = CreateEvent(
        NULL,  // security attributes
        TRUE,  // ReadFile(),WriteFile() will reset this manual event
        FALSE, // not set
        NULL); // name

}

MinSerClass::~MinSerClass()
{
    CloseHandle(_ThreadWaitObj);
    CloseHandle(_RWOlapEvent.hEvent);
    Close();
}


HRESULT MinSerClass::Open(wchar_t *ComPortName, long baud, int par, int dat, int stop, unsigned Flags)
{
    HRESULT retval = S_FALSE;
    UNREFERENCED_PARAMETER(Flags);  // Future option for flow-control settings

    Close();
    _HFile = CreateFile(ComPortName, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (_HFile == INVALID_HANDLE_VALUE) {
        PrtfLog(L"MinSer Error: Open(): CreateFile %s\n", GetLastErrorToString());
        return S_FALSE;
    }
    retval = S_OK;

    switch (toupper(par)) {
    case 'N': par = NOPARITY; break;
    case 'O': par = ODDPARITY; break;
    case 'E': par = EVENPARITY; break;
    default:  par = NOPARITY; break;
    }
    switch (stop) {
    case 1:  stop = ONESTOPBIT; break;
    case 2:  stop = TWOSTOPBITS; break;
    default: stop = ONESTOPBIT; break;
    }

    SetupComm(_HFile, 4096, 4096); /* 'suggest' 4k buffer for O/S for input and output */
    if (!SetCommMask(_HFile, EV_RXCHAR)) { 
        PrtfLog (L"MinSer Error: SetCommMask() %s\n", GetLastErrorToString()); 
        retval = S_FALSE;
    }

    DCB dcb;
    if (!GetCommState(_HFile, &dcb)) {
        PrtfLog (L"MinSer Error: GetCommState() %s\n", GetLastErrorToString());
        retval = S_FALSE;
    }
    dcb.BaudRate = baud;
    dcb.ByteSize = dat;
    dcb.Parity = par;
    dcb.StopBits = stop;
    dcb.fBinary = TRUE;
    dcb.fParity = TRUE;
    dcb.fOutxCtsFlow = TRUE;  
    dcb.fOutxDsrFlow = FALSE; 
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = TRUE;
    dcb.fOutX = TRUE;  
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fAbortOnError = FALSE;
    dcb.XonChar = 0x11;
    dcb.XoffChar = 0x13;
    if (!SetCommState(_HFile, &dcb)) { 
        PrtfLog (L"MinSer Error: SetCommState() %s\n", GetLastErrorToString());
        retval = S_FALSE;
    }

    COMMTIMEOUTS comtos;
    GetCommTimeouts(_HFile, &comtos);
    comtos.ReadIntervalTimeout = MAXDWORD;
    comtos.ReadTotalTimeoutMultiplier = 0;
    comtos.ReadTotalTimeoutConstant = 0;
    SetCommTimeouts(_HFile, &comtos);

    if (retval != S_OK && _HFile != INVALID_HANDLE_VALUE) CloseHandle(_HFile);
    return retval;
}

void MinSerClass::Close(void)
{
    if (_HFile != INVALID_HANDLE_VALUE) {
        CloseHandle(_HFile);
        _HFile = INVALID_HANDLE_VALUE;
    }
}


void MinSerClass::SchedWrite(BYTE *buf, int len, DWORD timeout)
{
    if (WaitToIdle(4000) != S_OK) {
        PrtfLog(L"MinSer Error: SchedWrite() previous operation was not completed.\n");
        return;
    }
    _IOState = Idle;
    _IODirection = Write;
    _IOBufp = buf;
    _IOBufLen = len;
    _IOTimeout = timeout;
    SetEvent(_ThreadWaitObj);
}

void MinSerClass::SchedRead(BYTE *buf, int bufsz, DWORD timeout)
{
    if (WaitToIdle(4000) != S_OK) {
        PrtfLog(L"MinSer Error: SchedRead() previous operation was not completed.\n");
        return;
    }
    _IOState = Idle;
    _IODirection = Read;
    _IOBufp = buf;
    _IOBufLen = bufsz;
    _IOTimeout = timeout;
    SetEvent(_ThreadWaitObj);
}

bool MinSerClass::IoBusy(void)
{
    return _IOState == Busy;
}

int MinSerClass::WaitToIdle(DWORD msecs)
{
    DWORD starttick = GetTickCount();
    for (;;) {
        if (!IoBusy()) return S_OK;
        if ((GetTickCount() - starttick) >= msecs) break;
        Sleep(20);
    }
    return S_FALSE;
}

bool MinSerClass::IoComplete(void)
{
    return _IOState == Complete;
}

int MinSerClass::WaitToComplete(DWORD msecs)
{
    DWORD starttick = GetTickCount();
    for (;;) {
        if (!IoComplete()) return S_OK;
        if ((GetTickCount() - starttick) >= msecs) break;
        Sleep(20);
    }
    return S_FALSE;
}

void MinSerClass::FlushOutput()
{
    WaitToIdle(1000);
    FlushFileBuffers(_HFile);
}

#pragma region private_routine


DWORD WINAPI MinSerClass::WorkThread(_In_  LPVOID lpParameter)
{
    MinSerClass *UMPtr = (MinSerClass *)lpParameter;
    UMPtr->_workthread_active = true;
    UMPtr->_IOState = Idle;

    for (;;) {
        // wait
        WaitForSingleObject(UMPtr->_ThreadWaitObj, INFINITE);
        if (!UMPtr->_workthread_active) break;

        // R/W, overlapped
        DWORD Count = 0;
        DWORD Error = 0;
        BOOL Ok = false; 

        switch (UMPtr->_IODirection) {
        case Read: 
            Ok = ReadFile(UMPtr->_HFile, UMPtr->_IOBufp, UMPtr->_IOBufLen, &Count, &(UMPtr->_RWOlapEvent));
            break;
        case Write: 
            Ok = WriteFile(UMPtr->_HFile, UMPtr->_IOBufp, UMPtr->_IOBufLen, &Count, &(UMPtr->_RWOlapEvent));
            break;
        default: 
            continue;
        }
        if (Ok) goto DoneState; // Error = 0; Count may be 0

        Count = 0;
        Error = GetLastError();
        if (Error != ERROR_IO_PENDING) {
            UMPtr->PrtfLog(L"MinSer Error during IO = %s ", UMPtr->GetLastErrorToString(Error));
            goto DoneState; // Error is set, Count = 0;
        }

        if (!GetOverlappedResult(UMPtr->_HFile, &(UMPtr->_RWOlapEvent), &Count, TRUE)) {
            Count = 0;
            Error = GetLastError();
            if (Error == ERROR_OPERATION_ABORTED) Error = 0; // Timeout, followed by CancelIo results in 995 = ERROR_OPERATION_ABORTED
            if (Error != 0) UMPtr->PrtfLog(L" MinSer Error from Overlapped Result = %s", UMPtr->GetLastErrorToString(Error));
        }

    DoneState: 
        UMPtr->_IOState = Complete;
        continue;

    }
    Sleep(200);
    return 0;
}

const wchar_t * MinSerClass::GetLastErrorToString(DWORD err)
{
    static wchar_t msgbuf[1024]; // not thread safe string
    if (err == 0) err = GetLastError();
    swprintf_s(msgbuf, _countof(msgbuf), _T("Err=%d:"), err);
    int n = (int)wcslen(msgbuf);
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, LANG_USER_DEFAULT, &(msgbuf[n]), _countof(msgbuf) - n, NULL);

    wchar_t *p;
    p = wcschr(msgbuf, '\r'); if (p != nullptr) *p = 0;
    p = wcschr(msgbuf, '\n'); if (p != nullptr) *p = 0;

    return msgbuf;
}


void MinSerClass::PrtfLog(wchar_t *ctrlp, ...)
{
    if (!_showlog) return;

    va_list marker;

    va_start(marker, ctrlp);
    int len = _vscwprintf(ctrlp, marker) + 1;
    wchar_t * buffer = new wchar_t[len];
    if (buffer != NULL)
    {
        len = vswprintf_s(buffer, len, ctrlp, marker);
        //wprintf(buffer);
        if (IsDebuggerPresent()) OutputDebugStringW(buffer);
        delete[](buffer);
    } else {
        vwprintf(ctrlp, marker);
    }
    va_end(marker);
}


#pragma endregion private_routines