/*=========================================================================

  Program:   Visualization Library
  Module:    UGridF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlUnstructuredGridFilter - filter that takes vlPolyData as input
// .SECTION Description
// vlUnstructuredGridFilter is a filter that takes a single vlPolyData data object
// as input.

#ifndef __vlUnstructuredGridFilter_hh
#define __vlUnstructuredGridFilter_hh

#include "Filter.hh"
#include "UGrid.hh"

class vlUnstructuredGridFilter : public vlFilter 
{
public:
  vlUnstructuredGridFilter() {};
  ~vlUnstructuredGridFilter();
  char *_GetClassName() {return "vlUnstructuredGridFilter";};
  void _PrintSelf(ostream& os, vlIndent indent);

  void SetInput(vlUnstructuredGrid *input);
  void SetInput(vlUnstructuredGrid &input) {this->SetInput(&input);};
  vlUnstructuredGrid *GetInput() {return (vlUnstructuredGrid *)this->Input;};
                               
};

#endif


