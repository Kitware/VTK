/*=========================================================================

  Program:   Visualization Toolkit
  Module:    RefCount.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkRefCount - subclasses of this object are reference counted
// .SECTION Description
// vtkRefCount is the base class for objects that are reference counted. 
// Objects that are reference counted exist as long as another object
// uses them. Once the last reference to a reference counted object is 
// removed, the object will spontaneously destruct. Typically only data
// objects that are passed between objects are reference counted.

#ifndef __vtkRefCount_hh
#define __vtkRefCount_hh

#include "Object.hh"

class vtkRefCount : public vtkObject
{
public:
  vtkRefCount();
  ~vtkRefCount();
  void PrintSelf(ostream& os, vtkIndent indent);
  char *GetClassName() {return "vtkRefCount";};

  void Register(vtkObject* o);
  void UnRegister(vtkObject* o);
  int  GetRefCount() {return this->RefCount;};

private:
  int RefCount;      // Number of uses of this object by other objects
};

#endif

