/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSocketCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// vtkCommunicator vtkSocketController

#ifndef __vtkSocketCommunicator_h
#define __vtkSocketCommunicator_h

#include "vtkCommunicator.h"

#include "vtkByteSwap.h" // Needed for vtkSwap macros

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

class vtkClientSocket;
class vtkServerSocket;

class VTK_PARALLEL_EXPORT vtkSocketCommunicator : public vtkCommunicator
{
public:
  static vtkSocketCommunicator *New();
  vtkTypeRevisionMacro(vtkSocketCommunicator,vtkCommunicator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Wait for connection on a given port.
  // These methods return 1 on success, 0 on error.
  virtual int WaitForConnection(int port);
  virtual int WaitForConnection(vtkServerSocket* socket,
    unsigned  long msec = 0);

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
  int GetIsConnected();

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

  //BTX
  // Description:
  // Get/Set the output stream to which communications should be
  // logged.  This is intended as a debugging feature.
  virtual void SetLogStream(ostream* stream);
  virtual ostream* GetLogStream();
  //ETX
  
  // Description:
  // Log messages to the given file.  The file is truncated unless the
  // second argument is non-zero (default is to truncate).  If the
  // file name is empty or NULL, logging is disabled.  Returns 0 if
  // the file failed to open, and 1 otherwise.
  virtual int LogToFile(const char* name);
  virtual int LogToFile(const char* name, int append);

  // Description:
  // If ReportErrors if false, all vtkErrorMacros are suppressed.
  vtkSetMacro(ReportErrors, int);
  vtkGetMacro(ReportErrors, int);

  // Description:
  // Get/Set the actual socket used for communication.
  vtkGetObjectMacro(Socket, vtkClientSocket);
  void SetSocket(vtkClientSocket*);

  // Description:
  // Performs ServerSide handshake.
  int ServerSideHandshake();

  // Description:
  // Performs ClientSide handshake.
  int ClientSideHandshake();
protected:

  vtkClientSocket* Socket;
  int NumberOfProcesses;
  int SwapBytesInReceivedData;
  int PerformHandshake;
  
  int ReportErrors;

  ofstream* LogFile;
  ostream* LogStream;
  
  vtkSocketCommunicator();
  ~vtkSocketCommunicator();
  
  // Wrappers around send/recv calls to implement loops.  Return 1 for
  // success, and 0 for failure.
  int SendTagged(void* data, int wordSize, int numWords, int tag,
                 const char* logName);
  int ReceiveTagged(void* data, int wordSize, int numWords, int tag,
                    const char* logName);
  int ReceivePartialTagged(void* data, int wordSize, int numWords, int tag,
                    const char* logName);
  
  // Internal utility methods.
  void LogTagged(const char* name, void* data, int wordSize, int numWords,
                 int tag, const char* logName);
  int CheckForErrorInternal(int id);
private:
  vtkSocketCommunicator(const vtkSocketCommunicator&);  // Not implemented.
  void operator=(const vtkSocketCommunicator&);  // Not implemented.

  int SelectSocket(int socket, unsigned long msec);

//BTX
  // SwapBytesInReceiveData needs an invalid / not set.
  // This avoids checking length of endian handshake.
  enum ErrorIds {
    SwapOff = 0,
    SwapOn,
    SwapNotSet
  };
//ETX
};

#endif
