/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SPt2Poly.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredPointsToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkStructuredPointsToPolyDataFilter is an abstract filter class whose
// subclasses take on input structured points and generate polygonal 
// data on output.

#ifndef __vtkStructuredPointsToPolyDataFilter_h
#define __vtkStructuredPointsToPolyDataFilter_h

#include "StrPtsF.hh"
#include "PolyData.hh"

class vtkStructuredPointsToPolyDataFilter : public vtkPolyData, 
                                              public vtkStructuredPointsFilter
{
public:
  char *GetClassName() {return "vtkDataSetToPolyDataFilter";};
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


