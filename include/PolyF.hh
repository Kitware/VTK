/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PolyF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPolyFilter - filter that takes vtkPolyData as input
// .SECTION Description
// vtkPolyFilter is a filter that takes a single vtkPolyData data object
// as input.

#ifndef __vtkPolyFilter_h
#define __vtkPolyFilter_h

#include "Filter.hh"
#include "PolyData.hh"

class vtkPolyFilter : public vtkFilter 
{
public:
  vtkPolyFilter() {};
  ~vtkPolyFilter();
  char *_GetClassName() {return "vtkPolyFilter";};
  void _PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkPolyData *input);
  void SetInput(vtkPolyData &input) {this->SetInput(&input);};
  vtkPolyData *GetInput() {return (vtkPolyData *)this->Input;};
                               
};

#endif


