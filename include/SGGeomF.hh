/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SGGeomF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredGridGeometryFilter - extract geometry for structured grid
// .SECTION Description
// vtkStructuredGridGeometryFilter is a filter that extracts geometry from a
// structured grid. By specifying appropriate i-j-k indices, it is possible
// to extract a point, a curve, a surface, or a "volume". Depending upon the
// type of data, the curve and surface may be curved or planar. The volume
// is actually a (n x m x o) region of points.
//    The extent specification is zero-offset. That is, the first k-plane in
// a 50x50x50 structured grid is given by (0,49, 0,49, 0,0).
// .SECTION Caveats
// If you don't know the dimensions of the input dataset, you can use a large
// number to specify extent (the number will be clamped appropriately). For 
// example, if the dataset dimensions are 50x50x50, and you want a the fifth 
// k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will 
// automatically be clamped to 49.
// .SECTION See Also
// vtkGeometryFilter, vtkStructuredPointsFilter

#ifndef __vtkStructuredGridGeometryFilter_h
#define __vtkStructuredGridGeometryFilter_h

#include "SG2PolyF.hh"

class vtkStructuredGridGeometryFilter : public vtkStructuredGridToPolyFilter
{
public:
  vtkStructuredGridGeometryFilter();
  ~vtkStructuredGridGeometryFilter() {};
  char *GetClassName() {return "vtkStructuredGridGeometryFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);
  void SetExtent(int *extent);
  int *GetExtent() { return this->Extent;};

protected:
  void Execute();
  int Extent[6];
};

#endif


