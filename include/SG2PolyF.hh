/*=========================================================================

  Program:   Visualization Library
  Module:    SG2PolyF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// .NAME vlStructuredDataSetToPolyFilter - abstract filter class
// .SECTION Description
// vlStructuredDataSetToPolyFilter are filters whose subclasses take as input
// structured data (e.g., structured points, structured grid) and generate
// polygonal data on output.

#ifndef __vlStructuredDataSetToPolyFilter_h
#define __vlStructuredDataSetToPolyFilter_h

#include "StrDataF.hh"
#include "PolyData.hh"

class vlStructuredDataSetToPolyFilter : public vlPolyData, public vlStructuredDataSetFilter
{
public:
  void Update();
  char *GetClassName() {return "vlStructuredDataSetToPolyFilter";};
  void PrintSelf(ostream& os, vlIndent indent);
};

#endif


