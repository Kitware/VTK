/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataCommand.h
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
// .NAME vtkCompositeDataCommand - abstract superclass for commands
// .SECTION Description
// vtkCompositeDataCommand and it's subclasses are used by
// the visitor classes. Visitors apply the command on each item
// of a collection by calling Execute and passing the appropriate
// arguments
// .SECTION See Also
// vtkCompositeDataVisitor

#ifndef __vtkCompositeDataCommand_h
#define __vtkCompositeDataCommand_h

#include "vtkObject.h"

class vtkCompositeDataVisitor;
class vtkDataObject;

class VTK_COMMON_EXPORT vtkCompositeDataCommand : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkCompositeDataCommand, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called by the visitor. The caller should pass itself, the
  // current dataset. The last argument can be used to pass
  // additional information.
  virtual void Execute(vtkCompositeDataVisitor *caller, 
                       vtkDataObject *input,
                       void* callData) = 0;

  // Description:
  // Initialize should be called before iteration starts. It allows
  // the command to initialize things like outputs.
  virtual void Initialize() {};

protected:
  vtkCompositeDataCommand(); 
  virtual ~vtkCompositeDataCommand(); 

private:
  vtkCompositeDataCommand(const vtkCompositeDataCommand&);  // Not implemented.
  void operator=(const vtkCompositeDataCommand&);  // Not implemented.
};

#endif /* __vtkCompositeDataCommand_h */
 
