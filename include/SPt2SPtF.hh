/*=========================================================================

  Program:   Visualization Library
  Module:    SPt2SPtF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// StructuredPointsToStructuredPointsFilter are filters that take 
// StructuredPoints as input and generate StructuredPoints data as output
//
#ifndef __vlStructuredPointsToStructuredPointsFilter_h
#define __vlStructuredPointsToStructuredPointsFilter_h

#include "StrPtsF.hh"
#include "StrPts.hh"

class vlStructuredPointsToStructuredPointsFilter : public vlStructuredPoints, 
                                              public vlStructuredPointsFilter
{
public:
  void Update();
  char *GetClassName() {return "vlDataSetToStructuredPointsFilter";};
  void PrintSelf(ostream& os, vlIndent indent);
};

#endif


