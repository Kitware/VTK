// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTransformFilter
 * @brief   transform points and associated normals and vectors
 *
 * vtkTransformFilter is a filter to transform point coordinates, and
 * associated point normals and vectors, as well as cell normals and vectors.
 * Transformed data array will be stored in a float array or a double array.
 * Other point and cell data are passed through the filter, unless
 * TransformAllInputVectors is set to true, in this case all other 3
 * components arrays from point and cell data will be transformed as well.
 *
 * An alternative method of transformation is to use vtkActor's methods
 * to scale, rotate, and translate objects. The difference between the
 * two methods is that vtkActor's transformation simply effects where
 * objects are rendered (via the graphics pipeline), whereas
 * vtkTransformFilter actually modifies point coordinates in the
 * visualization pipeline. This is necessary for some objects
 * (e.g., vtkProbeFilter) that require point coordinates as input.
 *
 * @sa
 * vtkAbstractTransform vtkTransformPolyDataFilter vtkActor
 */

#ifndef vtkTransformFilter_h
#define vtkTransformFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractTransform;

class VTKFILTERSGENERAL_EXPORT vtkTransformFilter : public vtkPointSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing.
   */
  static vtkTransformFilter* New();
  vtkTypeMacro(vtkTransformFilter, vtkPointSetAlgorithm);
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

  ///@{
  /**
   * If off (the default), only Vectors and Normals will be transformed.  If
   * on, all 3-component data arrays (treated as 3D vectors) will be
   * transformed, while other non-3-component data arrays will be passed
   * through to the output unchanged.
   */
  vtkSetMacro(TransformAllInputVectors, bool);
  vtkGetMacro(TransformAllInputVectors, bool);
  vtkBooleanMacro(TransformAllInputVectors, bool);
  ///@}

protected:
  vtkTransformFilter();
  ~vtkTransformFilter() override;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Specifies that the filter only takes input dataset types of vtkPointSet, vtkImageData,
  // and vtkRectilinearGrid.
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkDataArray* CreateNewDataArray(vtkDataArray* input = nullptr);

  vtkAbstractTransform* Transform;
  int OutputPointsPrecision;
  bool TransformAllInputVectors;

private:
  vtkTransformFilter(const vtkTransformFilter&) = delete;
  void operator=(const vtkTransformFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
