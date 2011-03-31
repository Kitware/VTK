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
  vtkTypeMacro(vtkSocketCommunicator,vtkCommunicator);
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
  virtual int ConnectTo(const char* hostName, int port);

  // Description:
  // Returns 1 if bytes must be swapped in received ints, floats, etc
  vtkGetMacro(SwapBytesInReceivedData, int);

  // Description:
  // Is the communicator connected?.
  int GetIsConnected();

  // Description:
  // Set the number of processes you will be using.
  virtual void SetNumberOfProcesses(int num);

  //------------------ Communication --------------------

  // Description:
  // Performs the actual communication.  You will usually use the convenience
  // Send functions defined in the superclass.
  virtual int SendVoidArray(const void *data, vtkIdType length, int type,
                            int remoteHandle, int tag);
  virtual int ReceiveVoidArray(void *data, vtkIdType length, int type,
                               int remoteHandle, int tag);

  // Description:
  // This class foolishly breaks the conventions of the superclass, so this
  // overload fixes the method.
  virtual void Barrier();

  // Description:
  // This class foolishly breaks the conventions of the superclass, so the
  // default implementations of these methods do not work.  These just give
  // errors instead.
  virtual int BroadcastVoidArray(void *data, vtkIdType length, int type,
                                 int srcProcessId);
  virtual int GatherVoidArray(const void *sendBuffer, void *recvBuffer,
                              vtkIdType length, int type, int destProcessId);
  virtual int GatherVVoidArray(const void *sendBuffer, void *recvBuffer,
                               vtkIdType sendLength, vtkIdType *recvLengths,
                               vtkIdType *offsets, int type, int destProcessId);
  virtual int ScatterVoidArray(const void *sendBuffer, void *recvBuffer,
                               vtkIdType length, int type, int srcProcessId);
  virtual int ScatterVVoidArray(const void *sendBuffer, void *recvBuffer,
                                vtkIdType *sendLengths, vtkIdType *offsets,
                                vtkIdType recvLength, int type,
                                int srcProcessId);
  virtual int AllGatherVoidArray(const void *sendBuffer, void *recvBuffer,
                                 vtkIdType length, int type);
  virtual int AllGatherVVoidArray(const void *sendBuffer, void *recvBuffer,
                                  vtkIdType sendLength, vtkIdType *recvLengths,
                                  vtkIdType *offsets, int type);
  virtual int ReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                              vtkIdType length, int type,
                              int operation, int destProcessId);
  virtual int ReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                              vtkIdType length, int type,
                              Operation *operation, int destProcessId);
  virtual int AllReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                                 vtkIdType length, int type,
                                 int operation);
  virtual int AllReduceVoidArray(const void *sendBuffer, void *recvBuffer,
                                 vtkIdType length, int type,
                                 Operation *operation);

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
  // Performs handshake. This uses vtkClientSocket::ConnectingSide to decide
  // whether to perform ServerSideHandshake or ClientSideHandshake. 
  int Handshake();

  // Description:
  // Performs ServerSide handshake.
  // One should preferably use Handshake() which calls ServerSideHandshake or
  // ClientSideHandshake as required.
  int ServerSideHandshake();

  // Description:
  // Performs ClientSide handshake.
  // One should preferably use Handshake() which calls ServerSideHandshake or
  // ClientSideHandshake as required.
  int ClientSideHandshake();

  // Description:
  // Returns true if this side of the socket is the server.  The result
  // is invalid if the socket is not connected.
  vtkGetMacro(IsServer, int);

  // Description:
  // Uniquely identifies the version of this class.  If the versions match,
  // then the socket communicators should be compatible.
  static int GetVersion();

  // Description:
  // This flag is cleared before vtkCommand::WrongTagEvent is fired when ever a
  // message with mismatched tag is received. If the handler wants the message
  // to be buffered for later use, it should set this flag to true. In which
  // case the vtkSocketCommunicator will  buffer the messsage and it will be
  // automatically processed the next time one does a ReceiveTagged() with a
  // matching tag.
  void BufferCurrentMessage()
    { this->BufferMessage = true; }

  // Description:
  // Returns true if there are any messages in the receive buffer.
  bool HasBufferredMessages();

//BTX
protected:

  vtkClientSocket* Socket;
  int SwapBytesInReceivedData;
  int RemoteHas64BitIds;
  int PerformHandshake;
  int IsServer;
  
  int ReportErrors;

  ofstream* LogFile;
  ostream* LogStream;
  
  vtkSocketCommunicator();
  ~vtkSocketCommunicator();
  
  // Wrappers around send/recv calls to implement loops.  Return 1 for
  // success, and 0 for failure.
  int SendTagged(const void* data, int wordSize, int numWords, int tag,
                 const char* logName);
  int ReceiveTagged(void* data, int wordSize, int numWords, int tag,
                    const char* logName);
  int ReceivePartialTagged(void* data, int wordSize, int numWords, int tag,
                    const char* logName);

  int ReceivedTaggedFromBuffer(
    void* data, int wordSize, int numWords, int tag, const char* logName);
  
  // Description:
  // Fix byte order for received data.
  void FixByteOrder(void* data, int wordSize, int numWords);

  // Internal utility methods.
  void LogTagged(const char* name, const void* data, int wordSize, int numWords,
                 int tag, const char* logName);
  int CheckForErrorInternal(int id);
  bool BufferMessage;
private:
  vtkSocketCommunicator(const vtkSocketCommunicator&);  // Not implemented.
  void operator=(const vtkSocketCommunicator&);  // Not implemented.

  int SelectSocket(int socket, unsigned long msec);

  // SwapBytesInReceiveData needs an invalid / not set.
  // This avoids checking length of endian handshake.
  enum ErrorIds {
    SwapOff = 0,
    SwapOn,
    SwapNotSet
  };

  // One may be tempted to change this to a vtkIdType, but really an int is
  // enough since we split messages > VTK_INT_MAX.
  int TagMessageLength;

  //  Buffer to save messages received with different tag than requested.
  class vtkMessageBuffer;
  vtkMessageBuffer* ReceivedMessageBuffer;
  
//ETX
};

#endif
