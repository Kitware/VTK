// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredGridGeometryFilter
 * @brief   extract geometry for structured grid
 *
 * vtkStructuredGridGeometryFilter is a filter that extracts geometry from a
 * structured grid. By specifying appropriate i-j-k indices, it is possible
 * to extract a point, a curve, a surface, or a "volume". Depending upon the
 * type of data, the curve and surface may be curved or planar. (The volume
 * is actually a (n x m x o) region of points.)
 *
 * The extent specification is zero-offset. That is, the first k-plane in
 * a 50x50x50 structured grid is given by (0,49, 0,49, 0,0).
 *
 * The output of this filter is affected by the structured grid blanking.
 * If blanking is on, and a blanking array defined, then those cells
 * attached to blanked points are not output. (Blanking is a property of
 * the input vtkStructuredGrid.)
 *
 * @warning
 * If you don't know the dimensions of the input dataset, you can use a large
 * number to specify extent (the number will be clamped appropriately). For
 * example, if the dataset dimensions are 50x50x50, and you want a the fifth
 * k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will
 * automatically be clamped to 49.
 *
 * @sa
 * vtkGeometryFilter vtkExtractGrid vtkStructuredGrid
 */

#ifndef vtkStructuredGridGeometryFilter_h
#define vtkStructuredGridGeometryFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGEOMETRY_EXPORT vtkStructuredGridGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkStructuredGridGeometryFilter* New();
  vtkTypeMacro(vtkStructuredGridGeometryFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the extent in topological coordinate range (imin,imax, jmin,jmax,
   * kmin,kmax).
   */
  vtkGetVectorMacro(Extent, int, 6);
  ///@}

  /**
   * Specify (imin,imax, jmin,jmax, kmin,kmax) indices.
   */
  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);

  /**
   * Specify (imin,imax, jmin,jmax, kmin,kmax) indices in array form.
   */
  void SetExtent(int extent[6]);

protected:
  vtkStructuredGridGeometryFilter();
  ~vtkStructuredGridGeometryFilter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int Extent[6];

private:
  vtkStructuredGridGeometryFilter(const vtkStructuredGridGeometryFilter&) = delete;
  void operator=(const vtkStructuredGridGeometryFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
