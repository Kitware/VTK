/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SPt2SPtF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredPointsToStructuredPointsFilter - abstract filter class
// .SECTION Description
// vtkStructuredPointsToStructuredPointsFilter is an abstract filter class 
// whose subclasses take on input structured points and generate
// structured points on output.

#ifndef __vtkStructuredPointsToStructuredPointsFilter_h
#define __vtkStructuredPointsToStructuredPointsFilter_h

#include "StrPtsF.hh"
#include "StrPts.hh"

class vtkStructuredPointsToStructuredPointsFilter : public vtkStructuredPoints, 
                                              public vtkStructuredPointsFilter
{
public:
  char *GetClassName() {return "vtkStructuredPointsToStructuredPointsFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Object interface
  void Modified();
  unsigned long int GetMTime();
  unsigned long int _GetMTime() {return this->GetMTime();};
  void DebugOn();
  void DebugOff();

  //DataSet interface
  void Update();

protected:
  //Filter interface
  int GetDataReleased();
  void SetDataReleased(int flag);

};

#endif


