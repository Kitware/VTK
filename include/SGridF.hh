/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SGridF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredGridFilter - filter that takes vtkStructuredGrid as input
// .SECTION Description
// vtkStructuredGridFilter is a filter that takes a single 
// vtkStructuredGrid Grid object as input.

#ifndef __vtkStructuredGridFilter_h
#define __vtkStructuredGridFilter_h

#include "Filter.hh"
#include "SGrid.hh"

class vtkStructuredGridFilter : public vtkFilter 
{
public:
  vtkStructuredGridFilter();
  ~vtkStructuredGridFilter();
  char *_GetClassName() {return "vtkStructuredGridFilter";};
  void _PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkStructuredGrid *input);
  void SetInput(vtkStructuredGrid &input) {this->SetInput(&input);};
  vtkStructuredGrid *GetInput() {return (vtkStructuredGrid *)this->Input;};

};

#endif
