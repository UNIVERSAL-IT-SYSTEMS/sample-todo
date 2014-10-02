[Sample] Button press to "Milk" on One Note shopping list.

**Description**

This sample demonstrates a physical input (a push button) interpreted to be a request (out of milk) and putting the request on a One Note grocery list.

**Architecture**

The implementation uses a client server model where the device(Galileo) is the client and the PC which it is connected to acts as the server.

PushButton.sln is the client program running on device which uses a socket (SockWrite.h) to establish connection to the Server (Minserver.sln) and then send a message to server. As soon as the server gets the message it runs OneNoteApp.sln which posts "Buy Milk" to Shopping List in user's OneNote.

**How to execute Sample**

Prerequisites

User should have OneNote on their PC with notebook named "Notes" and section "Tasks".
User should add the name of the PC to connect to in PushButton/Main.cpp

Wiring diagram to connect PushButton to Galileo is shown below.
![](http://1drv.ms/1v8503g)

Pushbutton.sln runs on device.
MinServer and OneNoteApp is a multi project CS solution with MinServer as startup project.
The project is designed so that the device and the Server PC is connected to the same network hub/switch.