/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkSocketCommunicator.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkSocketCommunicator.h"
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
  this->Initialized = 0;
  this->Sockets = 0;
  this->NumberOfProcesses = 1;
}

//----------------------------------------------------------------------------
vtkSocketCommunicator::~vtkSocketCommunicator()
{
  for(int i=0; i<this->NumberOfProcesses; i++)
    {
    if (this->IsConnected[i])
      {
      vtkCloseSocketMacro(this->Sockets[i]);
      }
    }
  delete[] this->Sockets;
  delete[] this->IsConnected;
  this->Sockets = 0;
  this->IsConnected = 0;
}

void vtkSocketCommunicator::Initialize(int vtkNotUsed(argc), char *argv[])
{
  argv = argv;
#ifdef _WIN32
  WSAData wsaData;  
  if (WSAStartup(WSA_VERSION, &wsaData))
    {
    vtkErrorMacro("Could not initialize sockets !");
    }
#endif
}

void vtkSocketCommunicator::SetNumberOfProcesses(int num)
{
  if (this->Sockets)
    {
    vtkErrorMacro("Can not change the number of processes.");
    return;
    }
  this->Sockets = new int[num];
  this->IsConnected = new int[num];
  for (int i=0; i<num; i++)
    {
    this->IsConnected[i] = 0;
    }
  this->NumberOfProcesses = num;
  return;
}

//----------------------------------------------------------------------------
void vtkSocketCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMultiProcessController::PrintSelf(os,indent);
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
static int sendMessage(T* data, int length, int tag, int sock)
{
  // Need to check the return value of these
  send(sock, (char *)&tag, sizeof(int), 0);
  send(sock, (char *)data, length*sizeof(T), 0);
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

  return sendMessage(data, length, tag, this->Sockets[remoteProcessId]);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(unsigned long *data, int length, 
				int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  return sendMessage(data, length, tag, this->Sockets[remoteProcessId]);
}
//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(char *data, int length, 
				int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  return sendMessage(data, length, tag, this->Sockets[remoteProcessId]);
}
//----------------------------------------------------------------------------
int vtkSocketCommunicator::Send(float *data, int length, 
				int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  return sendMessage(data, length, tag, this->Sockets[remoteProcessId]);
}

template <class T>
static int receiveMessage(T* data, int length, int tag, int sock)
{
  int recvTag=-1;

  // Need to check the return value of these
  recv(sock, (char *)&recvTag, sizeof(int), MSG_PEEK);
  if (recvTag != tag)
    return 0;

  recv(sock, (char *)&recvTag, sizeof(int), 0);
  recv(sock, (char *)data, length*sizeof(T), 0);

  return 1;

}

template <class T>
static int receiveFromAnySource(T* data, int length, 
				int tag, const int* sockets, 
				const int* isConnected,
				int nOfProcesses)
{
  int success;
  
  for ( int i=1; i < nOfProcesses; i++)
    {
    if (isConnected[i])
      {
      success = receiveMessage(data, length, tag, sockets[i]);
      if (success)
	{
	cout << "success" << endl;
	return i;
	}
      }
    }
  return 0;

}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(int *data, int length, int remoteProcessId, 
				   int tag)
{
  int status;

  cout << remoteProcessId << endl;
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    int id =  receiveFromAnySource(data, length, tag, this->Sockets,
				   this->IsConnected, this->NumberOfProcesses);
    if ( tag == VTK_MP_CONTROLLER_RMI_TAG )
      {
      data[2] = id;
      }
    return id;
    }

  return receiveMessage(data, length, tag, this->Sockets[remoteProcessId]);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(unsigned long *data, int length, 
				   int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    return receiveFromAnySource(data, length, tag, this->Sockets,
				this->IsConnected, this->NumberOfProcesses);
    }
  return receiveMessage(data, length, tag, this->Sockets[remoteProcessId]);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::Receive(char *data, int length, 
				   int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    return receiveFromAnySource(data, length, tag, this->Sockets,
				this->IsConnected, this->NumberOfProcesses);
    }
  return receiveMessage(data, length, tag, this->Sockets[remoteProcessId]);
}

int vtkSocketCommunicator::Receive(float *data, int length, 
				   int remoteProcessId, int tag)
{
  if ( checkForError(remoteProcessId, this->NumberOfProcesses) )
    {
    return 0;
    }

  if (remoteProcessId == VTK_MP_CONTROLLER_ANY_SOURCE)
    {
    return receiveFromAnySource(data, length, tag, this->Sockets,
				this->IsConnected, this->NumberOfProcesses);
    }
  return receiveMessage(data, length, tag, this->Sockets[remoteProcessId]);
}


int vtkSocketCommunicator::WaitForConnection(int port, int timeout, 
					     int processId)
{

  if ( checkForError(processId, this->NumberOfProcesses) )
    {
    return 0;
    }

  if ( this->IsConnected[processId] )
    {
    vtkErrorMacro("Port " << processId << " is occupied.");
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
  this->Sockets[processId] = accept(sock, 0, 0);
  if ( this->Sockets[processId] == -1 )
    {
    vtkErrorMacro("Error in accept.");
    return 0;
    }
  vtkCloseSocketMacro(sock);
    
  this->IsConnected[processId] = 1;
  return 1;
}

void vtkSocketCommunicator::CloseConnection ( int processId )
{
  if ( this->IsConnected[processId] )
    {
    vtkCloseSocketMacro(this->Sockets[processId]);
    this->IsConnected[processId] = 0;
    }
}

int vtkSocketCommunicator::ConnectTo ( char* hostName, int port, 
					int processId )
{

  if ( checkForError(processId, this->NumberOfProcesses) )
    {
    return 0;
    }

  if ( this->IsConnected[processId] )
    {
    vtkErrorMacro("Communicator port " << processId << " is occupied.");
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

  this->Sockets[processId] = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in name;
  name.sin_family = AF_INET;
  memcpy(&name.sin_addr, hp->h_addr, hp->h_length);
  name.sin_port = htons(port);

  if( connect(this->Sockets[processId], (sockaddr *)&name, sizeof(name)) < 0)
    {
    vtkErrorMacro("Can not connect to " << hostName << " on port " << port);
    return 0;
    }

  vtkDebugMacro("Connected to " << hostName << " on port " << port);
  this->IsConnected[processId] = 1;
  return 1;

}




