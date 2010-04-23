/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredGridGeometryFilter - extract geometry for structured grid
// .SECTION Description
// vtkStructuredGridGeometryFilter is a filter that extracts geometry from a
// structured grid. By specifying appropriate i-j-k indices, it is possible
// to extract a point, a curve, a surface, or a "volume". Depending upon the
// type of data, the curve and surface may be curved or planar. (The volume
// is actually a (n x m x o) region of points.)
//
// The extent specification is zero-offset. That is, the first k-plane in
// a 50x50x50 structured grid is given by (0,49, 0,49, 0,0).
//
// The output of this filter is affected by the structured grid blanking.
// If blanking is on, and a blanking array defined, then those cells 
// attached to blanked points are not output. (Blanking is a property of
// the input vtkStructuredGrid.)

// .SECTION Caveats
// If you don't know the dimensions of the input dataset, you can use a large
// number to specify extent (the number will be clamped appropriately). For 
// example, if the dataset dimensions are 50x50x50, and you want a the fifth 
// k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will 
// automatically be clamped to 49.

// .SECTION See Also
// vtkGeometryFilter vtkExtractGrid vtkStructuredGrid

#ifndef __vtkStructuredGridGeometryFilter_h
#define __vtkStructuredGridGeometryFilter_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkStructuredGridGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkStructuredGridGeometryFilter *New();
  vtkTypeMacro(vtkStructuredGridGeometryFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the extent in topological coordinate range (imin,imax, jmin,jmax,
  // kmin,kmax).
  vtkGetVectorMacro(Extent,int,6);

  // Description:
  // Specify (imin,imax, jmin,jmax, kmin,kmax) indices.
  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);

  // Description:
  // Specify (imin,imax, jmin,jmax, kmin,kmax) indices in array form.
  void SetExtent(int extent[6]);

protected:
  vtkStructuredGridGeometryFilter();
  ~vtkStructuredGridGeometryFilter() {}

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  int Extent[6];
private:
  vtkStructuredGridGeometryFilter(const vtkStructuredGridGeometryFilter&);  // Not implemented.
  void operator=(const vtkStructuredGridGeometryFilter&);  // Not implemented.
};

#endif


