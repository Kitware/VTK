/*=========================================================================

  Program:   Visualization Library
  Module:    PolyF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPolyFilter - filter that takes vlPolyData as input
// .SECTION Description
// vlPolyFilter is a filter that takes a single vlPolyData data object
// as input.

#ifndef __vlPolyFilter_h
#define __vlPolyFilter_h

#include "Filter.hh"
#include "PolyData.hh"

class vlPolyFilter : public vlFilter 
{
public:
  vlPolyFilter() {};
  ~vlPolyFilter();
  void _PrintSelf(ostream& os, vlIndent indent);

  void SetInput(vlPolyData *input);
  void SetInput(vlPolyData &input) {this->SetInput(&input);};
  vlPolyData *GetInput() {return (vlPolyData *)this->Input;};
                               
};

#endif


