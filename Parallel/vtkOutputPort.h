/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutputPort.h
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

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkOutputPort : public vtkProcessObject
{
public:
  static vtkOutputPort *New();
  vtkTypeRevisionMacro(vtkOutputPort,vtkProcessObject);
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
  void WaitForUpdate();
  
  // Description:
  // Access to the global controller.
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController*);

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


