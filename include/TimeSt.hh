/*=========================================================================

  Program:   Visualization Library
  Module:    TimeSt.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTimeStamp - record modification and/or execution time
// .SECTION Description
// vlTimeStamp records a unique time when the method Modified() is 
// executed. This time is guaranteed to be montonically increasing.
// Subclasses use this object to record modified and/or execution time.

#ifndef __vlTimeStamp_h
#define __vlTimeStamp_h

class vlTimeStamp 
{
public:
  vlTimeStamp() : ModifiedTime(0) {};
  void Modified() {this->ModifiedTime = ++vlTime;};
  unsigned long int GetMTime() {return ModifiedTime;};

  int operator>(vlTimeStamp& ts) {return (this->ModifiedTime > ts.ModifiedTime);};
  int operator<(vlTimeStamp& ts) {return (this->ModifiedTime < ts.ModifiedTime);};
  operator unsigned long int() {return this->ModifiedTime;};

private:
  unsigned long ModifiedTime;
  static unsigned long vlTime;
};

#endif
