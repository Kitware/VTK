/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkOutputPort.h
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
// .NAME vtkOutputPort - Connects pipelines in different processes.
// .SECTION Description
// OutputPort Connects the pipeline in this process to one in another 
// processes.  It communicates all the pipeline protocol so that
// the fact you are running in multiple processes is transparent.
// The output port is placed at the end of the pipeline
// (an output for a process).  It can have 
// multiple corresponding input ports in other processes that receive
// its data.  Updates in a port are triggered asynchronously, so
// filter with multiple inputs will take advantage of task parallelism.

// .SECTION see also
// vtkInputPort vtkMultiProcessController

#ifndef __vtkOutputPort_h
#define __vtkOutputPort_h

#include "vtkProcessObject.h"
#include "vtkMultiProcessController.h"


class VTK_PARALLEL_EXPORT vtkOutputPort : public vtkProcessObject
{
public:
  static vtkOutputPort *New();
  vtkTypeMacro(vtkOutputPort,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Should accept vtkDataObjects in the future.
  void SetInput(vtkDataObject *input);
  vtkDataObject *GetInput();
  
  // Description:
  // Output is specified by the process the output port is in,
  // and a tag so there can be more than one output port per process.
  // Tag must be set before this port can be used.
  // THIS TAG MUST BE EVEN BECAUSE TWO RMIs ARE CREATED FROM IT!!!
  void SetTag(int tag);
  vtkGetMacro(Tag, int);
  
  // Description:
  // This just forwards the wait onto the controller, which will wait
  // for a message for any of its ports (or any RMI).
  // Since this method is implemented in the controller, multiple
  // output ports can be waiting at once (as long as they share a controller).
  // This method will only return if the BreakFlag is turned on in the 
  // controller.  The controller automatically creates a break rmi with 
  // the vtkMultiProcessController::BREAK_RMI_TAG for doing this remotely.
  void WaitForUpdate() {this->Controller->ProcessRMIs();}
  
  // Description:
  // Access to the global controller.
  vtkMultiProcessController *GetController() {return this->Controller;}
  vtkSetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // RMI function needs to call this.  Should make function a friend.
  // No one else should call it.
  void TriggerUpdateInformation(int remoteProcessId);
  void TriggerUpdate(int remoteProcessId);
  
  // Description:
  // Trying to get pipeline parallelism working.
  vtkSetMacro(PipelineFlag, int);
  vtkGetMacro(PipelineFlag, int);
  vtkBooleanMacro(PipelineFlag, int);  
  
  // Description:
  // This method is called after the port updates.  It is meant to change
  // a parameter if a series is being processed (for pipeline parallelism).
  void SetParameterMethod(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetParameterMethodArgDelete(void (*f)(void *));
  
protected:
  vtkOutputPort();
  ~vtkOutputPort();  
  
  int Tag;
  
  vtkMultiProcessController *Controller;
  vtkTimeStamp UpdateTime;

  // Stuff for pipeline parallelism.
  int PipelineFlag;
  void (*ParameterMethod)(void *);
  void (*ParameterMethodArgDelete)(void *);
  void *ParameterMethodArg;
private:
  vtkOutputPort(const vtkOutputPort&);  // Not implemented.
  void operator=(const vtkOutputPort&);  // Not implemented.
};

#endif


