/*=========================================================================

  Program:   Visualization Library
  Module:    PtSetF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPointSetFilter - filter that takes vlPointSet as input
// .SECTION Description
// vlPointSetFilter is a filter that takes a single vlPointSet data object
// as input.

#ifndef __vlPointSetFilter_h
#define __vlPointSetFilter_h

#include "Filter.hh"
#include "PointSet.hh"

class vlPointSetFilter : public vlFilter 
{
public:
  vlPointSetFilter() {};
  ~vlPointSetFilter();
  void _PrintSelf(ostream& os, vlIndent indent);

  void SetInput(vlPointSet *input);
  void SetInput(vlPointSet &input) {this->SetInput(&input);};
  vlPointSet *GetInput() {return (vlPointSet *)this->Input;};
};

#endif


