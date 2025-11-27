// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointsMatchingTransformFilter
 * @brief   affine transform a vtkPointSet based on 4 pairs of points
 *
 * Allow the user to specify 4 pairs of points in space, to compute
 * the affine transform between the two defined coordinates systems and
 * to perform the transform on the input.
 */

#ifndef vtkPointsMatchingTransformFilter_h
#define vtkPointsMatchingTransformFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

#include "vtkMatrix4x4.h" // For vtkMatrix4x4

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix3x3;

class VTKFILTERSGENERAL_EXPORT vtkPointsMatchingTransformFilter : public vtkPointSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing.
   */
  static vtkPointsMatchingTransformFilter* New();
  vtkTypeMacro(vtkPointsMatchingTransformFilter, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Sets one of the four point pairs that define the transformation.
   * These methods directly update the corresponding column in the Source
   * or Target matrix used to compute the transform.
   */
  void SetSourcePoint1(double x, double y, double z);
  void SetSourcePoint2(double x, double y, double z);
  void SetSourcePoint3(double x, double y, double z);
  void SetSourcePoint4(double x, double y, double z);
  void SetTargetPoint1(double x, double y, double z);
  void SetTargetPoint2(double x, double y, double z);
  void SetTargetPoint3(double x, double y, double z);
  void SetTargetPoint4(double x, double y, double z);
  void SetSourcePoint(int index, double x, double y, double z);
  void SetTargetPoint(int index, double x, double y, double z);
  ///@}

  ///@{
  /**
   * Set/Get the source matrix.
   */
  void SetSourceMatrix(vtkMatrix4x4*);
  vtkGetObjectMacro(SourceMatrix, vtkMatrix4x4);
  ///@}

  ///@{
  /**
   * Set/Get the target matrix.
   */
  void SetTargetMatrix(vtkMatrix4x4*);
  vtkGetObjectMacro(TargetMatrix, vtkMatrix4x4);
  ///@}

  ///@{
  /**
   * Set/Get the RigidTransform option. If true, approximate the transform with the closest rigid +
   * scale transform. Default is false.
   */
  vtkSetMacro(RigidTransform, bool);
  vtkGetMacro(RigidTransform, bool);
  vtkBooleanMacro(RigidTransform, bool);
  ///@}

  /**
   * Return the mtime also considering the source and target matrices.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkPointsMatchingTransformFilter();
  ~vtkPointsMatchingTransformFilter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPointsMatchingTransformFilter(const vtkPointsMatchingTransformFilter&) = delete;
  void operator=(const vtkPointsMatchingTransformFilter&) = delete;

  vtkSmartPointer<vtkMatrix4x4> SourceMatrix;
  vtkSmartPointer<vtkMatrix4x4> TargetMatrix;
  bool RigidTransform = false;
};

VTK_ABI_NAMESPACE_END
#endif
