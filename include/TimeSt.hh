/*=========================================================================


  Program:   Visualization Library
  Module:    TimeSt.hh
  Language:  C++

Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization
Library. No part of this file or its
contents may be copied, reproduced or
altered in any way without the express
written consent of the authors.
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994
=========================================================================*/
//
// Classes that need to keep track of modification / execution time use
// this class.
//
#ifndef __vlTimeStamp_h
#define __vlTimeStamp_h

class vlTimeStamp 
{
public:
  vlTimeStamp() {this->ModifiedTime = 0;};
  void Modified() {this->ModifiedTime = ++vlTime;};
  unsigned long int GetMtime() {return ModifiedTime;};
  // Using >= and <= in the operators below handles special cases when
  // modified times are equal. Only occurs for recently instantiated objects.
  int operator>(vlTimeStamp& ts) 
    {return (this->ModifiedTime > ts.ModifiedTime);};
  int operator<(vlTimeStamp& ts) 
    {return (this->ModifiedTime < ts.ModifiedTime);};
  operator unsigned long int() {return this->ModifiedTime;};
private:
  unsigned long ModifiedTime;
  static unsigned long vlTime;
};

#endif
