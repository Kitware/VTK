/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSocketCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSocketCommunicator.h"

#include "vtkClientSocket.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkServerSocket.h"
#include "vtkSocketController.h"
#include "vtkStdString.h"
#include "vtkTypeTraits.h"

#include <vtkstd/algorithm>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkSocketCommunicator);
vtkCxxRevisionMacro(vtkSocketCommunicator, "1.67");
vtkCxxSetObjectMacro(vtkSocketCommunicator, Socket, vtkClientSocket);
//----------------------------------------------------------------------------
vtkSocketCommunicator::vtkSocketCommunicator()
{
  this->Socket = NULL;
  this->NumberOfProcesses = 2;
  this->SwapBytesInReceivedData = vtkSocketCommunicator::SwapNotSet;
  this->RemoteHas64BitIds = -1; // Invalid until handshake.
  this->PerformHandshake = 1;
  this->LogStream = 0;
  this->LogFile = 0;

  this->ReportErrors = 1;
}

//----------------------------------------------------------------------------
vtkSocketCommunicator::~vtkSocketCommunicator()
{
  this->SetSocket(0);
  this->SetLogStream(0);
}

//----------------------------------------------------------------------------
void vtkSocketCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);


  os << indent << "SwapBytesInReceivedData: ";
  if (this->SwapBytesInReceivedData == SwapOff) 
    {
    os << "Off\n";
    }
  if (this->SwapBytesInReceivedData == SwapOn) 
    {
    os << "On\n";
    }
  if (this->SwapBytesInReceivedData == SwapNotSet) 
    {
    os << "NotSet\n";
    }
  os << indent << "RemoteHas64BitIds: "
     << (this->RemoteHas64BitIds ? "yes" : "no") << endl;
  os << indent << "Socket: ";
  if (this->Socket)
    {
    os << endl;
    this->Socket->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
    
    
  os << indent << "Perform a handshake: " 
     << ( this->PerformHandshake ? "Yes" : "No" ) << endl;

  os << indent << "ReportErrors: " << this->ReportErrors << endl;
}

//----------------------------------------------------------------------------
void vtkSocketCommunicator::SetLogStream(ostream* stream)
{
  if(this->LogStream != stream)
    {
    // If the log stream is our own log file, close the file.
    if(this->LogFile && this->LogFile == this->LogStream)
      {
      delete this->LogFile;
      this->LogFile = 0;
      }
    
    // Use the given log stream.
    this->LogStream = stream;
    }
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::GetIsConnected()
{
  if (this->Socket)
    {
    return this->Socket->GetConnected();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSocketCommunicator::SetNumberOfProcesses(int vtkNotUsed(num))
{
  vtkErrorMacro("Can not change the number of processes.");
  return;
}

//----------------------------------------------------------------------------
ostream* vtkSocketCommunicator::GetLogStream()
{
  return this->LogStream;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::LogToFile(const char* name)
{
  return this->LogToFile(name, 0);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::LogToFile(const char* name, int append)
{
  // Close old logging file.
  if(this->LogFile)
    {
    delete this->LogFile;
    this->LogFile = 0;
    }
  this->LogStream = 0;
  
  // Log to given file, if any.
  if(name && name[0])
    {
    this->LogFile = new ofstream(name, (ios::out |
                                        (append? ios::ate : ios::trunc)));
    if(!this->LogFile)
      {
      return 0;
      }
    if(!*this->LogFile)
      {
      delete this->LogFile;
      this->LogFile = 0;
      return 0;
      }
    this->LogStream = this->LogFile;
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSocketCommunicator::SendVoidArray(const void *data, vtkIdType length,
                                         int type, int remoteProcessId, int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }

#ifdef VTK_USE_64BIT_IDS
  // Special case for type ids.  If the remote does not have 64 bit ids, we
  // need to convert them before sending them.
  if ((type == VTK_ID_TYPE) && !this->RemoteHas64BitIds)
    {
    vtkstd::vector<int> newData;
    newData.resize(length);
    vtkstd::copy(reinterpret_cast<const vtkIdType *>(data),
                 reinterpret_cast<const vtkIdType *>(data) + length,
                 newData.begin());
    return this->SendVoidArray(&newData[0], length, VTK_INT,
                               remoteProcessId, tag);
    }
#endif

  int typeSize;
  vtkStdString typeName;
  switch (type)
    {
    vtkTemplateMacro(typeSize = sizeof(VTK_TT);
                     typeName = vtkTypeTraits<VTK_TT>().SizedName());
    default:
      vtkWarningMacro(<< "Invalid data type " << type);
      typeSize = 1;
      typeName = "???";
      break;
    }
  // Special case for logging.
  if (type == VTK_CHAR) typeName = "char";

  const char *byteData = reinterpret_cast<const char *>(data);
  int maxSend = VTK_INT_MAX/typeSize;
  // If sending an array longer than the maximum number that can be held
  // in an integer, break up the array into pieces.
  while (length > maxSend)
    {
    if (!this->SendTagged(byteData, typeSize, maxSend, tag, typeName))
      {
      return 0;
      }
    byteData += maxSend*typeSize;
    length -= maxSend;
    }
  return this->SendTagged(byteData, typeSize, length, tag, typeName);
}

//-----------------------------------------------------------------------------
int vtkSocketCommunicator::ReceiveVoidArray(void *data, vtkIdType length,
                                            int type, int remoteProcessId,
                                            int tag)
{
  if(this->CheckForErrorInternal(remoteProcessId)) { return 0; }

#ifdef VTK_USE_64BIT_IDS
  // Special case for type ids.  If the remote does not have 64 bit ids, we
  // need to convert them before sending them.
  if ((type == VTK_ID_TYPE) && !this->RemoteHas64BitIds)
    {
    vtkstd::vector<int> newData;
    newData.resize(length);
    int retval = this->ReceiveVoidArray(&newData[0], length, VTK_INT,
                                        remoteProcessId, tag);
    vtkstd::copy(newData.begin(), newData.end(),
                 reinterpret_cast<vtkIdType *>(data));
    return retval;
    }
#endif

  int typeSize;
  vtkStdString typeName;
  switch (type)
    {
    vtkTemplateMacro(typeSize = sizeof(VTK_TT);
                     typeName = vtkTypeTraits<VTK_TT>().SizedName());
    default:
      vtkWarningMacro(<< "Invalid data type " << type);
      typeSize = 1;
      typeName = "???";
      break;
    }
  // Special case for logging.
  if (type == VTK_CHAR) typeName = "char";

  char *byteData = reinterpret_cast<char *>(data);
  int maxReceive = VTK_INT_MAX/typeSize;
  // If receiving an array longer than the maximum number that can be held
  // in an integer, break up the array into pieces.
  while (length > maxReceive)
    {
    if (!this->ReceiveTagged(byteData, typeSize, maxReceive, tag, typeName))
      {
      return 0;
      }
    byteData += maxReceive*typeSize;
    length -= maxReceive;
    }
  int ret = this->ReceiveTagged(byteData, typeSize, length, tag, typeName);

  // Some crazy special crud for RMIs that may one day screw someone up in
  // a weird way.  No, I did not write this, but I'm sure there is code that
  // relies on it.
  if ((tag == vtkMultiProcessController::RMI_TAG) && (type == VTK_INT))
    {
    int *idata = reinterpret_cast<int *>(data);
    idata[2] = 1;
    }

  return ret;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::ServerSideHandshake()
{
  if ( this->PerformHandshake )
    {
    // Handshake to determine if the client machine has the same endianness
    char clientIsBE;
    if(!this->ReceiveTagged(&clientIsBE, static_cast<int>(sizeof(char)),
        1, vtkSocketController::ENDIAN_TAG, 0))
      {
      if (this->ReportErrors)
        {
        vtkErrorMacro("Endian handshake failed.");
        }
      return 0;
      }
    vtkDebugMacro(<< "Client is " << ( clientIsBE ? "big" : "little" ) 
      << "-endian");

#ifdef VTK_WORDS_BIGENDIAN
    char IAmBE = 1;
#else
    char IAmBE = 0;
#endif
    vtkDebugMacro(<< "I am " << ( IAmBE ? "big" : "little" ) << "-endian");
    if(!this->SendTagged(&IAmBE, static_cast<int>(sizeof(char)),
        1, vtkSocketController::ENDIAN_TAG, 0))
      {
      if (this->ReportErrors)
        {
        vtkErrorMacro("Endian handshake failed.");
        }
      return 0;
      }

    if ( clientIsBE != IAmBE )
      {
      this->SwapBytesInReceivedData = vtkSocketCommunicator::SwapOn;
      }
    else
      {
      this->SwapBytesInReceivedData = vtkSocketCommunicator::SwapOff;
      }

    // Handshake to determine if remote has 64 bit ids.
#ifdef VTK_USE_64BIT_IDS
    int IHave64BitIds = 1;
#else
    int IHave64BitIds = 0;
#endif
    if (!this->ReceiveTagged(&(this->RemoteHas64BitIds),
                             static_cast<int>(sizeof(int)), 1,
                             vtkSocketController::IDTYPESIZE_TAG, 0))
      {
      if (this->ReportErrors)
        {
        vtkErrorMacro("Id Type Size handshake failed.");
        }
      return 0;
      }
    vtkDebugMacro(<< "Remote has 64 bit ids: " << this->RemoteHas64BitIds);
    if (!this->SendTagged(&IHave64BitIds, static_cast<int>(sizeof(int)), 1,
                          vtkSocketController::IDTYPESIZE_TAG, 0))
      {
      if (this->ReportErrors)
        {
        vtkErrorMacro("Id Type Size handshake failed.");
        }
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::ClientSideHandshake()
{
  if (!this->PerformHandshake)
    {
    return 1;
    }

  // Handshake to determine if the server machine has the same endianness
#ifdef VTK_WORDS_BIGENDIAN
  char IAmBE = 1;
#else
  char IAmBE = 0;
#endif
  vtkDebugMacro(<< "I am " << ( IAmBE ? "big" : "little" ) << "-endian");
  if(!this->SendTagged(&IAmBE, static_cast<int>(sizeof(char)),
      1, vtkSocketController::ENDIAN_TAG, 0))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Endian handshake failed.");
      }
    return 0;
    }

  char serverIsBE;
  if (!this->ReceiveTagged(&serverIsBE, static_cast<int>(sizeof(char)), 1,
      vtkSocketController::ENDIAN_TAG, 0))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Endian handshake failed.");
      }
    return 0;
    }
  vtkDebugMacro(<< "Server is " << ( serverIsBE ? "big" : "little" ) << "-endian");

  if ( serverIsBE != IAmBE )
    {
    this->SwapBytesInReceivedData = vtkSocketCommunicator::SwapOn;
    }
  else
    {
    this->SwapBytesInReceivedData = vtkSocketCommunicator::SwapOff;
    }

  // Handshake to determine if remote has 64 bit ids.
#ifdef VTK_USE_64BIT_IDS
  int IHave64BitIds = 1;
#else
  int IHave64BitIds = 0;
#endif
  if (!this->SendTagged(&IHave64BitIds, static_cast<int>(sizeof(int)), 1,
                        vtkSocketController::IDTYPESIZE_TAG, 0))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Id Type Size handshake failed.");
      }
    return 0;
    }
  if (!this->ReceiveTagged(&(this->RemoteHas64BitIds),
                           static_cast<int>(sizeof(int)), 1,
                           vtkSocketController::IDTYPESIZE_TAG, 0))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Id Type Size handshake failed.");
      }
    return 0;
    }
  vtkDebugMacro(<< "Remote has 64 bit ids: " << this->RemoteHas64BitIds);

  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::WaitForConnection(int port)
{
  if ( this->GetIsConnected() )
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Communicator port " << 1 << " is occupied.");
      }
    return 0;
    }
  vtkServerSocket * soc = vtkServerSocket::New();
  if (soc->CreateServer(port) != 0)
    {
    soc->Delete();
    return 0;
    }
  int ret = this->WaitForConnection(soc);
  soc->Delete();
 
  return ret;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::WaitForConnection(vtkServerSocket* socket, 
  unsigned long msec/*=0*/)
{
  if ( this->GetIsConnected() )
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Communicator port " << 1 << " is occupied.");
      }
    return 0;
    }

  if (!socket)
    {
    return 0;
    }

  vtkClientSocket *cs= socket->WaitForConnection(msec);
  if (cs)
    {
    this->SetSocket(cs);
    cs->Delete();
    }

  if (!this->Socket)
    {
    return 0;
    }
  return this->ServerSideHandshake();
}

//----------------------------------------------------------------------------
void vtkSocketCommunicator::CloseConnection()
{
  if (this->Socket)
    {
    this->Socket->CloseSocket();
    this->Socket->Delete();
    this->Socket = 0;
    }
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::ConnectTo ( char* hostName, int port )
{

  if ( this->GetIsConnected() )
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Communicator port " << 1 << " is occupied.");
      }
    return 0;
    }

  vtkClientSocket* tmp = vtkClientSocket::New();
  
  if(tmp->ConnectToServer(hostName, port))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Can not connect to " << hostName << " on port " << port);
      }
    tmp->Delete();
    return 0;
    }
  this->SetSocket(tmp);
  tmp->Delete();
  
  vtkDebugMacro("Connected to " << hostName << " on port " << port);
  return this->ClientSideHandshake();
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::SendTagged(const void* data, int wordSize,
                                      int numWords, int tag,
                                      const char* logName)
{
  if(!this->Socket->Send(&tag, static_cast<int>(sizeof(int))))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Could not send tag.");
      }
    return 0;
    }
  int length = wordSize * numWords;
  if(!this->Socket->Send(&length, 
      static_cast<int>(sizeof(int))))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Could not send length.");
      }
    return 0;
    }  
  if(!this->Socket->Send(data, wordSize*numWords))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Could not send message.");
      }
    return 0;
    }
  
  // Log this event.
  this->LogTagged("Sent", data, wordSize, numWords, tag, logName);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::ReceiveTagged(void* data, int wordSize,
                                         int numWords, int tag,
                                         const char* logName)
{
  int success = 0;
  int length = -1;
  while ( !success )
    {
    int recvTag = -1;
    length = -1;
    if(!this->Socket->Receive(&recvTag,
        static_cast<int>(sizeof(int))))
      {
      if (this->ReportErrors)
        {
        vtkErrorMacro("Could not receive tag. " << tag);
        }
      return 0;
      }
    if(this->SwapBytesInReceivedData == vtkSocketCommunicator::SwapOn)
      {
      vtkSwap4(reinterpret_cast<char*>(&recvTag));
      }
    if(!this->Socket->Receive(&length,
        static_cast<int>(sizeof(int))))
      {
      if (this->ReportErrors)
        {
        vtkErrorMacro("Could not receive length.");
        }
      return 0;
      }
    if(this->SwapBytesInReceivedData == vtkSocketCommunicator::SwapOn)
      {
      vtkSwap4(reinterpret_cast<char*>(&length));
      }
    if(recvTag != tag)
      {
      char* idata = new char[length + sizeof(recvTag) + sizeof(length)];
      char* ptr = idata;
      memcpy(ptr, (void*)&recvTag, sizeof(recvTag));
      ptr += sizeof(recvTag);
      memcpy(ptr, (void*)&length, sizeof(length));
      ptr += sizeof(length);
      this->ReceivePartialTagged(ptr, 1, length, tag, "Wrong tag");
      int res = this->InvokeEvent(vtkCommand::WrongTagEvent, idata);
      delete [] idata;
      if ( res )
        {
        continue;
        }

      if (this->ReportErrors)
        {
        vtkErrorMacro("Tag mismatch: got " << recvTag << ", expecting " << tag
                      << ".");
        }
      return 0;
      }
    else
      {
      success = 1;
      }
    }
  // Length may not be correct for the first message sent as an 
  // endian handshake because the SwapBytesInReceivedData flag 
  // is not initialized at this point.  We could just initialize it
  // here, but what is the point.
  if ((wordSize * numWords) != length && 
      this->SwapBytesInReceivedData != vtkSocketCommunicator::SwapNotSet)
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Requested size (" << (wordSize * numWords) 
                    << ") is different than the size that was sent (" << length << ")");
      }
    return 0;
    }
  return this->ReceivePartialTagged(data, wordSize, numWords, tag, logName);
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::ReceivePartialTagged(void* data, int wordSize,
                                         int numWords, int tag,
                                         const char* logName)
{
  if(!this->Socket->Receive(data, wordSize*numWords))
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Could not receive message.");
      }
    return 0;
    }
  // Unless we're dealing with chars, then check byte ordering.
  // This is really bad and should probably use some enum for types
  if(this->SwapBytesInReceivedData == vtkSocketCommunicator::SwapOn)
    {
    if(wordSize == 4)
      {
      vtkDebugMacro(<< " swapping 4 range, size = " << wordSize 
                    << " length = " << numWords);
      vtkSwap4Range(reinterpret_cast<char*>(data), numWords);
      }
    else if(wordSize == 8)
      {
      vtkDebugMacro(<< " swapping 8 range, size = " << wordSize
                    << " length = " << numWords );
      vtkSwap8Range(reinterpret_cast<char*>(data), numWords);
      }
    }
  
  // Log this event.
  this->LogTagged("Received", data, wordSize, numWords, tag, logName);
  
  return 1;
}

//----------------------------------------------------------------------------
template <class T, class OutType>
void vtkSocketCommunicatorLogArray(ostream& os, T* array, int length, int max,
                                   OutType*)
{
  if(length > 0)
    {
    int num = (length <= max)? length:max;
    os << " data={" << static_cast<OutType>(array[0]);
    for(int i=1; i < num; ++i)
      {
      os << " " << static_cast<OutType>(array[i]);
      }
    if(length > max)
      {
      os << " ...";
      }
    os << "}";
    }
}

//----------------------------------------------------------------------------
void vtkSocketCommunicator::LogTagged(const char* name, const void* data,
                                      int wordSize, int numWords,
                                      int tag, const char* logName)
{
  if(this->LogStream)
    {
    // Log the general event information.
    *this->LogStream << name;
    if(logName)
      {
      *this->LogStream << " " << logName;
      }
    *this->LogStream << " data: tag=" << tag
                     << " wordSize=" << wordSize
                     << " numWords=" << numWords;
    
    // If this is a string, log the first 70 characters.  If this is
    // an array of data values, log the first few.
    if(wordSize == static_cast<int>(sizeof(char)) && logName &&
       (strcmp(logName, "char") == 0))
      {
      const char* chars = reinterpret_cast<const char*>(data);
      if((chars[numWords-1]) == 0 &&
         (static_cast<int>(strlen(chars)) == numWords-1))
        {
        // String data.  Display the first 70 characters.
        *this->LogStream << " data={";
        if(numWords <= 71)
          {
          *this->LogStream << chars;
          }
        else
          {
          this->LogStream->write(reinterpret_cast<const char*>(data), 70);
          *this->LogStream << " ...";
          }
        *this->LogStream << "}";
        }
      else
        {
        // Not string data.  Display the characters as integer values.
        vtkSocketCommunicatorLogArray(*this->LogStream,
                                      reinterpret_cast<const char*>(data),
                                      numWords, 6, static_cast<int*>(0));
        }
      }
    else if ((wordSize == 1) && logName && (strcmp(logName, "Int8") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                    reinterpret_cast<const vtkTypeInt8*>(data),
                                    numWords, 6, static_cast<vtkTypeInt16*>(0));
      }
    else if ((wordSize == 1) && logName && (strcmp(logName, "UInt8") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                   reinterpret_cast<const vtkTypeUInt8*>(data),
                                   numWords, 6, static_cast<vtkTypeUInt16*>(0));
      }
    else if ((wordSize == 2) && logName && (strcmp(logName, "Int16") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                    reinterpret_cast<const vtkTypeInt16*>(data),
                                    numWords, 6, static_cast<vtkTypeInt16*>(0));
      }
    else if ((wordSize == 2) && logName && (strcmp(logName, "UInt16") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                   reinterpret_cast<const vtkTypeUInt16*>(data),
                                   numWords, 6, static_cast<vtkTypeUInt16*>(0));
      }
    else if ((wordSize == 4) && logName && (strcmp(logName, "Int32") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                    reinterpret_cast<const vtkTypeInt32*>(data),
                                    numWords, 6, static_cast<vtkTypeInt32*>(0));
      }
    else if ((wordSize == 4) && logName && (strcmp(logName, "UInt32") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                   reinterpret_cast<const vtkTypeUInt32*>(data),
                                   numWords, 6, static_cast<vtkTypeUInt32*>(0));
      }
    else if ((wordSize == 8) && logName && (strcmp(logName, "Int64") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                    reinterpret_cast<const vtkTypeInt64*>(data),
                                    numWords, 6, static_cast<vtkTypeInt64*>(0));
      }
    else if ((wordSize == 8) && logName && (strcmp(logName, "UInt64") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                   reinterpret_cast<const vtkTypeUInt64*>(data),
                                   numWords, 6, static_cast<vtkTypeUInt64*>(0));
      }
    else if ((wordSize == 4) && logName && (strcmp(logName, "Float32") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                  reinterpret_cast<const vtkTypeFloat32*>(data),
                                  numWords, 6, static_cast<vtkTypeFloat32*>(0));
      }
    else if ((wordSize == 8) && logName && (strcmp(logName, "Float64") == 0))
      {
      vtkSocketCommunicatorLogArray(*this->LogStream,
                                  reinterpret_cast<const vtkTypeFloat64*>(data),
                                  numWords, 6, static_cast<vtkTypeFloat64*>(0));
      }
    *this->LogStream << "\n";
    }
}

//----------------------------------------------------------------------------
int vtkSocketCommunicator::CheckForErrorInternal(int id)
{
  if(id == 0)
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("Can not connect to myself!");
      }
    return 1;
    }
  else if(id >= this->NumberOfProcesses)
    {
    if (this->ReportErrors)
      {
      vtkErrorMacro("No port for process " << id << " exists.");
      }
    return 1;
    }
  return 0;
}
