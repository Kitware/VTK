/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSocket.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSocket.h"

#include "vtkObjectFactory.h"

#if defined(__BORLANDC__)
# pragma warn -8012 /* signed/unsigned comparison */
#endif

// The VTK_SOCKET_FAKE_API definition is given to the compiler
// command line by CMakeLists.txt if there is no real sockets
// interface available.  When this macro is defined we simply make
// every method return failure.
//
// Perhaps we should add a method to query at runtime whether a real
// sockets interface is available.

#ifndef VTK_SOCKET_FAKE_API
#if defined(_WIN32) && !defined(__CYGWIN__)
  #define VTK_WINDOWS_FULL
  #include "vtkWindows.h"
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <sys/time.h>
  #include <errno.h>
  #include <string.h>
  #include <stdio.h>
#endif
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)

// TODO : document why we restrict to v1.1
#define WSA_VERSION MAKEWORD(1,1)

#define vtkCloseSocketMacro(sock)      (closesocket(sock))
#define vtkErrnoMacro                  (WSAGetLastError())
#define vtkStrerrorMacro(_num)         (wsaStrerror(_num))
#define vtkSocketErrorIdMacro(_id)     (WSA##_id)
#define vtkSocketErrorReturnMacro      (SOCKET_ERROR)

#else

#define vtkCloseSocketMacro(sock)      (close(sock))
#define vtkErrnoMacro                  (errno)
#define vtkStrerrorMacro(_num)         (strerror(_num))
#define vtkSocketErrorIdMacro(_id)     (_id)
#define vtkSocketErrorReturnMacro      (-1)

#endif

// This macro wraps a system function call(_call),
// restarting the call in case it was interrupted
// by a signal (EINTR).
#define vtkRestartInterruptedSystemCallMacro(_call,_ret)\
  do                                                    \
    {                                                   \
    (_ret)=(_call);                                     \
    }                                                   \
  while (((_ret)==vtkSocketErrorReturnMacro)            \
    && (vtkErrnoMacro==vtkSocketErrorIdMacro(EINTR)));

// use when _str may be a null pointer but _fallback is not.
#define vtkSafeStrMacro(_str,_fallback) ((_str)?(_str):(_fallback))

// convert error number to string and report via vtkErrorMacro.
#define vtkSocketErrorMacro(_eno, _message)             \
  vtkErrorMacro(                                        \
    << (_message)                                       \
    << " "                                              \
    << vtkSafeStrMacro(                                 \
         vtkStrerrorMacro(_eno),                        \
         "unknown error")                               \
    << ".");

// convert error number to string and report via vtkGenericWarningMacro
#define vtkSocketGenericErrorMacro(_message)            \
  vtkGenericWarningMacro(                                 \
    << (_message)                                       \
    << " "                                              \
    << vtkSafeStrMacro(                                 \
         vtkStrerrorMacro(vtkErrnoMacro),               \
         "unknown error")                               \
    << ".");

// on windows strerror doesn't handle socket error codes
#if defined(_WIN32) && !defined(__CYGWIN__)
static
const char *wsaStrerror(int wsaeid)
{
  static char buf[256]={'\0'};
  int ok;
  ok=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,wsaeid,0,buf,256,0);
  if (!ok)
    {
    return 0;
    }
  return buf;
}
#endif

//-----------------------------------------------------------------------------
vtkSocket::vtkSocket()
{
  this->SocketDescriptor = -1;
}

//-----------------------------------------------------------------------------
vtkSocket::~vtkSocket()
{
  if (this->SocketDescriptor != -1)
    {
    this->CloseSocket(this->SocketDescriptor);
    this->SocketDescriptor = -1;
    }
}

//-----------------------------------------------------------------------------
int vtkSocket::CreateSocket()
{
#ifndef VTK_SOCKET_FAKE_API
  int sock;
  vtkRestartInterruptedSystemCallMacro(socket(AF_INET,SOCK_STREAM, 0), sock);
  if (sock == vtkSocketErrorReturnMacro)
    {
    vtkSocketErrorMacro(vtkErrnoMacro, "Socket error in call to socket.");
    return -1;
    }

  // Elimate windows 0.2 second delay sending (buffering) data.
  int on = 1;
  int iErr;
  vtkRestartInterruptedSystemCallMacro(
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)),
    iErr);
  if (iErr == vtkSocketErrorReturnMacro)
    {
    vtkSocketErrorMacro(vtkErrnoMacro, "Socket error in call to setsockopt.");
    return -1;
    }

  return sock;
#else
  return -1;
#endif
}

//-----------------------------------------------------------------------------
void vtkSocket::CloseSocket()
{
  this->CloseSocket(this->SocketDescriptor);
  this->SocketDescriptor = -1;
}

//-----------------------------------------------------------------------------
int vtkSocket::BindSocket(int socketdescriptor, int port)
{
#ifndef VTK_SOCKET_FAKE_API
  struct sockaddr_in server;

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  // Allow the socket to be bound to an address that is already in use
  int opt=1;
  int iErr=~vtkSocketErrorReturnMacro;
#ifdef _WIN32
  vtkRestartInterruptedSystemCallMacro(
    setsockopt(socketdescriptor,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(int)),
    iErr);
#elif defined(VTK_HAVE_SO_REUSEADDR)
  vtkRestartInterruptedSystemCallMacro(
    setsockopt(socketdescriptor,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(int)),
    iErr);
#endif
  if (iErr == vtkSocketErrorReturnMacro)
    {
    vtkSocketErrorMacro(vtkErrnoMacro, "Socket error in call to setsockopt.");
    return -1;
    }

  vtkRestartInterruptedSystemCallMacro(
    bind(socketdescriptor,reinterpret_cast<sockaddr*>(&server),sizeof(server)),
    iErr);
  if (iErr == vtkSocketErrorReturnMacro)
    {
    vtkSocketErrorMacro(vtkErrnoMacro, "Socket error in call to bind.");
    return -1;
    }

  return 0;
#else
  static_cast<void>(socketdescriptor);
  static_cast<void>(port);
  return -1;
#endif
}

//-----------------------------------------------------------------------------
int vtkSocket::Accept(int socketdescriptor)
{
#ifndef VTK_SOCKET_FAKE_API
  if (socketdescriptor < 0)
    {
    vtkErrorMacro("Invalid descriptor.");
    return -1;
    }

  int newDescriptor;
  vtkRestartInterruptedSystemCallMacro(
    accept(socketdescriptor, 0, 0), newDescriptor);
  if (newDescriptor == vtkSocketErrorReturnMacro)
    {
    vtkSocketErrorMacro(vtkErrnoMacro, "Socket error in call to accept.");
    return -1;
    }

  return newDescriptor;
#else
  static_cast<void>(socketdescriptor);
  return -1;
#endif
}

//-----------------------------------------------------------------------------
int vtkSocket::Listen(int socketdescriptor)
{
#ifndef VTK_SOCKET_FAKE_API
  if (socketdescriptor < 0)
    {
    vtkErrorMacro("Invalid descriptor.");
    return -1;
    }

  int iErr;
  vtkRestartInterruptedSystemCallMacro(listen(socketdescriptor, 1), iErr);
  if (iErr == vtkSocketErrorReturnMacro)
    {
    vtkSocketErrorMacro(vtkErrnoMacro, "Socket error in call to listen.");
    return -1;
    }

  return 0;
#else
  static_cast<void>(socketdescriptor);
  return -1;
#endif
}

//-----------------------------------------------------------------------------
int vtkSocket::SelectSocket(int socketdescriptor, unsigned long msec)
{
#ifndef VTK_SOCKET_FAKE_API
  if (socketdescriptor < 0 )
    {
    vtkErrorMacro("Invalid descriptor.");
    return -1;
    }

  fd_set rset;
  int res;
  do
    {
    struct timeval tval;
    struct timeval* tvalptr = 0;
    if (msec>0)
      {
      tval.tv_sec = msec / 1000;
      tval.tv_usec = (msec % 1000)*1000;
      tvalptr = &tval;
      }

    FD_ZERO(&rset);
    FD_SET(socketdescriptor, &rset);

    // block until socket is readable.
    res = select(socketdescriptor+1, &rset, 0, 0, tvalptr);
    }
  while ((res == vtkSocketErrorReturnMacro)
    && (vtkErrnoMacro == vtkSocketErrorIdMacro(EINTR)));

  if (res == 0)
    {
    // time out
    return 0;
    }
  else
  if (res == vtkSocketErrorReturnMacro)
    {
    // error in the call
    vtkSocketErrorMacro(vtkErrnoMacro, "Socket error in call to select.");
    return -1;
    }
  else
  if (!FD_ISSET(socketdescriptor, &rset))
     {
     vtkErrorMacro("Socket error in select. Descriptor not selected.");
     return -1;
     }

  // NOTE: not checking for pending errors,these will be handled
  // in the next call to read/recv

  // The indicated socket has some activity on it.
  return 1;
#else
  static_cast<void>(socketdescriptor);
  static_cast<void>(msec);
  return -1;
#endif
}

//-----------------------------------------------------------------------------
int vtkSocket::SelectSockets(const int* sockets_to_select, int size,
    unsigned long msec, int* selected_index)
{
#ifndef VTK_SOCKET_FAKE_API

  *selected_index = -1;

  if (size < 0)
    {
    vtkGenericWarningMacro("Can't select fewer than 0.");
    return -1;
    }

  fd_set rset;
  int res = -1;
  do
    {
    struct timeval tval;
    struct timeval* tvalptr = 0;
    if (msec>0)
      {
      tval.tv_sec = msec / 1000;
      tval.tv_usec = msec % 1000;
      tvalptr = &tval;
      }

    FD_ZERO(&rset);
    int max_fd = -1;
    for (int i=0; i<size; i++)
      {
      FD_SET(sockets_to_select[i],&rset);
      max_fd = (sockets_to_select[i] > max_fd ? sockets_to_select[i] : max_fd);
      }

    // block until one socket is ready to read.
    res = select(max_fd + 1, &rset, 0, 0, tvalptr);
    }
  while ((res == vtkSocketErrorReturnMacro)
    && (vtkErrnoMacro == vtkSocketErrorIdMacro(EINTR)));

  if (res==0)
    {
    // time out
    return 0;
    }
  else
  if (res == vtkSocketErrorReturnMacro)
    {
    // error in the call
    vtkSocketGenericErrorMacro("Socket error in call to select.");
    return -1;
    }

  // find the first socket which has some activity.
  for (int i=0; i<size; i++)
    {
    if ( FD_ISSET(sockets_to_select[i],&rset) )
      {
      // NOTE: not checking for pending errors, these
      // will be handled in the next call to read/recv

      *selected_index = i;
      return 1;
      }
    }

  // no activity on any of the sockets
  vtkGenericWarningMacro("Socket error in select. No descriptor selected.");
  return -1; 
#else
  static_cast<void>(sockets_to_select);
  static_cast<void>(size);
  static_cast<void>(msec);
  static_cast<void>(selected_index);
  return -1;
#endif
}

//-----------------------------------------------------------------------------
int vtkSocket::Connect(int socketdescriptor, const char* hostName, int port)
{
#ifndef VTK_SOCKET_FAKE_API
  if (socketdescriptor < 0)
    {
    vtkErrorMacro("Invalid descriptor.");
    return -1;
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
    return -1;
    }

  struct sockaddr_in name;
  name.sin_family = AF_INET;
  memcpy(&name.sin_addr, hp->h_addr, hp->h_length);
  name.sin_port = htons(port);

  int iErr
    = connect(socketdescriptor, reinterpret_cast<sockaddr*>(&name),sizeof(name));
  if ( (iErr == vtkSocketErrorReturnMacro )
    && (vtkErrnoMacro == vtkSocketErrorIdMacro(EINTR)) )
    {
    // Restarting an interrupted connect call only works on linux,
    // other unix require a call to select which blocks until the
    // connection is complete.
    // See Stevens 2d ed, 15.4 p413, "interrupted connect"
    iErr = this->SelectSocket(socketdescriptor,0);
    if (iErr == -1)
      {
      // SelectSocket doesn't test for pending errors.
      int pendingErr;
#if defined(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
      socklen_t pendingErrLen = sizeof(pendingErr);
#else
      int pendingErrLen = sizeof(pendingErr);
#endif

      vtkRestartInterruptedSystemCallMacro(
        getsockopt(socketdescriptor, SOL_SOCKET, SO_ERROR, (char *)&pendingErr, &pendingErrLen),
        iErr);
      if (iErr == vtkSocketErrorReturnMacro)
        {
        vtkSocketErrorMacro(
          vtkErrnoMacro, "Socket error in call to getsockopt.");
        return -1;
        }
      else
      if (pendingErr)
        {
        vtkSocketErrorMacro(
          pendingErr, "Socket error pending from call to connect.");
        return -1;
        }
      }
    }
  else
  if (iErr == vtkSocketErrorReturnMacro)
    {
    vtkSocketErrorMacro(
      vtkErrnoMacro, "Socket error in call to connect.");
    return -1;
    }

  return 0;
#else
  static_cast<void>(socketdescriptor);
  static_cast<void>(hostName);
  static_cast<void>(port);
  return -1;
#endif
}

//-----------------------------------------------------------------------------
int vtkSocket::GetPort(int sock)
{
#ifndef VTK_SOCKET_FAKE_API
  struct sockaddr_in sockinfo;
  memset(&sockinfo, 0, sizeof(sockinfo));
#if defined(VTK_HAVE_GETSOCKNAME_WITH_SOCKLEN_T)
  socklen_t sizebuf = sizeof(sockinfo);
#else
  int sizebuf = sizeof(sockinfo);
#endif

  int iErr;
  vtkRestartInterruptedSystemCallMacro(
    getsockname(sock, reinterpret_cast<sockaddr*>(&sockinfo), &sizebuf),
    iErr);
  if (iErr == vtkSocketErrorReturnMacro)
    {
    vtkSocketErrorMacro(
      vtkErrnoMacro, "Socket error in call to getsockname.");
    return 0;
    }
  return ntohs(sockinfo.sin_port);
#else
  static_cast<void>(sock);
  return -1;
#endif
}

//-----------------------------------------------------------------------------
void vtkSocket::CloseSocket(int socketdescriptor)
{
#ifndef VTK_SOCKET_FAKE_API
  if (socketdescriptor < 0)
    {
    vtkErrorMacro("Invalid descriptor.");
    return;
    }
  int iErr;
  vtkRestartInterruptedSystemCallMacro(
    vtkCloseSocketMacro(socketdescriptor),
    iErr);
  if (iErr == vtkSocketErrorReturnMacro)
    {
    vtkSocketErrorMacro(
      vtkErrnoMacro, "Socket error in call to close/closesocket.");
    }
#else
  static_cast<void>(socketdescriptor);
  return;
#endif
}

//-----------------------------------------------------------------------------
int vtkSocket::Send(const void* data, int length)
{
#ifndef VTK_SOCKET_FAKE_API
  if (!this->GetConnected())
    {
    vtkErrorMacro("Not connected.");
    return 0;
    }
  if (length == 0)
    {
    // nothing to send.
    return 1;
    }
  const char* buffer = reinterpret_cast<const char*>(data);
  int total = 0;
  do
    {
    int flags=0;
    int nSent;
    vtkRestartInterruptedSystemCallMacro(
      send(this->SocketDescriptor, buffer+total, length-total, flags),
      nSent);
    if (nSent == vtkSocketErrorReturnMacro)
      {
      vtkSocketErrorMacro(vtkErrnoMacro, "Socket error in call to send.");
      return 0;
      }
    total += nSent;
    } while(total < length);

  return 1;
#else
  static_cast<void>(data);
  static_cast<void>(length);
  return 0;
#endif
}

//-----------------------------------------------------------------------------
int vtkSocket::Receive(void* data, int length, int readFully/*=1*/)
{
#ifndef VTK_SOCKET_FAKE_API
  if (!this->GetConnected())
    {
    vtkErrorMacro("Not connected.");
    return 0;
    }

#if defined(_WIN32) && !defined(__CYGWIN__)
  int trys = 0;
#endif

  char* buffer = reinterpret_cast<char*>(data);
  int total = 0;
  do
    {
    int nRecvd;
    vtkRestartInterruptedSystemCallMacro(
      recv(this->SocketDescriptor, buffer+total, length-total, 0),
      nRecvd);

    if (nRecvd == 0)
      {
      // peer shut down
      return 0;
      }

#if defined(_WIN32) && !defined(__CYGWIN__)
    if ((nRecvd == vtkSocketErrorReturnMacro)
      && (WSAGetLastError() == WSAENOBUFS))
      {
      // On long messages, Windows recv sometimes fails with WSAENOBUFS, but
      // will work if you try again.
      if ((trys++ < 1000))
        {
        Sleep(1);
        continue;
        }
      vtkSocketErrorMacro(vtkErrnoMacro, "Socket error in call to recv.");
      return 0;
      }
#endif

    total += nRecvd;
    }
  while( readFully && (total < length));

  return total;
#else
  static_cast<void>(data);
  static_cast<void>(length);
  static_cast<void>(readFully);
  return 0;
#endif
}

//-----------------------------------------------------------------------------
void vtkSocket::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SocketDescriptor: " << this->SocketDescriptor << endl;
}
