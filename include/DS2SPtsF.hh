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
// DataSetToStructuredPointsFilter are filters that take DataSets in and 
// generate StructuredPoints data
//
#ifndef __vlDataSetToStructuredPointsFilter_h
#define __vlDataSetToStructuredPointsFilter_h

#include "DataSetF.hh"
#include "StrPts.hh"

class vlDataSetToStructuredPointsFilter : public vlStructuredPoints, public vlDataSetFilter
{
public:
  void Update();
  char *GetClassName() {return "vlDataSetToStructuredPointsFilter";};
  void PrintSelf(ostream& os, vlIndent indent);
};

#endif


