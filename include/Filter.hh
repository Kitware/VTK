/*=========================================================================

  Program:   Visualization Library
  Module:    Filter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Abstract class for specifying filter behaviour
//
#ifndef __vlFilter_h
#define __vlFilter_h

#include "Object.hh"

class vlFilter : virtual public vlObject 
{
public:
  vlFilter();
  ~vlFilter() {};
  char *GetClassName() {return "vlFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Abstract Interface:
  // All filters must provide a method to update the visualization 
  // pipeline.
  virtual void Update() = 0;

  void SetStartMethod(void (*f)(void *), void *arg);
  void SetEndMethod(void (*f)(void *), void *arg);

protected:
  virtual void Execute() 
    {vlErrorMacro(<< "Execute is a Filter subclass responsibility");};
  char Updating;
  void (*StartMethod)(void *);
  void *StartMethodArg;
  void (*EndMethod)(void *);
  void *EndMethodArg;
  vlTimeStamp ExecuteTime;

};

#endif


