/*=========================================================================

  Program:   Visualization Library
  Module:    DS2SPtsF.hh
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
// PolyToStructuredPointsFilter are filters that take PolyData in and 
// generate StructuredPoints data
//
#ifndef __vlPolyToStructuredPointsFilter_h
#define __vlPolyToStructuredPointsFilter_h

#include "PolyF.hh"
#include "SPoints.hh"

class vlPolyToStructuredPointsFilter : public vlStructuredPoints, public vlPolyFilter
{
public:
  void Update();
  char *GetClassName() {return "vlPolyToStructuredPointsFilter";};
  void PrintSelf(ostream& os, vlIndent indent);
};

#endif


