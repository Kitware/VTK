/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkOutputPort.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkOutputPort - Sends data from this process to another process.
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


class VTK_EXPORT vtkOutputPort : public vtkProcessObject
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
  // For now, this method does not return.  I need to find an elegant
  // way to break this loop. (maybe a message between controllers)
  void WaitForUpdate() {this->Controller->ProcessRMIs();}
  
  // Description:
  // Access to the global controller.
  vtkMultiProcessController *GetController() {return this->Controller;}

  // Description:
  // RMI function needs to call this.  Should make function a friend.
  // No one else should call it.
  void TriggerUpdateInformation(int remoteProcessId);
  void TriggerUpdate(int remoteProcessId);
  
  // Description:
  // Trying to get pipeline parallism.
  vtkSetMacro(PipelineFlag, int);
  vtkGetMacro(PipelineFlag, int);
  vtkBooleanMacro(PipelineFlag, int);  
  
  // Description:
  // This method is called after the port updates.  It is meant to change
  // a parameter if a series is being processes.
  void SetParameterMethod(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetParameterMethodArgDelete(void (*f)(void *));
  
protected:
  vtkOutputPort();
  ~vtkOutputPort();  
  vtkOutputPort(const vtkOutputPort&) {};
  void operator=(const vtkOutputPort&) {};
  
  int Tag;
  
  vtkMultiProcessController *Controller;
  vtkTimeStamp UpdateTime;

  // Stuff for pipeline parallelism.
  int PipelineFlag;
  void (*ParameterMethod)(void *);
  void (*ParameterMethodArgDelete)(void *);
  void *ParameterMethodArg;
};

#endif


