/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Source.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkSource - abstract class specifies interface of data sources
// .SECTION Description
// vtkSource is an abstract object that specifies behavior and interface
// of source objects. Source objects are objects that begin visualization
// pipeline. Sources include readers (read data from file or communications
// port) and procedural sources (generate data programatically).

#ifndef __vtkSource_h
#define __vtkSource_h

#include "LWObject.hh"

class vtkSource : public vtkLWObject
{
public:
  vtkSource();
  virtual ~vtkSource() {};
  char *_GetClassName() {return "vtkSource";};
  void _PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Bring object up-to-date before execution. Update() checks modified
  // time against last execution time, and re-executes object if necessary.
  virtual void UpdateFilter();

  void SetStartMethod(void (*f)(void *), void *arg);
  void SetEndMethod(void (*f)(void *), void *arg);
  void SetStartMethodArgDelete(void (*f)(void *));
  void SetEndMethodArgDelete(void (*f)(void *));

protected:
  virtual void Execute();
  void (*StartMethod)(void *);
  void (*StartMethodArgDelete)(void *);
  void *StartMethodArg;
  void (*EndMethod)(void *);
  void (*EndMethodArgDelete)(void *);
  void *EndMethodArg;
  vtkTimeStamp ExecuteTime;

  // Get flag indicating whether data has been released since last execution.
  // Used during update method to determin whether to execute or not.
  virtual int GetDataReleased();
  virtual void SetDataReleased(int flag);

};

#endif


