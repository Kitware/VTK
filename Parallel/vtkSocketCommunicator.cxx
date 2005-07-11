/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSocketCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSocketCommunicator.h"

#include "vtkObjectFactory.h"
#include "vtkSocketController.h"
#include "vtkCommand.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
# define VTK_WINDOWS_FULL
# include "vtkWindows.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif


#if defined(_WIN32) && !defined(__CYGWIN__)
#define WSA_VERSION MAKEWORD(1,1)
#define vtkCloseSocketMacro(sock) (closesocket(sock))
#else
#define vtkCloseSocketMacro(sock) (close(sock))
#endif

vtkCxxRevisionMacro(vtkSocketCommunicator, "1.59");
vtkStandardNewMacro(vtkSocketCommunicator);

//----------------------------------------------------------------------------
vtkSocketCommunicator::vtkSocketCommunicator()
{
  this->Socket = -1;
  this->IsConnected = 0;
  this->NumberOfProcesses = 2;
  this->SwapBytesInReceivedData = vtkSocketCommunicator::SwapNotSet;
  this->PerformHandshake = 1;
  this->LogStream = 0;
  this->LogFile = 0;

  this->ReportErrors = 1;
}

//----------------------------------------------------------------------------
vtkSocketCommunicator::~vtkSocketCommunicator()
{
  if (this->IsConnected)
    {
    vtkCloseSocketMacro(this->Socket);
    this->Socket = -1;
    }
  this->SetLogStream(0);
}

//----------------------------------------------------------------------------
void vtkSocketCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);


  os << indent << "SwapBytesInReceivedData: ";
  if (this->SwapBytesInReceivedData == SwapOff) 
    {
    os << "Off\n";
    }
  if (this->SwapBytesInReceivedData == SwapOn) 
    {
    os << "On\n";
    }
  if (this->SwapBytesInReceivedData == SwapNotSet) 
    {
    os << "NotSet\n";
    }

  os << indent << "IsConnected: " << this->IsConnected << endl;
  os << indent << "Perform a handshake: " 
     << ( this->PerformHandshake ? "Yes" : "No" ) << endl;

  os << indent << "ReportErrors: " << this->ReportErrors << endl;
}

//----------------------------------------------------------------------------
void vtkSocketCommunicator::SetLogStream(ostream* stream)
{
  if(this->LogStream != stream)
    {
    // If the log stream is our own log file, close the file.
    if(this->LogFile && this->LogFile == this->LogStream)
      {
      delete this->LogFile;
      this->LogFile = 0;
      }
    
    // Use the given log stream.
    this->LogStream = stream;
    }
}

//----------------------------------------------------------------------------
ostream* vtkSocketCommunicator::GetLogStream()
{
  return this->LogStream;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::LogToFile(const char* name)
{
  return this->LogToFile(name, 0);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::LogToFile(const char* name, int append)
{
  // Close old logging file.
  if(this->LogFile)
    {
    delete this->LogFile;
    this->LogFile = 0;
    }
  this->LogStream = 0;
  
  // Log to given file, if any.
  if(name && name[0])
    {
    this->LogFile = new ofstream(name, (ios::out |
                                        (append? ios::ate : ios::trunc)));
    if(!this->LogFile)
      {
      return 0;
      }
    if(!*this->LogFile)
      {
      delete this->LogFile;
      this->LogFile = 0;
      return 0;
      }
    this->LogStream = this->LogFile;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(int* data, int length, int remoteProcessId, 
                                int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->SendTagged(data, static_cast<int>(sizeof(int)),
                          length, tag, "int");
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(unsigned long* data, int length, 
                                int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->SendTagged(data, static_cast<int>(sizeof(unsigned long)),
                          length, tag, "ulong");
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(char* data, int length, 
                                int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->SendTagged(data, static_cast<int>(sizeof(char)),
                          length, tag, "char");
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(unsigned char* data, int length, 
                                int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->SendTagged(data, static_cast<int>(sizeof(unsigned char)),
                          length, tag, "uchar");
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(float* data, int length, 
                                int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->SendTagged(data, static_cast<int>(sizeof(float)),
                          length, tag, "float");
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(double *data, int length, 
                                int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->SendTagged(data, static_cast<int>(sizeof(double)),
                          length, tag, "double");
}

#ifdef VTK_USE_64BIT_IDS
//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(vtkIdType *data, int length, 
                                int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->SendTagged(data, static_cast<int>(sizeof(vtkIdType)),
                          length, tag, "vtkIdType");
}
#endif

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(int* data, int length, int remoteProcessId, 
                                   int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  int ret = this->ReceiveTagged(data, static_cast<int>(sizeof(int)),
                                length, tag, "int");
  if(tag == vtkMultiProcessController::RMI_TAG)
    {
    data[2] = 1;
    }  
  return ret;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(unsigned long* data, int length, 
                                   int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->ReceiveTagged(data, static_cast<int>(sizeof(unsigned long)),
                             length, tag, "ulong");
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(char* data, int length, 
                                   int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->ReceiveTagged(data, static_cast<int>(sizeof(char)),
                             length, tag, "char");
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(unsigned char* data, int length, 
                                   int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->ReceiveTagged(data, static_cast<int>(sizeof(unsigned char)),
                             length, tag, "uchar");
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(float* data, int length, 
                                   int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->ReceiveTagged(data, static_cast<int>(sizeof(float)),
                             length, tag, "float");
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(double* data, int length, 
                                   int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->ReceiveTagged(data, static_cast<int>(sizeof(double)),
                             length, tag, "double");
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_64BIT_IDS
int vtkSocketCommunicator::Receive(vtkIdType* data, int length, 
                                   int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }
  return this->ReceiveTagged(data, static_cast<int>(sizeof(vtkIdType)),
                             length, tag, "vtkIdType");
}
#endif

//----------------------------------------------------------------------------
int vtkSocketCommunicator::GetPort(int sock)
{
  struct sockaddr_in sockinfo;
  memset(&sockinfo, 0, sizeof(sockinfo));
#if defined(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
  socklen_t sizebuf = sizeof(sockinfo);
#else
  int sizebuf = sizeof(sockinfo);
#endif
  if(getsockname(sock, reinterpret_cast<sockaddr*>(&sockinfo), &sizebuf) != 0)
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("No port found for socket " << sock);
      }
    return 0;
    }
  return ntohs(sockinfo.sin_port);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::OpenSocket(int port, const char* )
{
  if ( this->IsConnected )
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Port " << 1 << " is occupied.");
      }
    return 0;
    }

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  // Elimate windows 0.2 second delay sending (buffering) data.
  int on = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)))
    {
    return -1;
    }

  struct sockaddr_in server;

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  // Allow the socket to be bound to an address that is already in use
  int opt=1;
#ifdef _WIN32
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &opt, sizeof(int));
#else
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &opt, sizeof(int));
#endif

  if ( bind(sock, reinterpret_cast<sockaddr*>(&server), sizeof(server)) )
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Can not bind socket to port " << port);
      }
    return 0;
    }
  listen(sock,1);
  return sock;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::SelectSocket(int socket, unsigned long msec)
{
  if ( socket < 0 )
    {
    return 0;
    }
  fd_set rset;
  struct timeval tval;
  struct timeval* tvalptr = 0;
  if ( msec > 0 )
    {
    tval.tv_sec = msec / 1000;
    tval.tv_usec = msec % 1000;
    tvalptr = &tval;
    }
  FD_ZERO(&rset);
  FD_SET(socket, &rset);
  int res = select(socket + 1, &rset, 0, 0, tvalptr);
  if(res == 0)
    {
    return -1;//for time limit expire
    }
  if ( res < 0 || !(FD_ISSET(socket, &rset)) )
    {
    return 0;
    }
  return 1;
}
//----------------------------------------------------------------------------
int vtkSocketCommunicator::WaitForConnectionOnSocket(int sock, unsigned long timeout)
{
  int res = this->SelectSocket(sock, timeout);
  if ( res <= 0 )
    {
    return res;
    }
  this->Socket = accept(sock, 0, 0);
  if ( this->Socket == -1 )
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Error in accept.");
      }
    return 0;
    }
  vtkCloseSocketMacro(sock);
  sock = -1;
  
  this->IsConnected = 1;
  
  if ( this->PerformHandshake )
    {
    // Handshake to determine if the client machine has the same endianness
    char clientIsBE;
    if(!this->ReceiveTagged(&clientIsBE, static_cast<int>(sizeof(char)),
                            1, vtkSocketController::ENDIAN_TAG, 0))
      {
      if (this->ReportErrors)
        {
        vtkErrorMacro("Endian handshake failed.");
        }
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
    if(!this->SendTagged(&IAmBE, static_cast<int>(sizeof(char)),
                         1, vtkSocketController::ENDIAN_TAG, 0))
      {
      if (this->ReportErrors)
        {
        vtkErrorMacro("Endian handshake failed.");
        }
      return 0;
      }
    
    if ( clientIsBE != IAmBE )
      {
      this->SwapBytesInReceivedData = vtkSocketCommunicator::SwapOn;
      }
    else
      {
      this->SwapBytesInReceivedData = vtkSocketCommunicator::SwapOff;
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::WaitForConnection(int port)
{
  int sock = this->OpenSocket(port);
  if(sock == 0)
    {
    return 0;
    }
  return this->WaitForConnectionOnSocket(sock);
}

void vtkSocketCommunicator::CloseConnection()
{
  if ( this->IsConnected )
    {
    vtkCloseSocketMacro(this->Socket);
    this->Socket = -1; 
    this->IsConnected = 0;
    }
}

int vtkSocketCommunicator::ConnectTo ( char* hostName, int port )
{

  if ( this->IsConnected )
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Communicator port " << 1 << " is occupied.");
      }
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
    if (this->ReportErrors)
      {
      vtkErrorMacro("Unknown host: " << hostName);
      }
    return 0;
    }

  this->Socket = socket(AF_INET, SOCK_STREAM, 0);
  // Elimate windows 0.2 second delay sending (buffering) data.
  int on = 1;
  if (setsockopt(this->Socket, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)))
    {
    return -1;
    }

  struct sockaddr_in name;
  name.sin_family = AF_INET;
  memcpy(&name.sin_addr, hp->h_addr, hp->h_length);
  name.sin_port = htons(port);

  if( connect(this->Socket, reinterpret_cast<sockaddr*>(&name), sizeof(name)) < 0)
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Can not connect to " << hostName << " on port " << port);
      }
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
  if(!this->SendTagged(&IAmBE, static_cast<int>(sizeof(char)),
                       1, vtkSocketController::ENDIAN_TAG, 0))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Endian handshake failed.");
      }
    return 0;
    }

  char serverIsBE;
  if (!this->ReceiveTagged(&serverIsBE, static_cast<int>(sizeof(char)), 1,
                           vtkSocketController::ENDIAN_TAG, 0))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Endian handshake failed.");
      }
    return 0;
    }
  vtkDebugMacro(<< "Server is " << ( serverIsBE ? "big" : "little" ) << "-endian");

  if ( serverIsBE != IAmBE )
    {
    this->SwapBytesInReceivedData = vtkSocketCommunicator::SwapOn;
    }
  else
    {
    this->SwapBytesInReceivedData = vtkSocketCommunicator::SwapOff;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::SendInternal(int socket, void* data, int length)
{
  char* buffer = reinterpret_cast<char*>(data);
  int total = 0;
  do
    {
    int n = send(socket, buffer+total, length-total, 0);
    if(n < 1)
      {
      return 0;
      }
    total += n;
    } while(total < length);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::ReceiveInternal(int socket, void* data, int length)
{
  char* buffer = reinterpret_cast<char*>(data);
  int total = 0;
  do
    {
#if defined(_WIN32) && !defined(__CYGWIN__)
    int trys = 0;
#endif
    int n = recv(socket, buffer+total, length-total, 0);
    if(n < 1)
      {
#if defined(_WIN32) && !defined(__CYGWIN__)
      // On long messages, Windows recv sometimes fails with WSAENOBUFS, but
      // will work if you try again.
      int error = WSAGetLastError();
      if ((error == WSAENOBUFS) && (trys++ < 1000))
        {
        Sleep(1);
        continue;
        }
#endif
      return 0;
      }
    total += n;
    } while(total < length);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::SendTagged(void* data, int wordSize,
                                      int numWords, int tag,
                                      const char* logName)
{
  if(!this->SendInternal(this->Socket, &tag, static_cast<int>(sizeof(int))))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Could not send tag.");
      }
    return 0;
    }
  int length = wordSize * numWords;
  if(!this->SendInternal(this->Socket, &length, 
      static_cast<int>(sizeof(int))))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Could not send length.");
      }
    return 0;
    }  
  if(!this->SendInternal(this->Socket, data, wordSize*numWords))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Could not send message.");
      }
    return 0;
    }
  
  // Log this event.
  this->LogTagged("Sent", data, wordSize, numWords, tag, logName);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::ReceiveTagged(void* data, int wordSize,
                                         int numWords, int tag,
                                         const char* logName)
{
  int success = 0;
  int length = -1;
  while ( !success )
    {
    int recvTag = -1;
    length = -1;
    if(!this->ReceiveInternal(this->Socket, &recvTag,
        static_cast<int>(sizeof(int))))
      {
      if (this->ReportErrors)
        {
        vtkErrorMacro("Could not receive tag. " << tag);
        }
      return 0;
      }
    if(this->SwapBytesInReceivedData == vtkSocketCommunicator::SwapOn)
      {
      vtkSwap4(reinterpret_cast<char*>(&recvTag));
      }
    if(!this->ReceiveInternal(this->Socket, &length,
        static_cast<int>(sizeof(int))))
      {
      if (this->ReportErrors)
        {
        vtkErrorMacro("Could not receive length.");
        }
      return 0;
      }
    if(this->SwapBytesInReceivedData == vtkSocketCommunicator::SwapOn)
      {
      vtkSwap4(reinterpret_cast<char*>(&length));
      }
    if(recvTag != tag)
      {
      char* idata = new char[length + sizeof(recvTag) + sizeof(length)];
      char* ptr = idata;
      memcpy(ptr, (void*)&recvTag, sizeof(recvTag));
      ptr += sizeof(recvTag);
      memcpy(ptr, (void*)&length, sizeof(length));
      ptr += sizeof(length);
      this->ReceivePartialTagged(ptr, 1, length, tag, "Wrong tag");
      int res = this->InvokeEvent(vtkCommand::WrongTagEvent, idata);
      delete [] idata;
      if ( res )
        {
        continue;
        }

      if (this->ReportErrors)
        {
        vtkErrorMacro("Tag mismatch: got " << recvTag << ", expecting " << tag
                      << ".");
        }
      return 0;
      }
    else
      {
      success = 1;
      }
    }
  // Length may not be correct for the first message sent as an 
  // endian handshake because the SwapBytesInReceivedData flag 
  // is not initialized at this point.  We could just initialize it
  // here, but what is the point.
  if ((wordSize * numWords) != length && 
      this->SwapBytesInReceivedData != vtkSocketCommunicator::SwapNotSet)
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Requested size (" << (wordSize * numWords) 
                    << ") is different than the size that was sent (" << length << ")");
      }
    return 0;
    }
  return this->ReceivePartialTagged(data, wordSize, numWords, tag, logName);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::ReceivePartialTagged(void* data, int wordSize,
                                         int numWords, int tag,
                                         const char* logName)
{
  if(!this->ReceiveInternal(this->Socket, data, wordSize*numWords))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Could not receive message.");
      }
    return 0;
    }
  // Unless we're dealing with chars, then check byte ordering.
  // This is really bad and should probably use some enum for types
  if(this->SwapBytesInReceivedData == vtkSocketCommunicator::SwapOn)
    {
    if(wordSize == 4)
      {
      vtkDebugMacro(<< " swapping 4 range, size = " << wordSize 
                    << " length = " << numWords);
      vtkSwap4Range(reinterpret_cast<char*>(data), numWords);
      }
    else if(wordSize == 8)
      {
      vtkDebugMacro(<< " swapping 8 range, size = " << wordSize
                    << " length = " << numWords );
      vtkSwap8Range(reinterpret_cast<char*>(data), numWords);
      }
    }
  
  // Log this event.
  this->LogTagged("Received", data, wordSize, numWords, tag, logName);
  
  return 1;
}

//----------------------------------------------------------------------------
template <class T, class OutType>
void vtkSocketCommunicatorLogArray(ostream& os, T* array, int length, int max,
                                   OutType*)
{
  if(length > 0)
    {
    int num = (length <= max)? length:max;
    os << " data={" << static_cast<OutType>(array[0]);
    for(int i=1; i < num; ++i)
      {
      os << " " << static_cast<OutType>(array[i]);
      }
    if(length > max)
      {
      os << " ...";
      }
    os << "}";
    }
}

//----------------------------------------------------------------------------
void vtkSocketCommunicator::LogTagged(const char* name, void* data,
                                      int wordSize, int numWords,
                                      int tag, const char* logName)
{
  if(this->LogStream)
    {
    // Log the general event information.
    *this->LogStream << name;
    if(logName)
      {
      *this->LogStream << " " << logName;
      }
    *this->LogStream << " data: tag=" << tag
                     << " wordSize=" << wordSize
                     << " numWords=" << numWords;
    
    // If this is a string, log the first 70 characters.  If this is
    // an array of data values, log the first few.
    if(wordSize == static_cast<int>(sizeof(char)) && logName &&
       (strcmp(logName, "char") == 0))
      {
      char* chars = reinterpret_cast<char*>(data);
      if((chars[numWords-1]) == 0 &&
         (static_cast<int>(strlen(chars)) == numWords-1))
        {
        // String data.  Display the first 70 characters.
        *this->LogStream << " data={";
        if(numWords <= 71)
          {
          *this->LogStream << chars;
          }
        else
          {
          this->LogStream->write(reinterpret_cast<char*>(data), 70);
          *this->LogStream << " ...";
          }
        *this->LogStream << "}";
        }
      else
        {
        // Not string data.  Display the characters as integer values.
        vtkSocketCommunicatorLogArray(*this->LogStream,
                                      reinterpret_cast<char*>(data),
                                      numWords, 6, static_cast<int*>(0));
        }
      }
    else if(wordSize == static_cast<int>(sizeof(int)) && logName &&
            (strcmp(logName, "int") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                    reinterpret_cast<int*>(data),
                                    numWords, 6, static_cast<int*>(0));
      }
    else if(wordSize == static_cast<int>(sizeof(unsigned char)) && logName &&
            (strcmp(logName, "uchar") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                    reinterpret_cast<unsigned char*>(data),
                                    numWords, 6, static_cast<int*>(0));
      }
    else if(wordSize == static_cast<int>(sizeof(unsigned long)) && logName &&
            (strcmp(logName, "ulong") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                    reinterpret_cast<unsigned long*>(data),
                                    numWords, 6, static_cast<unsigned long*>(0));
      }
    else if(wordSize == static_cast<int>(sizeof(float)) && logName &&
            (strcmp(logName, "float") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                    reinterpret_cast<float*>(data),
                                    numWords, 6, static_cast<float*>(0));
      }
    else if(wordSize == static_cast<int>(sizeof(double)) && logName &&
            (strcmp(logName, "double") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                    reinterpret_cast<double*>(data),
                                    numWords, 6, static_cast<double*>(0));
      }
#ifdef VTK_USE_64BIT_IDS
    else if(wordSize == static_cast<int>(sizeof(vtkIdType)) && logName &&
            (strcmp(logName, "vtkIdType") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                    reinterpret_cast<vtkIdType*>(data),
                                    numWords, 6, static_cast<vtkIdType*>(0));
      }
#endif
    *this->LogStream << "\n";
    }
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::CheckForErrorInternal(int id)
{
  if(id == 0)
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Can not connect to myself!");
      }
    return 1;
    }
  else if(id >= this->NumberOfProcesses)
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("No port for process " << id << " exists.");
      }
    return 1;
    }
  return 0;
}
