
#include "stdafx.h"
#include "arduino.h"
#include "SockWrite.h"


int _tmain(int argc, _TCHAR* argv[])
{
	return RunArduinoSketch();
	
}

const int buttonPin = 2;     // the number of the pushbutton pin
const int ledPin = 13;      // the number of the LED pin

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status

void setup() {
	// initialize the LED pin as an output:
	pinMode(ledPin, OUTPUT);
	// initialize the pushbutton pin as an input:
	pinMode(buttonPin, INPUT);
}

void loop(){
	// read the state of the pushbutton value:
	buttonState = digitalRead(buttonPin);

	// check if the pushbutton is pressed.
	// if it is, the buttonState is HIGH:
	if (buttonState == HIGH) {
		// turn LED on:    
		digitalWrite(ledPin, HIGH);
		
	}
	else {
		// turn LED off:
		digitalWrite(ledPin, LOW);
		Log(L"Pushbutton pressed .. \n");
		char msg[] = "hello server - i am a galileo client and this is my message.";
		SockWriteOnce(L"169.254.130.199", 8080, (BYTE *)msg, _countof(msg));
		Sleep(5000);
	    
		}
}
