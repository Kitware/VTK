/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataVisitorCommand.h
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
// .NAME vtkCompositeDataVisitorCommand - abstract superclass for commands
// .SECTION Description
// vtkCompositeDataVisitorCommand and it's subclasses are used by
// the visitor classes. Visitors apply the command on each item
// of a collection by calling Execute and passing the appropriate
// arguments
// .SECTION See Also
// vtkCompositeDataVisitor

#ifndef __vtkCompositeDataVisitorCommand_h
#define __vtkCompositeDataVisitorCommand_h

#include "vtkObject.h"

class vtkCompositeDataVisitor;
class vtkDataObject;

class VTK_COMMON_EXPORT vtkCompositeDataVisitorCommand : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkCompositeDataVisitorCommand, vtkObject);
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
  vtkCompositeDataVisitorCommand(); 
  virtual ~vtkCompositeDataVisitorCommand(); 

private:
  vtkCompositeDataVisitorCommand(const vtkCompositeDataVisitorCommand&);  // Not implemented.
  void operator=(const vtkCompositeDataVisitorCommand&);  // Not implemented.
};

#endif /* __vtkCompositeDataVisitorCommand_h */
 
