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
// .NAME vlStructuredDataToPolyFilter - abstract filter class
// .SECTION Description
// vlStructuredDataToPolyFilter are filters whose subclasses take as input
// structured data (e.g., structured points, structured grid) and generate
// polygonal data on output.

#ifndef __vlStructuredDataToPolyFilter_h
#define __vlStructuredDataToPolyFilter_h

#include "StrDataF.hh"
#include "PolyData.hh"

class vlStructuredDataToPolyFilter : public vlPolyData, public vlStructuredDataFilter
{
public:
  void Update();
  char *GetClassName() {return "vlStructuredDataToPolyFilter";};
  void PrintSelf(ostream& os, vlIndent indent);
};

#endif


