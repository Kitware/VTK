/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PtSetF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPointSetFilter - filter that takes vtkPointSet as input
// .SECTION Description
// vtkPointSetFilter is a filter that takes a single vtkPointSet data object
// as input.

#ifndef __vtkPointSetFilter_h
#define __vtkPointSetFilter_h

#include "Filter.hh"
#include "PointSet.hh"

class vtkPointSetFilter : public vtkFilter 
{
public:
  vtkPointSetFilter() {};
  ~vtkPointSetFilter();
  char *_GetClassName() {return "vtkPointSetFilter";};
  void _PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkPointSet *input);
  void SetInput(vtkPointSet &input) {this->SetInput(&input);};
  vtkPointSet *GetInput() {return (vtkPointSet *)this->Input;};
};

#endif


