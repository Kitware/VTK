/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSocketCommunicator.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkObjectFactory.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
#define WSA_VERSION MAKEWORD(1,1)
#define vtkCloseSocketMacro(sock) (closesocket(sock))
#else
#define vtkCloseSocketMacro(sock) (close(sock))
#endif

// These macros are used to decrease the number of lines 
// not covered during testing (macros count as one line)
#define vtkSCCheckForError \
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) ) \
    { \
    return 0; \
    } 

#define vtkSCSendError \
  if (sent == -1) \
    { \
    vtkGenericWarningMacro("Could not send message."); \
    return 0; \
    } 

#define vtkSCReceiveError \
  if (received == -1) \
    { \
    vtkErrorMacro("Could not receive message."); \
    return 0; \
    }

vtkCxxRevisionMacro(vtkSocketCommunicator, "1.32");
vtkStandardNewMacro(vtkSocketCommunicator);

//----------------------------------------------------------------------------
vtkSocketCommunicator::vtkSocketCommunicator()
{
  this->Socket = -1;
  this->IsConnected = 0;
  this->NumberOfProcesses = 2;
  this->SwapBytesInReceivedData = 0;
  this->PerformHandshake = 1;
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
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SwapBytesInReceivedData: " << this->SwapBytesInReceivedData
     << endl;
  os << indent << "IsConnected: " << this->IsConnected << endl;
  os << indent << "Perform a handshake: " 
     << ( this->PerformHandshake ? "Yes" : "No" ) << endl;
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

static int SendMessage(char* data, int length, int tag, int sock)
{

  int sent, total = 0;

  total = send(sock, (char*)&tag, sizeof(int), 0);
  if (total == -1)
    {
    vtkGenericWarningMacro("Could not send tag.");
    return 0;
    }
  while ( total < (int)sizeof(int) )
    {
    sent = send ( sock, data + total, sizeof(int) - total, 0 );
    vtkSCSendError;
    total += sent;
    }

  total = send(sock, data, length, 0);
  if (total == -1)
    {
    vtkGenericWarningMacro("Could not send message.");
    return 0;
    }
  while ( total < length )
    {
    sent = send ( sock, data + total, length - total, 0 );
    vtkSCSendError;
    total += sent;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(int *data, int length, int remoteProcessId, 
                                int tag)
{
  vtkSCCheckForError;

  return SendMessage(reinterpret_cast<char*>(data), length*sizeof(int), 
                     tag, this->Socket);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(unsigned long *data, int length, 
                                int remoteProcessId, int tag)
{
  vtkSCCheckForError;

  return SendMessage(reinterpret_cast<char*>(data),length*sizeof(unsigned long), 
                     tag, this->Socket);
}
//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(char *data, int length, 
                                int remoteProcessId, int tag)
{
  vtkSCCheckForError;

  return SendMessage(reinterpret_cast<char*>(data), length, tag, this->Socket);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(unsigned char *data, int length, 
                                int remoteProcessId, int tag)
{
  vtkSCCheckForError;

  return SendMessage(reinterpret_cast<char*>(data), length, tag, this->Socket);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(float *data, int length, 
                                int remoteProcessId, int tag)
{
  vtkSCCheckForError;

  return SendMessage(reinterpret_cast<char*>(data), length*sizeof(float), 
                     tag, this->Socket);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(double *data, int length, 
                                int remoteProcessId, int tag)
{
  vtkSCCheckForError;

  return SendMessage(reinterpret_cast<char*>(data), length*sizeof(double), 
                     tag, this->Socket);
}

#ifdef VTK_USE_64BIT_IDS
//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(vtkIdType *data, int length, 
                                int remoteProcessId, int tag)
{
  vtkSCCheckForError;

  return SendMessage(reinterpret_cast<char*>(data), length*sizeof(vtkIdType), 
                     tag, this->Socket);
}
#endif

//----------------------------------------------------------------------------
int vtkSocketCommunicator::ReceiveMessage( char *data, int size, int length,
                                           int tag )
{
  int totalLength = length * size;
  int received, total;
  int recvTag = -1;
  char* charTag = (char*)&recvTag;

  total = recv( this->Socket, charTag, sizeof(int), MSG_PEEK );

  if ( total == -1 )
    {
    vtkErrorMacro("Could not receive tag.");
    return 0;
    }
  while ( total < (int)sizeof(int) )
    {
    received = recv( this->Socket, &(charTag[total]), sizeof(int) - total, 0 );
    vtkSCReceiveError;
    total += received;
    }

  if ( this->SwapBytesInReceivedData )
    {
    vtkSwap4( charTag );
    }
  if ( recvTag != tag )
    {
    return 0;
    }
  
  // Since we've already peeked at the entire tag, it must all be here, so
  // we should be able to get all of it in one try.
  if (recv( this->Socket, charTag, sizeof(int), 0 ) == -1)
    {
    vtkErrorMacro("Could not receive tag (even though it's already here).");
    return 0;
    }

  total = recv( this->Socket, data, totalLength, 0 );
  if (total == -1)
    {
    vtkErrorMacro("Could not receive message.");
    return 0;
    }
  while ( total < totalLength )
    {
    received = recv( this->Socket, &(data[total]), totalLength - total, 0 );
    vtkSCReceiveError;
    total += received;
    }

  // Unless we're dealing with chars, then check byte ordering
  // This is really bad and should probably use some enum for types
  if (this->SwapBytesInReceivedData)
    {
    if (size == 4)
      {
      vtkDebugMacro(<< " swapping 4 range, size = " << size 
      << " length = " << length);
      vtkSwap4Range(data, length);
      }
    else if (size == 8)
      {
      vtkDebugMacro(<< " swapping 8 range, size = " << size 
      << " length = " << length );
      vtkSwap8Range(data, length);
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::ReceiveMessage(char *data, int *length, 
                                          int maxlength)
{
  *length = recv( this->Socket, data, maxlength, 0 );

  return 1;
}
//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(int *data, int length, int remoteProcessId, 
                                   int tag)
{
  vtkSCCheckForError;

  int retval = this->ReceiveMessage( (char *)data, sizeof(int), length, tag );

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
  vtkSCCheckForError;

  return this->ReceiveMessage( (char *)data, sizeof(unsigned long), 
                               length, tag );
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(char *data, int length, 
                                   int remoteProcessId, int tag)
{
  vtkSCCheckForError;

  return this->ReceiveMessage( data, sizeof(char), length, tag);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(unsigned char *data, int length, 
                                   int remoteProcessId, int tag)
{
  vtkSCCheckForError;

  return this->ReceiveMessage( (char *)data, sizeof(char), length, tag);
}

int vtkSocketCommunicator::Receive(float *data, int length, 
                                   int remoteProcessId, int tag)
{
  vtkSCCheckForError;

  return this->ReceiveMessage( (char *)data, sizeof(float), length, tag);
}

int vtkSocketCommunicator::Receive(double *data, int length, 
                                   int remoteProcessId, int tag)
{
  vtkSCCheckForError;

  return this->ReceiveMessage( (char *)data, sizeof(double), length, tag);
}

#ifdef VTK_USE_64BIT_IDS
int vtkSocketCommunicator::Receive(vtkIdType *data, int length, 
                                   int remoteProcessId, int tag)
{
  vtkSCCheckForError;

  return this->ReceiveMessage( (char *)data, sizeof(vtkIdType), length, tag);
}
#endif

int vtkSocketCommunicator::WaitForConnection(int port)
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

  if ( this->PerformHandshake )
    {
    // Handshake to determine if the client machine has the same endianness
    char clientIsBE;
    if ( !this->ReceiveMessage( &clientIsBE, sizeof(char), 1,
                                vtkSocketController::ENDIAN_TAG) )
      {
      vtkErrorMacro("Endian handshake failed.");
      return 0;
      }
    vtkDebugMacro(<< "Client is " << ( clientIsBE ? "big" : "little" ) 
    << "-endian");
    
#ifdef VTK_WORDS_BIGENDIAN
    char IAmBE = 1;
#else
    char IAmBE = 0;
#endif
    vtkDebugMacro(<< "I am " << ( IAmBE ? "big" : "little" ) << "-endian");
    SendMessage( &IAmBE, 1, vtkSocketController::ENDIAN_TAG, this->Socket );
    
    if ( clientIsBE != IAmBE )
      {
      this->SwapBytesInReceivedData = 1;
      }
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
  SendMessage( &IAmBE, 1, vtkSocketController::ENDIAN_TAG, this->Socket );

  char serverIsBE;
  if ( !this->ReceiveMessage( &serverIsBE, sizeof(char), 1,
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
