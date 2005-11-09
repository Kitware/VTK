/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientSocket.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClientSocket - Encapsulates a client socket.

#ifndef __vtkClientSocket_h
#define __vtkClientSocket_h

#include "vtkSocket.h"
class vtkServerSocket;

class VTK_PARALLEL_EXPORT vtkClientSocket : public vtkSocket
{
public:
  static vtkClientSocket* New();
  vtkTypeRevisionMacro(vtkClientSocket, vtkSocket);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Connects to host. Returns 0 on success, -1 on error.
  int ConnectToServer(const char* hostname, int port); 
  
protected:
  vtkClientSocket();
  ~vtkClientSocket();

//BTX
  friend class vtkServerSocket;
//ETX
private:
  vtkClientSocket(const vtkClientSocket&); // Not implemented.
  void operator=(const vtkClientSocket&); // Not implemented.
};


#endif

