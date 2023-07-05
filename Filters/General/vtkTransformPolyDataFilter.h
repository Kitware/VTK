// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTransformPolyDataFilter
 * @brief   transform points and associated normals and vectors for polygonal dataset
 *
 * vtkTransformPolyDataFilter is a filter to transform point
 * coordinates and associated point and cell normals and
 * vectors. Other point and cell data is passed through the filter
 * unchanged. This filter is specialized for polygonal data. See
 * vtkTransformFilter for more general data.
 *
 * An alternative method of transformation is to use vtkActor's methods
 * to scale, rotate, and translate objects. The difference between the
 * two methods is that vtkActor's transformation simply effects where
 * objects are rendered (via the graphics pipeline), whereas
 * vtkTransformPolyDataFilter actually modifies point coordinates in the
 * visualization pipeline. This is necessary for some objects
 * (e.g., vtkProbeFilter) that require point coordinates as input.
 *
 * @sa
 * vtkTransform vtkTransformFilter vtkActor
 */

#ifndef vtkTransformPolyDataFilter_h
#define vtkTransformPolyDataFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractTransform;

class VTKFILTERSGENERAL_EXPORT vtkTransformPolyDataFilter : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing.
   */
  static vtkTransformPolyDataFilter* New();
  vtkTypeMacro(vtkTransformPolyDataFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Return the MTime also considering the transform.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Specify the transform object used to transform points.
   */
  virtual void SetTransform(vtkAbstractTransform*);
  vtkGetObjectMacro(Transform, vtkAbstractTransform);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkTransformPolyDataFilter();
  ~vtkTransformPolyDataFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkAbstractTransform* Transform;
  int OutputPointsPrecision;

private:
  vtkTransformPolyDataFilter(const vtkTransformPolyDataFilter&) = delete;
  void operator=(const vtkTransformPolyDataFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
