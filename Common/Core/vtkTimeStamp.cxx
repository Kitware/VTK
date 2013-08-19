/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimeStamp.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTimeStamp.h"

#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkTimeStamp* vtkTimeStamp::New()
{
  // If the factory was unable to create the object, then create it here.
  return new vtkTimeStamp;
}

//-------------------------------------------------------------------------
void vtkTimeStamp::Modified()
{
  // Note that this would not normally be thread safe. However,
  // VTK initializes static objects at load time, which in turn call
  // this functions, which make this thread safe. If this behavior
  // changes, this should also be fixed by converting it to a static
  // class member which is initialized at load time.
  static vtkAtomicInt64 GlobalTimeStamp(0);

  this->ModifiedTime =
    (unsigned long)GlobalTimeStamp.Increment();
}
