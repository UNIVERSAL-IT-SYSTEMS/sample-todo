// Copyright(c) Microsoft Open Technologies, Inc. All rights reserved. 
// Licensed under the BSD 2 - Clause License. 
// See License.txt in the project root for license information. 

// Main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "arduino.h"
#include "OneNoteHelper.h"
#include "MinSerLib.h"
#include <time.h>



int _tmain(int argc, _TCHAR* argv[])
{
    return RunArduinoSketch();
}

const int buttonPin = 2;     
int buttonState = 0;         
OneNoteHelper *One;
std::list<std::wstring> skipIDs;
MinSerClass * msc = nullptr;
int idlecount = 0;

const char trailer[] = "\r\n\r\n\r\n----------\r\n\r\n\r\n";
BYTE byteBuf[2048];

void PostToDo(void)
{
    // get current time
    char buf[80];
    time_t     now = time(0);
    struct tm  tstruct;
    localtime_s(&tstruct, &now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    // Write a page
    std::string message = "";
    message += "<!DOCTYPE html><html><head><title>TODO</title><meta name = \"created\" content = \"2014-10-13T07:00:00.000-7:00\" /></head>";
    message += "<body>";
    message += "<p>";
    message += buf;
    message += "</p>";
    message += "<p>Buy: milk, bread<br/>Pick up: laundry, dog<br/>Clean: floors, car<br>Fix: sink, door<br/>Appt: 6pm football</p>";
    message += "</body>";
    message += "</html>";
    One->PageWrite(message.c_str());
}

bool PrintToDo(bool force)
{
    std::string respStr;

    // Read a page
    One->PageRead(respStr, skipIDs);

    if (force || respStr.length())
    {
        // Print it
        One->StripMarkup(respStr);
        if (msc->Open(L"\\\\.\\COM2") == S_OK) {
            respStr += trailer;
            strcpy_s((char*)byteBuf, _countof(byteBuf), respStr.c_str());
            msc->SchedWrite(byteBuf, respStr.length());
            int ok = msc->WaitToComplete(10000);
        }
    }

    return (respStr.length() != 0);
}


void setup()
{
    pinMode(buttonPin, INPUT);

    One = new OneNoteHelper();
    One->_showLog = true;
    One->OpenNotebook(NULL, NULL, L"TODO", NULL);
    One->GetPageIDs(skipIDs);
    msc = new MinSerClass();

}

// the loop routine runs over and over again forever:
void loop()
{
    delay(100);

    // read the state of the pushbutton value:
    buttonState = digitalRead(buttonPin);
    if (buttonState == LOW) {
        Log(L"Pushbutton pressed .. \n");
        PostToDo();
        PrintToDo(true);
    }
}

