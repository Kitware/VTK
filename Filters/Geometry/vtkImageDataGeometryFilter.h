// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageDataGeometryFilter
 * @brief   extract geometry for structured points
 *
 * vtkImageDataGeometryFilter is a filter that extracts geometry from a
 * structured points dataset. By specifying appropriate i-j-k indices (via the
 * "Extent" instance variable), it is possible to extract a point, a line, a
 * plane (i.e., image), or a "volume" from dataset. (Since the output is
 * of type polydata, the volume is actually a (n x m x o) region of points.)
 *
 * The extent specification is zero-offset. That is, the first k-plane in
 * a 50x50x50 volume is given by (0,49, 0,49, 0,0).
 * @warning
 * If you don't know the dimensions of the input dataset, you can use a large
 * number to specify extent (the number will be clamped appropriately). For
 * example, if the dataset dimensions are 50x50x50, and you want a the fifth
 * k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will
 * automatically be clamped to 49.
 *
 * @sa
 * vtkGeometryFilter vtkStructuredGridSource
 */

#ifndef vtkImageDataGeometryFilter_h
#define vtkImageDataGeometryFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGEOMETRY_EXPORT vtkImageDataGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkImageDataGeometryFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with initial extent of all the data
   */
  static vtkImageDataGeometryFilter* New();

  ///@{
  /**
   * Set / get the extent (imin,imax, jmin,jmax, kmin,kmax) indices.
   */
  void SetExtent(int extent[6]);
  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);
  int* GetExtent() VTK_SIZEHINT(6) { return this->Extent; }
  ///@}

  ///@{
  /**
   * Set ThresholdCells to true if you wish to skip any voxel/pixels which have scalar
   * values less than the specified threshold.
   * Currently this functionality is only implemented for 2D imagedata
   */
  vtkSetMacro(ThresholdCells, vtkTypeBool);
  vtkGetMacro(ThresholdCells, vtkTypeBool);
  vtkBooleanMacro(ThresholdCells, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set ThresholdValue to the scalar value by which to threshold cells when extracting geometry
   * when ThresholdCells is true. Cells with scalar values greater than the threshold will be
   * output.
   */
  vtkSetMacro(ThresholdValue, double);
  vtkGetMacro(ThresholdValue, double);
  vtkBooleanMacro(ThresholdValue, double);
  ///@}

  ///@{
  /**
   * Set OutputTriangles to true if you wish to generate triangles instead of quads
   * when extracting cells from 2D imagedata
   * Currently this functionality is only implemented for 2D imagedata
   */
  vtkSetMacro(OutputTriangles, vtkTypeBool);
  vtkGetMacro(OutputTriangles, vtkTypeBool);
  vtkBooleanMacro(OutputTriangles, vtkTypeBool);
  ///@}

protected:
  vtkImageDataGeometryFilter();
  ~vtkImageDataGeometryFilter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int Extent[6];
  vtkTypeBool ThresholdCells;
  double ThresholdValue;
  vtkTypeBool OutputTriangles;

private:
  vtkImageDataGeometryFilter(const vtkImageDataGeometryFilter&) = delete;
  void operator=(const vtkImageDataGeometryFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
