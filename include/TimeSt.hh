/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TimeSt.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTimeStamp - record modification and/or execution time
// .SECTION Description
// vtkTimeStamp records a unique time when the method Modified() is 
// executed. This time is guaranteed to be montonically increasing.
// Subclasses use this object to record modified and/or execution time.

#ifndef __vtkTimeStamp_h
#define __vtkTimeStamp_h

class vtkTimeStamp 
{
public:
  vtkTimeStamp() : ModifiedTime(0) {};
  void Modified() {this->ModifiedTime = ++vtkTime;};
  unsigned long int GetMTime() {return ModifiedTime;};

  int operator>(vtkTimeStamp& ts) {return (this->ModifiedTime > ts.ModifiedTime);};
  int operator<(vtkTimeStamp& ts) {return (this->ModifiedTime < ts.ModifiedTime);};
  operator unsigned long int() {return this->ModifiedTime;};

private:
  unsigned long ModifiedTime;
  static unsigned long vtkTime;
};

#endif
