#pragma once
#include <Windows.h>

class MinSerClass
{

public:
	MinSerClass();
	~MinSerClass();

    HRESULT MinSerClass::Open(wchar_t *ComPortName, long baud = 9600, int par = 'N', int dat = 8, int stop = 1, unsigned Flags = 0);
    void Close(void);
	void SchedWrite(BYTE *buf, int len, DWORD timeout = 1000);
	void SchedRead(BYTE *buf, int bufsz, DWORD timeout = 1000);

    bool IoComplete(void);
    int WaitToComplete(DWORD msecs);

	bool IoBusy (void);
    int  WaitToIdle(DWORD msecs);

    void FlushOutput(void);

private:
	// library control
	bool _workthread_active;
	bool _showlog;
    HANDLE _ThreadWaitObj;

    static DWORD WINAPI WorkThread(_In_  LPVOID lpParameter);
    const wchar_t * GetLastErrorToString(DWORD err = 0);
    void PrtfLog(wchar_t *ctrlp, ...);

    // I/O control
    HANDLE _HFile;
    OVERLAPPED _RWOlapEvent;
    BYTE *_IOBufp;
    int _IOBufLen;

    enum IODirection { None, Read, Write };
    IODirection _IODirection;

    enum IOState { Idle, Busy, Complete};
    IOState _IOState;
    DWORD _IOTimeout;

};