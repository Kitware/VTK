/*=========================================================================

  Program:   Visualization Library
  Module:    SPtGeomF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredPointsGeometryFilter - extract geometry for structured points
// .SECTION Description
// vlStructuredPointsGeometryFilter is a filter that extracts geometry from a
// structured points dataset. By specifying appropriate i-j-k indices (via the 
// "Extent instance variable, it is possible to extract a point, a line, a 
// plane (i.e., image), or a "volume" from dataset. (Since the output is 
// of type polydata, the volume is actually a (n x m x o) region of points).
//    The extent specification is zero-offset. That is, the first k-plane in
// a 50x50x50 volume is given by (0,49, 0,49, 0,0).
// .SECTION Caveats
// If you don't know the dimensions of the input dataset, you can use a large
// number to specify extent (the number will be clamped appropriately). For 
// example, if the dataset dimensions are 50x50x50, and you want a the fifth 
// k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will 
// automatically be clamped to 49.
// .SECTION See Also
// vlGeometryFilter, vlStructuredGridFilter

#ifndef __vlStructuredPointsGeometryFilter_h
#define __vlStructuredPointsGeometryFilter_h

#include "SPt2Poly.hh"

class vlStructuredPointsGeometryFilter : public vlStructuredPointsToPolyDataFilter
{
public:
  vlStructuredPointsGeometryFilter();
  ~vlStructuredPointsGeometryFilter() {};
  char *GetClassName() {return "vlStructuredPointsGeometryFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);
  void SetExtent(int *extent);
  int *GetExtent() { return this->Extent;};

protected:
  void Execute();
  int Extent[6];
};

#endif


