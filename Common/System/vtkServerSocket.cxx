/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkServerSocket.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkServerSocket.h"

#include "vtkClientSocket.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkServerSocket);
//-----------------------------------------------------------------------------
vtkServerSocket::vtkServerSocket()
{
}

//-----------------------------------------------------------------------------
vtkServerSocket::~vtkServerSocket()
{
}

//-----------------------------------------------------------------------------
int vtkServerSocket::GetServerPort()
{
  if (!this->GetConnected())
  {
    return 0;
  }
  return this->GetPort(this->SocketDescriptor);
}

//-----------------------------------------------------------------------------
int vtkServerSocket::CreateServer(int port)
{
  if (this->SocketDescriptor != -1)
  {
    vtkWarningMacro("Server Socket already exists. Closing old socket.");
    this->CloseSocket(this->SocketDescriptor);
    this->SocketDescriptor = -1;
  }
  this->SocketDescriptor = this->CreateSocket();
  if (this->SocketDescriptor < 0)
  {
    return -1;
  }
  if ( this->BindSocket(this->SocketDescriptor, port) != 0||
    this->Listen(this->SocketDescriptor) != 0)
  {
    // failed to bind or listen.
    this->CloseSocket(this->SocketDescriptor);
    this->SocketDescriptor = -1;
    return -1;
  }
  // Success.
  return 0;
}

//-----------------------------------------------------------------------------
vtkClientSocket* vtkServerSocket::WaitForConnection(unsigned long msec /*=0*/)
{
  if (this->SocketDescriptor < 0)
  {
    vtkErrorMacro("Server Socket not created yet!");
    return NULL;
  }

  int ret = this->SelectSocket(this->SocketDescriptor, msec);
  if (ret == 0)
  {
    // Timed out.
    return NULL;
  }
  if (ret == -1)
  {
    vtkErrorMacro("Error selecting socket.");
    return NULL;
  }
  int clientsock = this->Accept(this->SocketDescriptor);
  if (clientsock == -1)
  {
    vtkErrorMacro("Failed to accept the socket.");
    return NULL;
  }
  // Create a new vtkClientSocket and return it.
  vtkClientSocket* cs = vtkClientSocket::New();
  cs->SocketDescriptor = clientsock;
  cs->SetConnectingSide(false);
  return cs;
}

//-----------------------------------------------------------------------------
void vtkServerSocket::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
