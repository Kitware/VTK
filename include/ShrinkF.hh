/*=========================================================================

  Program:   Visualization Library
  Module:    ShrinkF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Shrinks arbitrary input
//
#ifndef __vlShrinkFilter_h
#define __vlShrinkFilter_h

#include "DS2UGrid.hh"

class vlShrinkFilter : public vlDataSetToUnstructuredGridFilter
{
public:
  vlShrinkFilter(const float sf=0.5) {this->ShrinkFactor = sf;};
  ~vlShrinkFilter() {};
  char *GetClassName() {return "vlShrinkFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetMacro(ShrinkFactor,float);
  vlGetMacro(ShrinkFactor,float);

protected:
  void Execute();
  float ShrinkFactor;
};

#endif


