/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataGeometryFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageDataGeometryFilter - extract geometry for structured points
// .SECTION Description
// vtkImageDataGeometryFilter is a filter that extracts geometry from a
// structured points dataset. By specifying appropriate i-j-k indices (via the 
// "Extent" instance variable), it is possible to extract a point, a line, a 
// plane (i.e., image), or a "volume" from dataset. (Since the output is 
// of type polydata, the volume is actually a (n x m x o) region of points.)
//
// The extent specification is zero-offset. That is, the first k-plane in
// a 50x50x50 volume is given by (0,49, 0,49, 0,0).
// .SECTION Caveats
// If you don't know the dimensions of the input dataset, you can use a large
// number to specify extent (the number will be clamped appropriately). For 
// example, if the dataset dimensions are 50x50x50, and you want a the fifth 
// k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will 
// automatically be clamped to 49.

// .SECTION See Also
// vtkGeometryFilter vtkStructuredGridFilter

#ifndef __vtkImageDataGeometryFilter_h
#define __vtkImageDataGeometryFilter_h

#include "vtkStructuredPointsToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkImageDataGeometryFilter : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkImageDataGeometryFilter,vtkStructuredPointsToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct with initial extent of all the data
  static vtkImageDataGeometryFilter *New();

  
  // Description:
  // Set / get the extent (imin,imax, jmin,jmax, kmin,kmax) indices.
  void SetExtent(int extent[6]);
  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);
  int *GetExtent() { return this->Extent;};

protected:
  vtkImageDataGeometryFilter();
  ~vtkImageDataGeometryFilter() {};

  void Execute();
  int Extent[6];
private:
  vtkImageDataGeometryFilter(const vtkImageDataGeometryFilter&);  // Not implemented.
  void operator=(const vtkImageDataGeometryFilter&);  // Not implemented.
};

#endif
