/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientSocket.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientSocket.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkClientSocket);
//-----------------------------------------------------------------------------
vtkClientSocket::vtkClientSocket()
{
  this->ConnectingSide = false;
}

//-----------------------------------------------------------------------------
vtkClientSocket::~vtkClientSocket()
{
}

//-----------------------------------------------------------------------------
int vtkClientSocket::ConnectToServer(const char* hostName, int port)
{
  if (this->SocketDescriptor != -1)
    {
    vtkWarningMacro("Client connection already exists. Closing it.");
    this->CloseSocket(this->SocketDescriptor);
    this->SocketDescriptor = -1;
    }
  
  this->SocketDescriptor = this->CreateSocket();
  if (!this->SocketDescriptor)
    {
    vtkErrorMacro("Failed to create socket.");
    return -1;
    }

  if (this->Connect(this->SocketDescriptor, hostName, port) == -1)
    {
    this->CloseSocket(this->SocketDescriptor);
    this->SocketDescriptor = -1;

    vtkErrorMacro("Failed to connect to server " << hostName << ":" << port);
    return -1;
    }

  this->ConnectingSide = true;
  return 0;
}

//-----------------------------------------------------------------------------
void vtkClientSocket::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConnectingSide: " << this->ConnectingSide << endl;
}
