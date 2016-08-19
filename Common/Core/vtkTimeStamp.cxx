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
#include "vtkWindows.h"

// We use the Schwarz Counter idiom to make sure that GlobalTimeStamp
// is initialized before any other class uses it.

#include "vtkAtomicTypes.h"

//-------------------------------------------------------------------------
vtkTimeStamp* vtkTimeStamp::New()
{
  // If the factory was unable to create the object, then create it here.
  return new vtkTimeStamp;
}

//-------------------------------------------------------------------------
void vtkTimeStamp::Modified()
{
#if VTK_SIZEOF_VOID_P == 8
  static vtkAtomicUInt64 GlobalTimeStamp(0);
#else
  static vtkAtomicUInt32 GlobalTimeStamp(0);
#endif
  this->ModifiedTime = (vtkMTimeType)++GlobalTimeStamp;
}
