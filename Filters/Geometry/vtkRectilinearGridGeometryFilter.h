/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRectilinearGridGeometryFilter
 * @brief   extract geometry for a rectilinear grid
 *
 * vtkRectilinearGridGeometryFilter is a filter that extracts geometry from a
 * rectilinear grid. By specifying appropriate i-j-k indices, it is possible
 * to extract a point, a curve, a surface, or a "volume". The volume
 * is actually a (n x m x o) region of points.
 *
 * The extent specification is zero-offset. That is, the first k-plane in
 * a 50x50x50 rectilinear grid is given by (0,49, 0,49, 0,0).
 *
 * @warning
 * If you don't know the dimensions of the input dataset, you can use a large
 * number to specify extent (the number will be clamped appropriately). For
 * example, if the dataset dimensions are 50x50x50, and you want a the fifth
 * k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will
 * automatically be clamped to 49.
 *
 * @sa
 * vtkGeometryFilter vtkExtractGrid
*/

#ifndef vtkRectilinearGridGeometryFilter_h
#define vtkRectilinearGridGeometryFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGEOMETRY_EXPORT vtkRectilinearGridGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkRectilinearGridGeometryFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct with initial extent (0,100, 0,100, 0,0) (i.e., a k-plane).
   */
  static vtkRectilinearGridGeometryFilter *New();

  //@{
  /**
   * Get the extent in topological coordinate range (imin,imax, jmin,jmax,
   * kmin,kmax).
   */
  vtkGetVectorMacro(Extent,int,6);
  //@}

  /**
   * Specify (imin,imax, jmin,jmax, kmin,kmax) indices.
   */
  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);

  /**
   * Specify (imin,imax, jmin,jmax, kmin,kmax) indices in array form.
   */
  void SetExtent(int extent[6]);

protected:
  vtkRectilinearGridGeometryFilter();
  ~vtkRectilinearGridGeometryFilter() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  int Extent[6];
private:
  vtkRectilinearGridGeometryFilter(const vtkRectilinearGridGeometryFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRectilinearGridGeometryFilter&) VTK_DELETE_FUNCTION;
};

#endif
