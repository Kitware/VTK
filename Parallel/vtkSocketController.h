/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSocketController.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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

#if !defined(_WIN32) || defined(__CYGWIN__)
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <netdb.h>
 #include <unistd.h>
#endif

#include "vtkMultiProcessController.h"
#include "vtkSocketCommunicator.h"

class VTK_PARALLEL_EXPORT vtkSocketController : public vtkMultiProcessController
{
public:
  static vtkSocketController *New();
  vtkTypeRevisionMacro(vtkSocketController,vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is for initialiazing sockets.
  // One of these is REQUIRED for Windows.
  virtual void Initialize(int* argc, char*** argv, int intitalizedExternally)
    { this->Initialize(argc,argv); }
  virtual void Initialize(int* argc, char*** argv); 
  virtual void Initialize()
    { this->Initialize(0,0); }

  // Description:
  // Does not apply to sockets. Does nothing.
  void Finalize() {};
  void Finalize(int finalizedExternally) {};

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

  // Initialize only once, finialize on destruction.
  static int Initialized;
private:
  vtkSocketController(const vtkSocketController&);  // Not implemented.
  void operator=(const vtkSocketController&);  // Not implemented.
};


#endif // __vtkSocketController_h
