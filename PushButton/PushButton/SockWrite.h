#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <tchar.h>
#include <stdlib.h>

const wchar_t * GetLastWSAErrorToString(DWORD err = 0)
{
	static TCHAR msgbuf[1024]; // not thread safe string
	if (err == 0) err = WSAGetLastError();
	_stprintf_s(msgbuf, _countof(msgbuf), _T("Err=%d:"), err);
	int n = (int)_tcslen(msgbuf);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, LANG_USER_DEFAULT, &(msgbuf[n]), _countof(msgbuf) - n, NULL);
	return msgbuf;
}

bool SockWriteOnce(wchar_t *host, int port, BYTE *buffer, int buflen)
{
	bool success = false;
	WSADATA wsaData;
	ADDRINFOW *waddrinfop = nullptr;
	ADDRINFOW hints = { 0 };

	int dwresult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (dwresult != 0)
	{
		wprintf(L"WSAStartup failed with error: %d\n", dwresult);
		goto Exit;
	}
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	wchar_t portstr[32];
	_itow_s(port, portstr, _countof(portstr), 10);
	dwresult = GetAddrInfoW(host, portstr, &hints, &waddrinfop);
	if (dwresult != 0)
	{
		wprintf(L"getaddrinfo failed with error: %d\n", dwresult);
		goto Exit;
	}

	SOCKET _socket = INVALID_SOCKET;
	_socket = socket(waddrinfop->ai_family, waddrinfop->ai_socktype, waddrinfop->ai_protocol);
	if (_socket == INVALID_SOCKET)
	{
		wprintf(L"socket failed with error: %s\n", GetLastWSAErrorToString());
		goto Exit;
	}

	dwresult = connect(_socket, waddrinfop->ai_addr, waddrinfop->ai_addrlen);
	if (dwresult < 0) {
		DWORD err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) {
			wprintf(L"Error: Could not connect: sock=%u stat=%d err=%s\n", _socket, dwresult, GetLastWSAErrorToString(err));
			goto Exit;
		}
	}

	dwresult = send(_socket, (const char*)buffer, buflen, 0);
	if (dwresult == SOCKET_ERROR)
	{
		wprintf(L"send failed with error: %s\n", GetLastWSAErrorToString());
	}
	else {
		success = true;
	}


Exit:
	if (waddrinfop != nullptr) FreeAddrInfoW(waddrinfop);
	if (_socket != INVALID_SOCKET) closesocket(_socket);
	WSACleanup();
	return success;
}

