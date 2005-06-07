/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiProcessController.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiProcessController - Multiprocessing communication superclass
// .SECTION Description
// vtkMultiProcessController is used to control multiple processes/threads
// in a shared memory/distributed computing environment. It has
// methods for executing single/multiple method(s) on multiple processors,
// triggering registered callbacks (Remote Methods) (AddRMI(), TriggerRMI())
// and communication. Please note that the communication is done using
// the communicator which is accessible to the user. Therefore it is
// possible to get the communicator with GetCommunicator() and use
// it to send and receive data. This is the encoured communication method.
// The internal (RMI) communications are done using a second internal
// communicator (called RMICommunicator). For the threaded controllers,
// this is identical to the user communicator. However, for the MPI
// controller, this is a communicator which shares the same process
// group as the user communicator but uses a different context. Therefore,
// the user and the internal communications can not interfere with each
// other (even if the same message tag is used).
//
// .SECTION Note
// The internal behavior can be changed using the environment variable: 
// VTK_CONTROLLER ( can be 'MPI' or 'Threaded' ). By defaut a vtkDummyController
// is created
//
// .SECTION see also
// vtkMPIController vtkThreadedController vtkInputPort vtkOutputPort 
// vtkCommunicator vtkSharedMemoryCommunicator vtkMPICommunicator

#ifndef __vtkMultiProcessController_h
#define __vtkMultiProcessController_h

#include "vtkObject.h"

#include "vtkCommunicator.h" // Needed for direct access to communicator

class vtkDataSet;
class vtkImageData;
class vtkCollection;
class vtkOutputWindow;
class vtkDataObject;
class vtkMultiProcessController;

//BTX
// The type of function that gets called when new processes are initiated.
typedef void (*vtkProcessFunctionType)(vtkMultiProcessController *controller, 
                                       void *userData);

// The type of function that gets called when an RMI is triggered.
typedef void (*vtkRMIFunctionType)(void *localArg, 
                                   void *remoteArg, int remoteArgLength, 
                                   int remoteProcessId);
//ETX


class VTK_PARALLEL_EXPORT vtkMultiProcessController : public vtkObject
{
public:
  static vtkMultiProcessController *New();
  vtkTypeRevisionMacro(vtkMultiProcessController,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is for setting up the processes.
  // If a subclass needs to initialize process communication (i.e. MPI)
  // it would over ride this method.
  virtual void Initialize(int* vtkNotUsed(argc), char*** vtkNotUsed(argv))=0;

  // Description:
  // This method is for setting up the processes.
  // If a subclass needs to initialize process communication (i.e. MPI)
  // it would over ride this method.  Provided for initialization outside vtk.
  virtual void Initialize(int* vtkNotUsed(argc), char*** vtkNotUsed(argv),
                          int initializedExternally)=0;

  // Description:
  // This method is for cleaning up.
  // If a subclass needs to clean up process communication (i.e. MPI)
  // it would over ride this method.
  virtual void Finalize()=0;

  // Description:
  // This method is for cleaning up.
  // If a subclass needs to clean up process communication (i.e. MPI)
  // it would over ride this method.  Provided for finalization outside vtk.
  virtual void Finalize(int finalizedExternally)=0;

  // Description:
  // Set the number of processes you will be using.  This defaults
  // to the maximum number available.  If you set this to a value
  // higher than the default, you will get an error.
  virtual void SetNumberOfProcesses(int num);
  vtkGetMacro( NumberOfProcesses, int );

  //BTX
  // Description:
  // Set the SingleMethod to f() and the UserData of the
  // for the method to be executed by all of the processes
  // when SingleMethodExecute is called.  All the processes will
  // start by calling this function.
  void SetSingleMethod(vtkProcessFunctionType, void *data);
  //ETX

  // Description:
  // Execute the SingleMethod (as define by SetSingleMethod) using
  // this->NumberOfProcesses processes.  This will only return when
  // all the processes finish executing their methods.
  virtual void SingleMethodExecute() = 0;
  
  //BTX
  // Description:
  // Set the MultipleMethod to f() and the UserData of the
  // for the method to be executed by the process index
  // when MultipleMethodExecute is called.  This is for having each 
  // process start with a different function and data argument.
  void SetMultipleMethod(int index, vtkProcessFunctionType, void *data); 
  //ETX

  // Description:
  // Execute the MultipleMethods (as define by calling SetMultipleMethod
  // for each of the required this->NumberOfProcesses methods) using
  // this->NumberOfProcesses processes.
  virtual void MultipleMethodExecute() = 0;

  // Description:
  // Tells you which process [0, NumProcess) you are in.
  vtkGetMacro(LocalProcessId, int);

  // Description:
  // This convenience method returns the controller associated with the 
  // local process.  It returns NULL until the processes are spawned.
  // It is better if you hang on to the controller passed as an argument to the
  // SingleMethod or MultipleMethod functions.
  static vtkMultiProcessController *GetGlobalController();

  // Description:
  // This method can be used to tell the controller to create
  // a special output window in which all messages are preceded
  // by the process id.
  virtual void CreateOutputWindow() = 0;
  
  // Description:
  // By default, sending objects use shallow copy whenever possible.
  // This flag forces the controller to use deep copies instead.
  // This is necessary when asynchronous processing occurs 
  // (i.e. pipeline parallelism). This is only important when using
  // vtkThreadedController.
  vtkSetMacro(ForceDeepCopy, int);
  vtkGetMacro(ForceDeepCopy, int);
  vtkBooleanMacro(ForceDeepCopy, int);

  
  //------------------ RMIs --------------------
  //BTX
  // Description:
  // Register remote method invocation in the receiving process
  // which makes the call.  It must have a unique tag as an RMI id.
  // The vtkRMIFunctionType has several arguments: localArg (same as passed in),
  // remoteArg, remoteArgLength (memory passed by process triggering the RMI),
  // remoteProcessId.
  void AddRMI(vtkRMIFunctionType, void *localArg, int tag);
  
  // Description:
  // Remove the first RMI matching the tag.
  int RemoveFirstRMI(int tag);

  // Description:
  // Take an RMI away.
  void RemoveRMI(vtkRMIFunctionType f, void *arg, int tag)
    {f = f; arg = arg; tag = tag; vtkErrorMacro("RemoveRMI Not Implemented Yet");};
  //ETX
  
  // Description:
  // A method to trigger a method invocation in another process.
  void TriggerRMI(int remoteProcessId, void *arg, int argLength, int tag);

  // Description:
  // A conveniance method.  Called on process 0 to break "ProcessRMIs" loop
  // on all other processes.
  void TriggerBreakRMIs();

  // Description:
  // Convenience method when the arg is a string. 
  void TriggerRMI(int remoteProcessId, const char *arg, int tag) 
    { this->TriggerRMI(remoteProcessId, (void*)arg, 
                       static_cast<int>(strlen(arg))+1, tag); }

  // Description:
  // Convenience method when there is no argument.
  void TriggerRMI(int remoteProcessId, int tag)
    { this->TriggerRMI(remoteProcessId, NULL, 0, tag); }

  // Description:
  // Calling this method gives control to the controller to start
  // processing RMIs. Possible return values are:
  // RMI_NO_ERROR,
  // RMI_TAG_ERROR : rmi tag could not be received,
  // RMI_ARG_ERROR : rmi arg could not be received.
  // If reportErrors is false, no vtkErrorMacro is called.
  // ProcessRMIs() calls ProcessRMIs(int) with reportErrors = 0.
  int ProcessRMIs(int reportErrors);
  int ProcessRMIs();

  // Description:
  // Setting this flag to 1 will cause the ProcessRMIs loop to return.
  // This also causes vtkUpStreamPorts to return from
  // their WaitForUpdate loops.
  vtkSetMacro(BreakFlag, int);
  vtkGetMacro(BreakFlag, int);

  // Description:
  vtkGetObjectMacro(Communicator, vtkCommunicator);
  
//BTX

  enum Errors 
  {
    RMI_NO_ERROR,
    RMI_TAG_ERROR,
    RMI_ARG_ERROR
  };

  enum Consts 
  {
    MAX_PROCESSES  = 8192,
    ANY_SOURCE     = -1,
    INVALID_SOURCE = -2,
    RMI_TAG        = 315167,
    RMI_ARG_TAG    = 315168,
    BREAK_RMI_TAG  = 239954
  };

//ETX

  // Description:
  // This method can be used to synchronize processes/threads.
  virtual void Barrier() = 0;

  static void SetGlobalController(vtkMultiProcessController *controller);

  //------------------ Communication --------------------
  
  // Description:
  // This method sends data to another process.  Tag eliminates ambiguity
  // when multiple sends or receives exist in the same process.
  int Send(int* data, int length, int remoteProcessId, int tag);
  int Send(unsigned long* data, int length, int remoteProcessId, 
           int tag);
  int Send(char* data, int length, int remoteProcessId, int tag);
  int Send(unsigned char* data, int length, int remoteProcessId, int tag);
  int Send(float* data, int length, int remoteProcessId, int tag);
  int Send(double* data, int length, int remoteProcessId, int tag);
#ifdef VTK_USE_64BIT_IDS
  int Send(vtkIdType* data, int length, int remoteProcessId, int tag);
#endif
  int Send(vtkDataObject *data, int remoteId, int tag);
  int Send(vtkDataArray *data, int remoteId, int tag);

  // Description:
  // This method receives data from a corresponding send. It blocks
  // until the receive is finished.  It calls methods in "data"
  // to communicate the sending data.
  int Receive(int* data, int length, int remoteProcessId, int tag);
  int Receive(unsigned long* data, int length, int remoteProcessId, 
              int tag);
  int Receive(char* data, int length, int remoteProcessId, int tag);
  int Receive(unsigned char* data, int length, int remoteProcessId, int tag);
  int Receive(float* data, int length, int remoteProcessId, int tag);
  int Receive(double* data, int length, int remoteProcessId, int tag);
#ifdef VTK_USE_64BIT_IDS
  int Receive(vtkIdType* data, int length, int remoteProcessId, int tag);
#endif
  int Receive(vtkDataObject* data, int remoteId, int tag);
  int Receive(vtkDataArray* data, int remoteId, int tag);

// Internally implemented RMI to break the process loop.

protected:
  vtkMultiProcessController();
  ~vtkMultiProcessController();
  
  int MaximumNumberOfProcesses;
  int NumberOfProcesses;

  int LocalProcessId;
  
  vtkProcessFunctionType      SingleMethod;
  void                       *SingleData;
  vtkProcessFunctionType      MultipleMethod[MAX_PROCESSES];
  void                       *MultipleData[MAX_PROCESSES];  
  
  vtkCollection *RMIs;
  
  // This is a flag that can be used by the ports to break
  // their update loop. (same as ProcessRMIs)
  int BreakFlag;

  void ProcessRMI(int remoteProcessId, void *arg, int argLength, int rmiTag);

  // This method implements "GetGlobalController".  
  // It needs to be virtual and static.
  virtual vtkMultiProcessController *GetLocalController();

  
  // This flag can force deep copies during send.
  int ForceDeepCopy;

  vtkOutputWindow* OutputWindow;

  // Note that since the communicators can be created differently
  // depending on the type of controller, the subclasses are
  // responsible of deleting them.
  vtkCommunicator* Communicator;

  // Communicator which is a copy of the current user
  // level communicator except the context; i.e. even if the tags 
  // are the same, the RMI messages will not interfere with user 
  // level messages. (This only works with MPI. When using threads,
  // the tags have to be unique.)
  // Note that since the communicators can be created differently
  // depending on the type of controller, the subclasses are
  // responsible of deleting them.
  vtkCommunicator* RMICommunicator;

private:
  vtkMultiProcessController(const vtkMultiProcessController&);  // Not implemented.
  void operator=(const vtkMultiProcessController&);  // Not implemented.
};


inline int vtkMultiProcessController::Send(vtkDataObject *data, 
                                           int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Send(data, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Send(vtkDataArray *data, 
                                           int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Send(data, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Send(int* data, int length, 
                                           int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Send(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Send(unsigned long* data, 
                                           int length, int remoteThreadId, 
                                           int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Send(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Send(char* data, int length, 
                                           int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Send(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Send(unsigned char* data, int length, 
                                           int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Send(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Send(float* data, int length, 
                                           int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Send(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Send(double* data, int length, 
                                           int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Send(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

#ifdef VTK_USE_64BIT_IDS
inline int vtkMultiProcessController::Send(vtkIdType* data, int length, 
                                           int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Send(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}
#endif

inline int vtkMultiProcessController::Receive(vtkDataObject* data, 
                                              int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Receive(data, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Receive(vtkDataArray* data, 
                                              int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Receive(data, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Receive(int* data, int length, 
                                              int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Receive(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Receive(unsigned long* data, 
                                              int length,int remoteThreadId, 
                                              int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Receive(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Receive(char* data, int length, 
                                              int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Receive(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Receive(unsigned char* data, int length, 
                                              int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Receive(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Receive(float* data, int length, 
                                              int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Receive(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

inline int vtkMultiProcessController::Receive(double* data, int length, 
                                              int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Receive(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}

#ifdef VTK_USE_64BIT_IDS
inline int vtkMultiProcessController::Receive(vtkIdType* data, int length, 
                                              int remoteThreadId, int tag)
{
  if (this->Communicator)
    {
    return this->Communicator->Receive(data, length, remoteThreadId, tag);
    }
  else
    {
    return 0;
    }
}
#endif

#endif
