/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DataSetF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDataSetFilter - filter that takes vtkDataSet as input
// .SECTION Description
// vtkDataSetFilter is a filter that takes a single vtkDataSet data object
// as input.

#ifndef __vtkDataSetFilter_h
#define __vtkDataSetFilter_h

#include "Filter.hh"
#include "DataSet.hh"

class vtkDataSetFilter : public vtkFilter 
{
public:
  vtkDataSetFilter() {};
  ~vtkDataSetFilter();
  char *_GetClassName() {return "vtkDataSetFilter";};
  void _PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkDataSet *input);
  void SetInput(vtkDataSet &input) {this->SetInput(&input);};
  vtkDataSet *GetInput() {return this->Input;};
};

#endif


