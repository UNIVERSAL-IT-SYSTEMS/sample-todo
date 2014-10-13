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

const int buttonPin = 2;     // the number of the pushbutton pin

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status

OneNoteHelper *One;
std::list<std::wstring> skipIDs;
MinSerClass * msc = nullptr;
int idlecount = 0;

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
    // Read a page
    char * p = One->PageRead(NULL, 0, skipIDs);

    if (force || (p != NULL && *p != 0))
    {
        // Print it
        One->StripMarkup(One->_buf, _countof(One->_buf));
        if (msc->Open(L"\\\\.\\COM2") == S_OK) {
            const char trailer[] = "\r\n\r\n\r\n----------\r\n\r\n\r\n";
            strcat_s(One->_buf, _countof(One->_buf), trailer);
            msc->SchedWrite((BYTE*)One->_buf, strlen(One->_buf));
            int ok = msc->WaitToComplete(10000);
        }
    }

    return (p != NULL && *p != 0);
}


void setup()
{
    pinMode(buttonPin, INPUT);

    One = new OneNoteHelper();
    One->_showLog = true;
    One->OpenNotebook(NULL, NULL, L"TODO", NULL);
    One->GetPageIDs(NULL, 0, skipIDs);
    msc = new MinSerClass();

}

// the loop routine runs over and over again forever:
void loop()
{
    delay(100);

    // read the state of the pushbutton value:
    buttonState = digitalRead(buttonPin);

    // check if the pushbutton is pressed.
    // if it is, the buttonState is HIGH:
    if (buttonState == LOW) {
        Log(L"Pushbutton pressed .. \n");
        std::string msg;
        PostToDo();
        PrintToDo(true);
    }
}

