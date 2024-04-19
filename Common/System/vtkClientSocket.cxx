// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkClientSocket.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkClientSocket);
//------------------------------------------------------------------------------
vtkClientSocket::vtkClientSocket()
{
  this->ConnectingSide = false;
}

//------------------------------------------------------------------------------
vtkClientSocket::~vtkClientSocket() = default;

//------------------------------------------------------------------------------
int vtkClientSocket::ConnectToServer(const char* hostName, int port)
{
  if (this->SocketDescriptor != -1)
  {
    vtkWarningMacro("Client connection already exists. Closing it.");
    this->CloseSocket(this->SocketDescriptor);
    this->SocketDescriptor = -1;
  }

  this->SocketDescriptor = this->CreateSocket();
  if (this->SocketDescriptor == -1)
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

//------------------------------------------------------------------------------
void vtkClientSocket::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConnectingSide: " << this->ConnectingSide << endl;
}
VTK_ABI_NAMESPACE_END
