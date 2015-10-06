#TODO Sample on Galileo

## Contents
- [Components](#components)
- [Connection](#Connection)
- [How to Run it](#how-to-run-it)
- [How The Code Is Broken Up](#how-the-code-is-broken-up)
- [Tips](#tips)

---

### Components
- Raspberry Pi 2
- Windows IoT Image on microSD card
- Push button (3 -pin switch)

---

### Connection

1. Push button or 3 -pin switch is connected to 5V, GND and GPIO pin 5 (pin 29) on the RPi2.
1. RPi2 is plugged in and connected to network.



---

### How to Run it

visit http://windowsondevices.com/en-US/win10/WiringProjectGuide.htm for detailed instructions on deployment

---

### How The Code is Broken Up
- TodoSample.ino
- MinHttpGP.cpp and .h
- OneNoteHelper.cpp and .h

**_TodoSample.ino_**

- 	Example setup() and loop() functions for working with OneNote pages (http GET/POST interface) and printing contents to a serial POS printer.

**_MinHttpGP_**

- 	A minimal object library for sending high level https:// GET and POST requests to a web server.  
-	In conjunction with MinXHttpRqst, shows how to use the built in WinINET com library.

**_OneNoteHelper_**

- 	A basic object library for Reading and Writing pages to OneNote cloud services.  OneNote services API online docs are [here](http://dev.onenote.com/docs) 
-	Getting started with OneNoteAPI, including how to sign in a user, and obtaining an OAuth token to include in page requests to the server, can be found in the MSDN reference [here](http://msdn.microsoft.com/en-us/library/office/dn575425(v=office.15).aspx) 


### Authentication

-	Specifics on developing for Live services (OneDrive, Outlook.com, etc.) can be found on [MSDN](http://msdn.microsoft.com/en-us/library/hh243641.aspx) 
-	A sample request URI for getting a temporary authentication code from live.com is below. This URI can be used with any browser, and includes client_id, scope, response_type, and redirect value for our sample project. The returned URI contains a fresh auth_token, which is placed in $auth$.txt alongside the executable on the device for our sample to access OneNote. 

https://login.live.com/oauth20_authorize.srf?client_id=000000004812E454&scope=office.onenote%20wl.signin%20wl.basic%20office.onenote_create&response_type=token&redirect_uri=https:%2F%2Flogin.live.com%2Foauth20_desktop.srf](https://login.live.com/oauth20_authorize.srf?client_id=000000004812E454&scope=office.onenote%20wl.signin%20wl.basic%20office.onenote_create&response_type=token&redirect_uri=https:%2F%2Flogin.live.com%2Foauth20_desktop.srf

For more instructions on how to obtain an access token to use with this sample, visit http://windowsondevices.com/en-US/win10/samples/OneNoteWiring.htm

#### Tips/ Notes
1. The authorization token is valid for one hour and will expire after that. User will need to acquire a new Authorization token as described above.
