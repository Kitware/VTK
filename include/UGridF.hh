/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UGridF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkUnstructuredGridFilter - filter that takes vtkPolyData as input
// .SECTION Description
// vtkUnstructuredGridFilter is a filter that takes a single vtkPolyData data object
// as input.

#ifndef __vtkUnstructuredGridFilter_hh
#define __vtkUnstructuredGridFilter_hh

#include "Filter.hh"
#include "UGrid.hh"

class vtkUnstructuredGridFilter : public vtkFilter 
{
public:
  vtkUnstructuredGridFilter() {};
  ~vtkUnstructuredGridFilter();
  char *_GetClassName() {return "vtkUnstructuredGridFilter";};
  void _PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkUnstructuredGrid *input);
  void SetInput(vtkUnstructuredGrid &input) {this->SetInput(&input);};
  vtkUnstructuredGrid *GetInput() {return (vtkUnstructuredGrid *)this->Input;};
                               
};

#endif


