/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkMultiProcessController.h
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
// .NAME vtkMultiProcessController - Multiprocessing communication superclass
// .SECTION Description
// vtkMultiProcessController supplies an API for sending and receiving
// message between processes.  The controller also defines calls for
// sending and receiving vtkDataObjects, and remote method invocations.

// .SECTION see also
// vtkMPIController vtkThreadedController

#ifndef __vtkMultiProcessController_h
#define __vtkMultiProcessController_h

#include "vtkObject.h"
#include "vtkDataObject.h"
class vtkDataSet;
class vtkImageData;
class vtkCollection;


#define VTK_MP_CONTROLLER_MAX_PROCESSES 256
#define VTK_MP_CONTROLLER_ANY_SOURCE -1
#define VTK_MP_CONTROLLER_INVALID_SOURCE -2

// Internally implememented RMI to break the process loop.
#define VTK_BREAK_RMI_TAG           239954


class vtkMultiProcessController;


//BTX
// The type of function that gets called when new processes are initiated.
typedef void (*vtkProcessFunctionType)(int id, int numProcs, 
                                       vtkMultiProcessController *controller, 
                                       void *userData);

// The type of function that gets called when an RMI is triggered.
typedef void (*vtkRMIFunctionType)(void *localArg, 
                                   void *remoteArg, int remoteArgLength, 
                                   int remoteProcessId);
//ETX


class VTK_EXPORT vtkMultiProcessController : public vtkObject
{
public:
  static vtkMultiProcessController *New();
  vtkTypeMacro(vtkMultiProcessController,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is for setting up the processes.
  // If a subclass needs to initialize process communication (i.e. MPI)
  // it would over ride this method.
  virtual void Initialize(int vtkNotUsed(argc), char *arcv[]) {arcv=arcv;}

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
  // when SingleMethodExecute is called.
  void SetSingleMethod(vtkProcessFunctionType, void *data);
  //ETX

  // Description:
  // Execute the SingleMethod (as define by SetSingleMethod) using
  // this->NumberOfProcesses processes.  You should not expect this to return.
  virtual void SingleMethodExecute() = 0;
  
  //BTX
  // Description:
  // Set the MultipleMethod to f() and the UserData of the
  // for the method to be executed by the process index
  // when MultipleMethodExecute is called.
  void SetMultipleMethod(int index, vtkProcessFunctionType, void *data); 
  //ETX

  // Description:
  // Execute the MultipleMethods (as define by calling SetMultipleMethod
  // for each of the required this->NumberOfProcesses methods) using
  // this->NumberOfProcesses processes.
  virtual void MultipleMethodExecute() = 0;
  
  // Description:
  // Tells you which process [0, NumProcess) you are in.
  virtual int GetLocalProcessId() { return this->LocalProcessId; }
  
  //------------------ Communication --------------------
  
  // Description:
  // This method sends an object to another process.  Tag eliminates ambiguity
  // when multiple sends or receives exist in the same process.
  virtual int Send(vtkDataObject *data, int remoteProcessId, int tag);
  
  // Description:
  // Subclass have to supply these methods to send various arrays of data.
  virtual int Send(int *data, int length, int remoteProcessId, int tag) = 0;
  virtual int Send(unsigned long *data, int length, 
		   int remoteProcessId, int tag) = 0;
  virtual int Send(char *data, int length, 
		   int remoteProcessId, int tag) = 0;
  virtual int Send(float *data, int length, 
		   int remoteProcessId, int tag) = 0;

  // Description:
  // This method receives data from a corresponding send. It blocks
  // until the receive is finished.  It calls methods in "data"
  // to communicate the sending data.
  virtual int Receive(vtkDataObject *data, int remoteProcessId, int tag);

  // Description:
  // Subclass have to supply these methods to receive various arrays of data.
  // The methods also have to support a remoteProcessId of 
  // VTK_MP_CONTROLLER_ANY_SOURCE
  virtual int Receive(int *data, int length, int remoteProcessId, int tag) = 0;
  virtual int Receive(unsigned long *data, int length, 
		      int remoteProcessId, int tag) = 0;
  virtual int Receive(char *data, int length, 
		      int remoteProcessId, int tag) = 0;
  virtual int Receive(float *data, int length, 
		      int remoteProcessId, int tag) = 0;
  
  // Description:
  // By default, sending objects use shallow copy whenever possible.
  // This flag forces the controller to use deep copies instead.
  // This is necessary when asyncronous processing occurs 
  // (i.e. pipeline parallism). Right now, it is important that all the
  // controllers in the different processes agree to force a deep copy.
  // Deep copy is not implmented and just uses mashalling in the threaded
  // controller.
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
  // Take an RMI away.
  void RemoveRMI(void (*f)(void *localArg, void *remoteArg, int remoteProcessId), 
                 void *arg, int tag)
    {f = f; arg = arg; tag = tag; vtkErrorMacro("RemoveRMI Not Implemented Yet");};
  //ETX
  
  // Description:
  // A method to trigger a method invocation in another process.
  void TriggerRMI(int remoteProcessId, void *arg, int argLength, int tag);

  // Description:
  // Convenience method when the arg is a string. (Do we need a +1 for the length?)
  void TriggerRMI(int remoteProcessId, char *arg, int tag) 
    { this->TriggerRMI(remoteProcessId, (void*)arg, strlen(arg), tag); }

  // Description:
  // Convenience method when there is no argument.
  void TriggerRMI(int remoteProcessId, int tag)
    { this->TriggerRMI(remoteProcessId, NULL, 0, tag); }

  // Description:
  // Calling this method gives control to the controller to start
  // processing RMIs.
  void ProcessRMIs();

  // Description:
  // This will cause the ProcessRMIs loop to return.
  // This also causes vtkUpStreamPorts to return from
  // their WaitForUpdate loops.
  vtkSetMacro(BreakFlag, int);
  vtkGetMacro(BreakFlag, int);
  
  //------------------ Timing --------------------
  // Description:
  // For perfomance monitoring, reading and writing polydata to strings
  // are timed.  Access to these times is provided by these methods.
  vtkGetMacro(WriteTime, float);
  vtkGetMacro(ReadTime, float);
  vtkGetMacro(SendWaitTime, float);
  vtkGetMacro(SendTime, float);
  vtkGetMacro(ReceiveWaitTime, float);
  vtkGetMacro(ReceiveTime, float);

protected:
  vtkMultiProcessController();
  ~vtkMultiProcessController();
  vtkMultiProcessController(const vtkMultiProcessController&) {};
  void operator=(const vtkMultiProcessController&) {};
  
  int MaximumNumberOfProcesses;
  int NumberOfProcesses;
  // Since we cannot use this ivar in vtkThreadController subclass,
  // maybe we should elimnated it from this superclass.
  int LocalProcessId;
  
  vtkProcessFunctionType      SingleMethod;
  void                       *SingleData;
  vtkProcessFunctionType      MultipleMethod[VTK_MP_CONTROLLER_MAX_PROCESSES];
  void                       *MultipleData[VTK_MP_CONTROLLER_MAX_PROCESSES];  
  
  vtkCollection *RMIs;
  
  char *MarshalString;
  int MarshalStringLength;
  // The data may not take up all of the string.
  int MarshalDataLength;
  
  // This is a flag that can be used by the ports to break
  // their update loop. (same as ProcessRMIs)
  int BreakFlag;

  // convenience method
  void DeleteAndSetMarshalString(char *str, int strLength);
  
  // Write and read from marshal string
  // return 1 success, 0 fail
  int WriteObject(vtkDataObject *object);
  int ReadObject(vtkDataObject *object);
  
  int WriteDataSet(vtkDataSet *object);
  int ReadDataSet(vtkDataSet *object);

  int WriteImageData(vtkImageData *object);
  int ReadImageData(vtkImageData *object);

  void ProcessRMI(int remoteProcessId, void *arg, int argLength, int rmiTag);

  float ReadTime;
  float WriteTime;

  float SendWaitTime;
  float SendTime;
  float ReceiveWaitTime;
  float ReceiveTime;

  // This flag can force deep copies during send.
  int ForceDeepCopy;
};


#endif









