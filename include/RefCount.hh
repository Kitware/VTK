/*=========================================================================

  Program:   Visualization Library
  Module:    RefCount.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlRefCount - subclasses of this object are reference counted
// .SECTION Description
// vlRefCount is the base class for objects that are reference counted. 
// Objects that are reference counted exist as long as another object
// uses them. Once the last reference to a reference counted object is 
// removed, the object will spontaneously destruct. Typically only data
// objects that are passed between objects are reference counted.

#ifndef __vlRefCount_hh
#define __vlRefCount_hh

#include "Object.hh"

class vlRefCount : public vlObject
{
public:
  vlRefCount();
  ~vlRefCount();
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlRefCount";};

  void Register(vlObject* o);
  void UnRegister(vlObject* o);
  int  GetRefCount() {return this->RefCount;};

private:
  int RefCount;      // Number of uses of this object by other objects
};

#endif

