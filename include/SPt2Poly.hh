/*=========================================================================

  Program:   Visualization Library
  Module:    SPt2Poly.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredPointsToPolyDataFilter - abstract filter class
// .SECTION Description
// vlStructuredPointsToPolyDataFilter is an abstract filter class whose
// subclasses take on input structured points and generate polygonal 
// data on output.

#ifndef __vlStructuredPointsToPolyDataFilter_h
#define __vlStructuredPointsToPolyDataFilter_h

#include "StrPtsF.hh"
#include "PolyData.hh"

class vlStructuredPointsToPolyDataFilter : public vlPolyData, 
                                              public vlStructuredPointsFilter
{
public:
  char *GetClassName() {return "vlDataSetToPolyDataFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Modified();
  unsigned long int GetMTime();
  void Update();
};

#endif


