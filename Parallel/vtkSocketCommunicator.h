/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSocketCommunicator.h
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
// .NAME vtkSocketCommunicator - Process communication using Sockets
// .SECTION Description
// This is a concrete implementation of vtkCommunicator which supports
// interprocess communication using BSD style sockets. 
// It supports byte swapping for the communication of  machines 
// with different endianness.

// .SECTION Caveats
// Communication between 32 bit and 64 bit systems is not fully
// supported. If a type does not have the same length on both
// systems, this communicator can not be used to transfer data
// of that type.

// .SECTION see also
// vtkCommunicator vtkSharedMemoryCommunicator vtkSocketController

#ifndef __vtkSocketCommunicator_h
#define __vtkSocketCommunicator_h

#if !defined(_WIN32) || defined(__CYGWIN__)
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <netdb.h>
 #include <unistd.h>
#endif

#include "vtkObject.h"
#include "vtkByteSwap.h"
#include "vtkCommunicator.h"

#ifdef VTK_WORDS_BIGENDIAN
# define vtkSwap4 vtkByteSwap::Swap4LE
# define vtkSwap4Range vtkByteSwap::Swap4LERange
# define vtkSwap8 vtkByteSwap::Swap8LE
# define vtkSwap8Range vtkByteSwap::Swap8LERange
#else
# define vtkSwap4 vtkByteSwap::Swap4BE
# define vtkSwap4Range vtkByteSwap::Swap4BERange
# define vtkSwap8 vtkByteSwap::Swap8BE
# define vtkSwap8Range vtkByteSwap::Swap8BERange
#endif

class VTK_PARALLEL_EXPORT vtkSocketCommunicator : public vtkCommunicator
{
public:
  static vtkSocketCommunicator *New();
  vtkTypeRevisionMacro(vtkSocketCommunicator,vtkCommunicator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Wait for connection on a given port.
  virtual int WaitForConnection(int port);

  // Description:
  // Close a connection.
  virtual void CloseConnection();

  // Description:
  // Open a connection to host.
  virtual int ConnectTo( char* hostName, int port);

  // Description:
  // Returns 1 if bytes must be swapped in received ints, floats, etc
  vtkGetMacro(SwapBytesInReceivedData, int);

  // Description:
  // Is the communicator connected?.
  vtkGetMacro(IsConnected, int);

  //------------------ Communication --------------------
  
  // Description:
  // This method sends data to another process.  Tag eliminates ambiguity
  // when multiple sends or receives exist in the same process.
  int Send(int *data, int length, int remoteProcessId, int tag);
  int Send(unsigned long *data, int length, int remoteProcessId, int tag);
  int Send(char *data, int length, int remoteProcessId, int tag);
  int Send(unsigned char *data, int length, int remoteProcessId, int tag);
  int Send(float *data, int length, int remoteProcessId, int tag);
  int Send(double *data, int length, int remoteProcessId, int tag);
#ifdef VTK_USE_64BIT_IDS
  int Send(vtkIdType *data, int length, int remoteProcessId, int tag);
#endif
  int Send(vtkDataObject *data, int remoteId, int tag)
    {return this->vtkCommunicator::Send(data,remoteId,tag);}
  int Send(vtkDataArray *data, int remoteId, int tag)
    {return this->vtkCommunicator::Send(data,remoteId,tag);}

  // Description:
  // This method receives data from a corresponding send. It blocks
  // until the receive is finished.  It calls methods in "data"
  // to communicate the sending data.
  int Receive(int *data, int length, int remoteProcessId, int tag);
  int Receive(unsigned long *data, int length, int remoteProcessId, int tag);
  int Receive(char *data, int length, int remoteProcessId, int tag);
  int Receive(unsigned char *data, int length, int remoteProcessId, int tag);
  int Receive(float *data, int length, int remoteProcessId, int tag);
  int Receive(double *data, int length, int remoteProcessId, int tag);
#ifdef VTK_USE_64BIT_IDS
  int Receive(vtkIdType *data, int length, int remoteProcessId, int tag);
#endif
  int Receive(vtkDataObject *data, int remoteId, int tag)
    {return this->vtkCommunicator::Receive(data, remoteId, tag);}
  int Receive(vtkDataArray *data, int remoteId, int tag)
    {return this->vtkCommunicator::Receive(data, remoteId, tag);}

  // Description:
  // Set or get the PerformHandshake ivar. If it is on, the communicator
  // will try to perform a handshake when connected.
  // It is on by default.
  vtkSetClampMacro(PerformHandshake, int, 0, 1);
  vtkBooleanMacro(PerformHandshake, int);
  vtkGetMacro(PerformHandshake, int);

  // Description:
  // Wait for message and store it in the data. The message
  // has maximum length of maxlength. It's actual length
  // is stored in length.
  int ReceiveMessage(char *data, int *length, int maxlength);

protected:

  int Socket;
  int IsConnected;
  int NumberOfProcesses;
  int SwapBytesInReceivedData;
  int PerformHandshake;

  vtkSocketCommunicator();
  ~vtkSocketCommunicator();

  int ReceiveMessage(char *data, int size, int length, int tag );

private:
  vtkSocketCommunicator(const vtkSocketCommunicator&);  // Not implemented.
  void operator=(const vtkSocketCommunicator&);  // Not implemented.
};

#endif
