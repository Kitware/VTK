/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimeStamp.cxx
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
//
// Initialize static member
//
#include "vtkTimeStamp.h"
#include "vtkCriticalSection.h"
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
#if defined(WIN32) || defined(_WIN32)
  static LONG vtkTimeStampTime = 0;

  this->ModifiedTime = (unsigned long)InterlockedIncrement(&vtkTimeStampTime);
#else
  static unsigned long vtkTimeStampTime = 0;
  static vtkSimpleCriticalSection TimeStampCritSec;
  
  TimeStampCritSec.Lock();
  this->ModifiedTime = ++vtkTimeStampTime;
  TimeStampCritSec.Unlock();
#endif
}









