/*=========================================================================

  Program:   Visualization Library
  Module:    SGGeomF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredGridGeometryFilter - extract geometry for structured grid
// .SECTION Description
// vlStructuredGridGeometryFilter is a filter that extracts geometry from a
// structured grid. By specifying appropriate i-j-k indices, it is possible
// to extract a point, a curve, a surface, or a "volume". Depending upon the
// type of data, the curve and surface may be curved or planar. The volume
// is actually a (n x m x o) region of points.

#ifndef __vlStructuredGridGeometryFilter_h
#define __vlStructuredGridGeometryFilter_h

#include "SG2PolyF.hh"

class vlStructuredGridGeometryFilter : public vlStructuredGridToPolyFilter
{
public:
  vlStructuredGridGeometryFilter();
  ~vlStructuredGridGeometryFilter() {};
  char *GetClassName() {return "vlStructuredGridGeometryFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);
  void SetExtent(int *extent);
  int *GetExtent() { return this->Extent;};

protected:
  void Execute();
  int Extent[6];
};

#endif


