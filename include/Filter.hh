/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Filter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkFilter - abstract class for specifying filter behaviour
// .SECTION Description
// vtkFilter is an abstract class that specifies the interface for data 
// filters. Each filter must have an UpdateFilter() and Execute() method 
// that will cause the filter to execute if its input or the filter itself 
// has been modified since the last execution time.

#ifndef __vtkFilter_h
#define __vtkFilter_h

#include "LWObject.hh"
#include "DataSet.hh"

class vtkFilter : public vtkLWObject
{
public:
  vtkFilter();
  virtual ~vtkFilter() {};
  void _PrintSelf(ostream& os, vtkIndent indent);
  char *_GetClassName() {return "vtkFilter";};

  // Description:
  // All filters must provide a method to update the visualization 
  // pipeline.
  virtual void UpdateFilter();

  void SetStartMethod(void (*f)(void *), void *arg);
  void SetEndMethod(void (*f)(void *), void *arg);
  void SetStartMethodArgDelete(void (*f)(void *));
  void SetEndMethodArgDelete(void (*f)(void *));

protected:
  vtkDataSet *Input;
  char Updating;
  void (*StartMethod)(void *);
  void (*StartMethodArgDelete)(void *);
  void *StartMethodArg;
  void (*EndMethod)(void *);
  void (*EndMethodArgDelete)(void *);
  void *EndMethodArg;
  vtkTimeStamp ExecuteTime;

  // Every filter must have execute method.
  virtual void Execute();

  // Get flag indicating whether data has been released since last execution.
  // Used during update method to determin whether to execute or not.
  virtual int GetDataReleased();
  virtual void SetDataReleased(int flag);

};

#endif


