/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkSocketController.h
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
// .NAME vtkSocketController - Process communication using Sockets
// .SECTION Description
// This is a concrete implementation of vtkMultiProcessController.
// It supports one-to-one communication using sockets. Note that
// process 0 will always correspond to self and process 1 to the
// remote process. This class is best used with ports.

// .SECTION see also
// vtkMultiProcessController vtkSocketCommunicator vtkInputPort vtkOutputPort

#ifndef __vtkSocketController_h
#define __vtkSocketController_h

#if defined(_WIN32) && !defined(__CYGWIN__)
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
#include "vtkSocketCommunicator.h"

class VTK_PARALLEL_EXPORT vtkSocketController : public vtkMultiProcessController
{
public:
  static vtkSocketController *New();
  vtkTypeMacro(vtkSocketController,vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is for initialiazing sockets.
  // One of these is REQUIRED for Windows.
  virtual void Initialize(int* , char***);
  virtual void Initialize()
    { this->Initialize(0,0); }

  // Description:
  // Does not apply to sockets. Does nothing.
  void Finalize() {};

  // Description:
  //  Does not apply to sockets. Does nothing.
  void SingleMethodExecute() {};
  
  // Description:
  //  Does not apply to sockets.  Does nothing.
  void MultipleMethodExecute() {};

  // Description:
  //  Does not apply to sockets. Does nothing.
  void CreateOutputWindow() {};

  // Description:
  //  Does not apply to sockets. Does nothing.
  void Barrier() {};

  // Description:
  // Set the number of processes you will be using.
  virtual void SetNumberOfProcesses(int num);

  // Description:
  // Wait for connection on a given port, forwarded
  // to the communicator
  virtual int WaitForConnection(int port)
    { 
    return vtkSocketCommunicator::SafeDownCast(this->Communicator)->
      WaitForConnection(port); 
    }

  // Description:
  // Close a connection, forwarded
  // to the communicator
  virtual void CloseConnection()
    { 
    vtkSocketCommunicator::SafeDownCast(this->Communicator)->
      CloseConnection(); 
    }

  // Description:
  // Open a connection to a give machine, forwarded
  // to the communicator
  virtual int ConnectTo( char* hostName, int port )
    { 
    return vtkSocketCommunicator::SafeDownCast(this->Communicator)->
      ConnectTo(hostName, port); 
    }

  int GetSwapBytesInReceivedData()
    {
    return vtkSocketCommunicator::SafeDownCast(this->Communicator)->
      GetSwapBytesInReceivedData();
    }

  // Description:
  // Set the communicator used in normal and rmi communications.
  void SetCommunicator(vtkSocketCommunicator* comm);

//BTX

  enum Consts {
    ENDIAN_TAG=1010580540 // 0x3c3c3c3c
  };

//ETX

protected:

  vtkSocketController();
  ~vtkSocketController();
  vtkSocketController(const vtkSocketController&);
  void operator=(const vtkSocketController&);

  // Initialize only once, finialize on destruction.
  static int Initialized;
};


#endif // __vtkSocketController_h
