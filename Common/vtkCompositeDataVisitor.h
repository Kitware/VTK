/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataVisitor.h
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
// .NAME vtkCompositeDataVisitor - abstract superclass for composite data visitors
// .SECTION Description
// vtkCompositeDataVisitor is a superclass for composite data visitors.
// Composite data visitors iterate through dataset collections and
// apply an operation to each item.

#ifndef __vtkCompositeDataVisitor_h
#define __vtkCompositeDataVisitor_h

#include "vtkObject.h"

class vtkCompositeDataCommand;

class VTK_COMMON_EXPORT vtkCompositeDataVisitor : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkCompositeDataVisitor,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Set/get the command object applied to each item. The visitor
  // will call Execute() on the command with the current item
  // as an argument.
  void SetCommand(vtkCompositeDataCommand* comm);
  vtkGetObjectMacro(Command, vtkCompositeDataCommand);
//ETX

  // Description:
  // Apply the command on each object in the collection.
  virtual void Execute() = 0;

  // Description:
  // When CreateTransitionElements is enabled, the visitor will
  // try to create extra datasets to remove boundary artifacts.
  // This is used by sub-classes that have such capability.
  vtkSetMacro(CreateTransitionElements, int);
  vtkGetMacro(CreateTransitionElements, int);
  vtkBooleanMacro(CreateTransitionElements, int);

protected:
  vtkCompositeDataVisitor(); 
  virtual ~vtkCompositeDataVisitor(); 

  vtkCompositeDataCommand* Command;
  int CreateTransitionElements;

private:
  vtkCompositeDataVisitor(const vtkCompositeDataVisitor&);  // Not implemented.
  void operator=(const vtkCompositeDataVisitor&);  // Not implemented.
};

#endif

