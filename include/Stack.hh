/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Stack.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStack - create and manipulate lists of objects
// .SECTION Description
// vtkStack is a general object for creating and manipulating lists
// of objects. vtkStack also serves as a base class for lists of
// specific types of objects.

#ifndef __vtkStack_hh
#define __vtkStack_hh

#include "Object.hh"

//BTX begin tcl exclude
class vtkStackElement //;prevents pick-up by man page generator
{
 public:
  vtkStackElement():Item(NULL),Next(NULL) {};
  vtkObject *Item;
  vtkStackElement *Next;
};
//ETX end tcl exclude

class vtkStack : public vtkObject
{
public:
  vtkStack();
  ~vtkStack();
  void PrintSelf(ostream& os, vtkIndent indent);
  char *GetClassName() {return "vtkStack";};

  void Push(vtkObject *);
  vtkObject *Pop();
  vtkObject *GetTop();
  int  GetNumberOfItems();

protected:
  int NumberOfItems;
  vtkStackElement *Top;
  vtkStackElement *Bottom;
};

#endif
