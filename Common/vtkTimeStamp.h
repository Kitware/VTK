/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimeStamp.h
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
// .NAME vtkTimeStamp - record modification and/or execution time
// .SECTION Description
// vtkTimeStamp records a unique time when the method Modified() is 
// executed. This time is guaranteed to be monotonically increasing.
// Classes use this object to record modified and/or execution time.
// There is built in support for the binary < and > comparison
// operators between two vtkTimeStamp objects.

#ifndef __vtkTimeStamp_h
#define __vtkTimeStamp_h

#include "vtkWin32Header.h"

class VTK_COMMON_EXPORT vtkTimeStamp 
{
public:
  vtkTimeStamp() {this->ModifiedTime = 0;};
  static vtkTimeStamp *New();
  void Delete() {delete this;};

  virtual const char *GetClassName() {return "vtkTimeStamp";};

  // Description:
  // Set this objects time to the current time. The current time is
  // just a monotonically increasing unsigned long integer. It is
  // possible for this number to wrap around back to zero.
  // This should only happen for processes that have been running
  // for a very long time, while constantly changing objects
  // within the program. When this does occur, the typical consequence
  // should be that some filters will update themselves when really
  // they don't need to.
  void Modified();

  // Description:
  // Return this object's Modified time.
  unsigned long int GetMTime() {return this->ModifiedTime;};

  // Description:
  // Support comparisons of time stamp objects directly.
  int operator>(vtkTimeStamp& ts) {
    return (this->ModifiedTime > ts.ModifiedTime);};
  int operator<(vtkTimeStamp& ts) {
    return (this->ModifiedTime < ts.ModifiedTime);};

  // Description:
  // Allow for typecasting to unsigned long.
  operator unsigned long() {return this->ModifiedTime;};

private:
  unsigned long ModifiedTime;
};

#endif
