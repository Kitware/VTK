/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkSocketCommunicator.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkObjectFactory.h"

#ifdef _WIN32
#define WSA_VERSION MAKEWORD(1,1)
#define vtkCloseSocketMacro(sock) (closesocket(sock))
#else
#define vtkCloseSocketMacro(sock) (close(sock))
#endif

//------------------------------------------------------------------------------
vtkSocketCommunicator* vtkSocketCommunicator::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSocketCommunicator");
  if(ret)
    {
    return (vtkSocketCommunicator*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSocketCommunicator;
}

//----------------------------------------------------------------------------
vtkSocketCommunicator::vtkSocketCommunicator()
{
  this->Socket = -1;
  this->IsConnected = 0;
  this->NumberOfProcesses = 2;
  this->SwapBytesInReceivedData = 0;
  this->MaximumMessageSize = 16000;
}

//----------------------------------------------------------------------------
vtkSocketCommunicator::~vtkSocketCommunicator()
{
  if (this->IsConnected)
    {
    vtkCloseSocketMacro(this->Socket);
    }
}


//----------------------------------------------------------------------------
void vtkSocketCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkCommunicator::PrintSelf(os,indent);

  os << indent << "SwapBytesInReceivedData: " << this->SwapBytesInReceivedData
     << endl;
}

static inline int checkForError(int id, int maxId)
{
  if ( id == 0 )
    {
    vtkGenericWarningMacro("Can not connect to myself!");
    return 1;
    }
  else if ( id >= maxId )
    {
    vtkGenericWarningMacro("No port for process " << id << " exists.");
    return 1;
    }
  return 0;
}

template <class T>
static int SendMessage(T* data, int length, int tag, int sock, int maxSize)
{
  // Need to check the return value of these
  send(sock, (char *)&tag, sizeof(int), 0);

  int totalLength = length*sizeof(T);
  int sent;
  sent = send(sock, (char*)data, totalLength, 0);
  if (sent == -1)
    {
    vtkGenericWarningMacro("Could not send message.");
    return 0;
    }
  cout << "Sent: " << sent << endl;
  while ( sent < totalLength )
    {
    sent += send ( sock, (char*)data+sent, totalLength-sent, 0 );
    if (sent == -1)
      {
      vtkGenericWarningMacro("Could not send message.");
      return 0;
      }
    cout << "Sent: " << sent << endl;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(int *data, int length, int remoteProcessId, 
				int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  return SendMessage(data, length, tag, this->Socket , this->MaximumMessageSize);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(unsigned long *data, int length, 
				int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  return SendMessage(data, length, tag, this->Socket , this->MaximumMessageSize);
}
//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(char *data, int length, 
				int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  return SendMessage(data, length, tag, this->Socket, this->MaximumMessageSize);
}
//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(float *data, int length, 
				int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  return SendMessage(data, length, tag, this->Socket , this->MaximumMessageSize);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::ReceiveMessage( char *data, int size, int length,
                                           int tag )
{
  int recvTag=-1;

  // Need to check the return value of these
  recv( this->Socket, (char *)&recvTag, sizeof(int), MSG_PEEK );

  if ( this->SwapBytesInReceivedData )
    {
    vtkSwap4( &recvTag );
    }
  if ( recvTag != tag )
    {
    return 0;
    }
  
  if (recv( this->Socket, (char *)&recvTag, sizeof(int), 0 ) == -1)
    {
    vtkErrorMacro("Could not receive message.");
    return 0;
    }
  int totalLength = length * size;
  int received;
  cout << "Expecting " << totalLength << endl;

  received = recv( this->Socket, data, totalLength, 0 );
  if (received == -1)
    {
    vtkErrorMacro("Could not receive message.");
    return 0;
    }
  cout << "Received: " << received << endl;
  while ( received < totalLength )
    {
    received += recv( this->Socket, &(data[received]), totalLength-received, 0 );
    if (received == -1)
      {
      vtkErrorMacro("Could not receive message.");
      return 0;
      }
    cout << "Received: " << received << endl;
    }

  cout << "Expected " << totalLength << endl;
  cout << "Received: " << received << endl;
  
  // Unless we've dealing with chars, then check byte ordering
  if ( this->SwapBytesInReceivedData && size == 4 )
    {
    vtkSwap4Range( data, length );
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(int *data, int length, int remoteProcessId, 
				   int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  int retval = ReceiveMessage( (char *)data, sizeof(int), length, tag );

  if ( tag == vtkMultiProcessController::RMI_TAG )
    {
    data[2] = 1;
    }

  return retval;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(unsigned long *data, int length, 
				   int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  return ReceiveMessage( (char *)data, sizeof(unsigned long), length, tag );
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(char *data, int length, 
				   int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  return ReceiveMessage( (char *)data, sizeof(char), length, tag);
}

int vtkSocketCommunicator::Receive(float *data, int length, 
				   int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  return ReceiveMessage( (char *)data, sizeof(float), length, tag);
}


int vtkSocketCommunicator::WaitForConnection(int port, int timeout)
{

  if ( this->IsConnected )
    {
    vtkErrorMacro("Port " << 1 << " is occupied.");
    return 0;
    }

  int sock = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server;

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  if ( bind(sock, (sockaddr *)&server, sizeof(server)) )
    {
    vtkErrorMacro("Can not bind socket to port " << port);
    return 0;
    }
  listen(sock,1);
  this->Socket = accept(sock, 0, 0);
  if ( this->Socket == -1 )
    {
    vtkErrorMacro("Error in accept.");
    return 0;
    }
  vtkCloseSocketMacro(sock);
    
  this->IsConnected = 1;

  // Handshake to determine if the client machine has the same endianness
  char clientIsBE;
  if ( !ReceiveMessage( &clientIsBE, sizeof(char), 1,
                        vtkSocketController::ENDIAN_TAG) )
    {
    vtkErrorMacro("Endian handshake failed.");
    return 0;
    }
  vtkDebugMacro(<< "Client is " << ( clientIsBE ? "big" : "little" ) << "-endian");

#ifdef VTK_WORDS_BIGENDIAN
  char IAmBE = 1;
#else
  char IAmBE = 0;
#endif
  vtkDebugMacro(<< "I am " << ( IAmBE ? "big" : "little" ) << "-endian");
  SendMessage( &IAmBE, 1, vtkSocketController::ENDIAN_TAG, this->Socket , this->MaximumMessageSize );

  if ( clientIsBE != IAmBE )
    {
    this->SwapBytesInReceivedData = 1;
    }

  return 1;
}

void vtkSocketCommunicator::CloseConnection()
{
  if ( this->IsConnected )
    {
    vtkCloseSocketMacro(this->Socket);
    this->IsConnected = 0;
    }
}

int vtkSocketCommunicator::ConnectTo ( char* hostName, int port )
{

  if ( this->IsConnected )
    {
    vtkErrorMacro("Communicator port " << 1 << " is occupied.");
    return 0;
    }

  struct hostent* hp;
  hp = gethostbyname(hostName);
  if (!hp)
    {
    unsigned long addr = inet_addr(hostName);
    hp = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
    }
  if (!hp)
    {
    vtkErrorMacro("Unknown host: " << hostName);
    return 0;
    }

  this->Socket = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in name;
  name.sin_family = AF_INET;
  memcpy(&name.sin_addr, hp->h_addr, hp->h_length);
  name.sin_port = htons(port);

  if( connect(this->Socket, (sockaddr *)&name, sizeof(name)) < 0)
    {
    vtkErrorMacro("Can not connect to " << hostName << " on port " << port);
    return 0;
    }

  vtkDebugMacro("Connected to " << hostName << " on port " << port);
  this->IsConnected = 1;

  // Handshake to determine if the server machine has the same endianness
#ifdef VTK_WORDS_BIGENDIAN
  char IAmBE = 1;
#else
  char IAmBE = 0;
#endif
  vtkDebugMacro(<< "I am " << ( IAmBE ? "big" : "little" ) << "-endian");
  SendMessage( &IAmBE, 1, vtkSocketController::ENDIAN_TAG, this->Socket , this->MaximumMessageSize );

  char serverIsBE;
  if ( !ReceiveMessage( &serverIsBE, sizeof(char), 1,
                        vtkSocketController::ENDIAN_TAG ) )
    {
    vtkErrorMacro("Endian handshake failed.");
    return 0;
    }
  vtkDebugMacro(<< "Server is " << ( serverIsBE ? "big" : "little" ) << "-endian");

  if ( serverIsBE != IAmBE )
    {
    this->SwapBytesInReceivedData = 1;
    }

  return 1;
}
