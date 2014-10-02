using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;
using System.Linq;
using System.Xml.Linq;
using Microsoft.Office.Interop.OneNote;
using System.Diagnostics;
using System.ComponentModel;

namespace MinServer
{
    class Program
    {
       
        static void Main(string[] args)
        {
            const Int32 _Port = 8080;
            Socket server = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            IPEndPoint localEndPoint = new IPEndPoint(IPAddress.Any, _Port);
            server.Bind(localEndPoint);
            server.Listen(Int32.MaxValue);
            for (; ; )
            {
                Socket socket = server.Accept();
                new RequestHandler(socket);
            }

        }

       

        internal sealed class RequestHandler
        {
            private Socket _Socket;

            public RequestHandler(Socket socket)
            {
                _Socket = socket;
                new Thread(ProcessRequest).Start();
            }

            private void ProcessRequest()
            {
                using (_Socket)
                {
                    Byte[] _Buffer = new Byte[1024];
                    if (_Socket.Poll(5000000, SelectMode.SelectRead))
                    {
                        if (_Socket.Available == 0) return;
                        Int32 bytesRead = _Socket.Receive(_Buffer, _Socket.Available, SocketFlags.None);

                        String TempString = Encoding.UTF8.GetString(_Buffer, 0, bytesRead);
                        Console.WriteLine("Recieved: '" + TempString + "'");
                        
                        string myPath = System.Reflection.Assembly.GetEntryAssembly().Location;
                        string myDir = System.IO.Path.GetDirectoryName(myPath);
                        string path = System.IO.Path.Combine(myDir, "OnenoteApp.exe");
                        System.Diagnostics.Process.Start(path);

                      
                    }
                }
             }
          }

    }
}
