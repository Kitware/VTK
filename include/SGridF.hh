/*=========================================================================

  Program:   Visualization Library
  Module:    SGridF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredGridFilter - filter that takes vlStructuredGrid as input
// .SECTION Description
// vlStructuredGridFilter is a filter that takes a single 
// vlStructuredGrid Grid object as input.

#ifndef __vlStructuredGridFilter_h
#define __vlStructuredGridFilter_h

#include "Filter.hh"
#include "SGrid.hh"

class vlStructuredGridFilter : public vlFilter 
{
public:
  vlStructuredGridFilter();
  ~vlStructuredGridFilter();
  void _PrintSelf(ostream& os, vlIndent indent);

  void SetInput(vlStructuredGrid *input);
  void SetInput(vlStructuredGrid &input) {this->SetInput(&input);};
  vlStructuredGrid *GetInput() {return (vlStructuredGrid *)this->Input;};

};

#endif
