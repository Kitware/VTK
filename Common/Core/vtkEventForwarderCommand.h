/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEventForwarderCommand.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkEventForwarderCommand - a simple event forwarder command
// .SECTION Description
// Use vtkEventForwarderCommand to forward an event to a new object.
// This command will intercept the event, and use InvokeEvent
// on a 'target' as if that object was the one that invoked the event instead
// of the object this commmand was attached to using AddObserver.
//
// .SECTION See Also
// vtkCommand

#ifndef __vtkEventForwarderCommand_h
#define __vtkEventForwarderCommand_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkCommand.h"

class VTKCOMMONCORE_EXPORT vtkEventForwarderCommand : public vtkCommand
{
public:
  vtkTypeMacro(vtkEventForwarderCommand,vtkCommand);

  static vtkEventForwarderCommand *New()
    {return new vtkEventForwarderCommand;};

  // Description:
  // Satisfy the superclass API for callbacks. Recall that the caller is
  // the instance invoking the event; eid is the event id (see
  // vtkCommand.h); and calldata is information sent when the callback
  // was invoked (e.g., progress value in the vtkCommand::ProgressEvent).
  void Execute(vtkObject *caller, unsigned long eid, void *callData);

  // Description:
  // Methods to set and get client and callback information, and the callback
  // function.
  virtual void SetTarget(vtkObject *obj)
    { this->Target = obj; }
  virtual void* GetTarget()
    { return this->Target; }

protected:

  vtkObject *Target;

  vtkEventForwarderCommand();
  ~vtkEventForwarderCommand() {}
};

#endif /* __vtkEventForwarderCommand_h */

// VTK-HeaderTest-Exclude: vtkEventForwarderCommand.h
