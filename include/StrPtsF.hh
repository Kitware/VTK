/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StrPtsF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredPointsFilter - filter that takes vtkStructuredPoints as input
// .SECTION Description
// vtkStructuredPointsFilter is a filter that takes a single vtkStructuredPoints 
// data object as input.

#ifndef __vtkStructuredPointsFilter_hh
#define __vtkStructuredPointsFilter_hh

#include "Filter.hh"
#include "StrPts.hh"

class vtkStructuredPointsFilter : public vtkFilter 
{
public:
  vtkStructuredPointsFilter() {};
  ~vtkStructuredPointsFilter();
  char *_GetClassName() {return "vtkStructuredPointsFilter";};
  void _PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkStructuredPoints *input);
  void SetInput(vtkStructuredPoints &input) {this->SetInput(&input);};
  vtkStructuredPoints *GetInput() {return (vtkStructuredPoints *)this->Input;};

};

#endif


