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
// .NAME vlFilter - abstract class for specifying filter behaviour
// .SECTION Description
// vlFilter is an abstract class that specifies the interface for data 
// filters. Each filter must have an UpdateFilter() and Execute() method 
// that will cause the filter to execute if its input or the filter itself 
// has been modified since the last execution time.

#ifndef __vlFilter_h
#define __vlFilter_h

#include "LWObject.hh"
#include "DataSet.hh"

class vlFilter : public vlLWObject
{
public:
  vlFilter();
  virtual ~vlFilter() {};
  void _PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // All filters must provide a method to update the visualization 
  // pipeline.
  virtual void UpdateFilter();

  void SetStartMethod(void (*f)(void *), void *arg);
  void SetEndMethod(void (*f)(void *), void *arg);

protected:
  vlDataSet *Input;

  virtual void Execute();
  char Updating;
  void (*StartMethod)(void *);
  void *StartMethodArg;
  void (*EndMethod)(void *);
  void *EndMethodArg;
  vlTimeStamp ExecuteTime;

};

#endif


