/*=========================================================================

  Program:   Visualization Library
  Module:    SGOutlF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredGridOutlineFilter - create wireframe outline for structured grid
// .SECTION Description
// vlStructuredGridOutlineFilter is a filter that generates a wireframe 
// outline of a structured grid (vlStructuredGrid). Structured data is 
// topologically a cube, so the outline will have 12 "edges".

#ifndef __vlStructuredGridOutlineFilter_h
#define __vlStructuredGridOutlineFilter_h

#include "SG2PolyF.hh"

class vlStructuredGridOutlineFilter : public vlStructuredGridToPolyFilter
{
public:
  vlStructuredGridOutlineFilter() {};
  ~vlStructuredGridOutlineFilter() {};
  char *GetClassName() {return "vlStructuredGridOutlineFilter";};

protected:
  void Execute();
};

#endif


