/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkSocketCommunicator.h
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
// .NAME vtkSocketCommunicator - Process communication using Sockets

#ifndef __vtkSocketCommunicator_h
#define __vtkSocketCommunicator_h

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "vtkObject.h"
#include "vtkMultiProcessController.h"


class VTK_EXPORT vtkSocketCommunicator : public vtkMultiProcessController
{
public:
  static vtkSocketCommunicator *New();
  vtkTypeMacro(vtkSocketCommunicator,vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is for setting the sockets.
  // This is REQUIRED for Windows.
  virtual void Initialize(int vtkNotUsed(argc), char *arcv[]);

  // Description:
  // Execute the SingleMethod (as define by SetSingleMethod) using
  // this->NumberOfProcesses processes.  You should not expect this to return.
  void SingleMethodExecute() {};
  
  // Description:
  // Execute the MultipleMethods (as define by calling SetMultipleMethod
  // for each of the required this->NumberOfProcesses methods) using
  // this->NumberOfProcesses processes.
  void MultipleMethodExecute() {};

  // Description:
  // Set the number of processes you will be using.
  virtual void SetNumberOfProcesses(int num);

  // Description:
  // Wait for connection on a given port
  virtual int WaitForConnection(int port, int timeout, int processId);

  // Description:
  // Close a connection
  virtual void CloseConnection(int processId);

  // Description:
  // Open a connection to a give machine
  virtual int ConnectTo( char* hostName, int port, int processId );

  //------------------ Communication --------------------
  
  // Description:
  // This method sends data to another process.  Tag eliminates ambiguity
  // when multiple sends or receives exist in the same process.
  int Send(int *data, int length, int remoteProcessId, int tag);
  int Send(unsigned long *data, int length, int remoteProcessId, int tag);
  int Send(char *data, int length, int remoteProcessId, int tag);
  int Send(float *data, int length, int remoteProcessId, int tag);
  int Send(vtkDataObject *data, int remoteId, int tag)
    {return this->vtkMultiProcessController::Send(data,remoteId,tag);}

  // Description:
  // This method receives data from a corresponding send. It blocks
  // until the receive is finished.  It calls methods in "data"
  // to communicate the sending data.
  int Receive(int *data, int length, int remoteProcessId, int tag);
  int Receive(unsigned long *data, int length, int remoteProcessId, int tag);
  int Receive(char *data, int length, int remoteProcessId, int tag);
  int Receive(float *data, int length, int remoteProcessId, int tag);
  int Receive(vtkDataObject *data, int remoteId, int tag)
    {return this->vtkMultiProcessController::Receive(data, remoteId, tag);}

protected:

  int* Sockets;
  int* IsConnected;

  vtkSocketCommunicator();
  ~vtkSocketCommunicator();
  vtkSocketCommunicator(const vtkSocketCommunicator&) {};
  void operator=(const vtkSocketCommunicator&) {};

  // Initialize only once, finialize on destruction.
  int Initialized;

};


#endif


