/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSocket.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSocket - BSD socket encapsulation.
// .SECTION Description
// This abstract class encapsulates a BSD socket. It provides an API for  
// basic socket operations.

#ifndef __vtkSocket_h
#define __vtkSocket_h

#include "vtkObject.h"

class vtkSocketCollection;
class VTK_COMMON_EXPORT vtkSocket : public vtkObject
{
public:
  vtkTypeMacro(vtkSocket, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // ----- Status API ----
  // Description:
  // Check is the socket is alive.
  int GetConnected() { return (this->SocketDescriptor >=0); }

  // Description:
  // Close the socket.
  void CloseSocket() {this->CloseSocket(this->SocketDescriptor);}
 
  // ------ Communication API ---
  // Description:
  // These methods send data over the socket.
  // Returns 1 on success, 0 on error and raises vtkCommand::ErrorEvent.
  int Send(const void* data, int length);

  // Description:
  // Receive data from the socket.
  // This call blocks until some data is read from the socket.
  // When readFully is set, this call will block until all the
  // requested data is read from the socket.
  // 0 on error, else number of bytes read is returned. On error,
  // vtkCommand::ErrorEvent is raised.
  int Receive(void* data, int length, int readFully=1);

protected:
  vtkSocket();
  ~vtkSocket();

  int SocketDescriptor;
  vtkGetMacro(SocketDescriptor, int);

  //BTX
  friend class vtkSocketCollection;
  //ETX
 
  // Description:
  // Creates an endpoint for communication and returns the descriptor.
  // -1 indicates error.
  int CreateSocket();

  // Description:
  // Close the socket.
  void CloseSocket(int socketdescriptor);

  // Description:
  // Binds socket to a particular port.
  // Returns 0 on success other -1 is returned.
  int BindSocket(int socketdescriptor, int port);

  // Description:
  // Selects a socket ie. waits for it to change status.
  // Returns 1 on success; 0 on timeout; -1 on error. msec=0 implies
  // no timeout.
  int SelectSocket(int socketdescriptor, unsigned long msec);

  // Description:
  // Accept a connection on a socket. Returns -1 on error. Otherwise
  // the descriptor of the accepted socket.
  int Accept(int socketdescriptor);

  // Description:
  // Listen for connections on a socket. Returns 0 on success. -1 on error.
  int Listen(int socketdescriptor);

  // Description:
  // Connect to a server socket. Returns 0 on success, -1 on error.
  int Connect(int socketdescriptor, const char* hostname, int port);

  // Description:
  // Returns the port to which the socket is connected.
  // 0 on error.
  int GetPort(int socketdescriptor);

  // Description:
  // Selects set of sockets. Returns 0 on timeout, -1 on error.
  // 1 on success. Selected socket's index is returned thru 
  // selected_index
  static int SelectSockets(const int* sockets_to_select, int size,
    unsigned long msec, int* selected_index);
private:
  vtkSocket(const vtkSocket&); // Not implemented.
  void operator=(const vtkSocket&); // Not implemented.
};


#endif

