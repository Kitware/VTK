// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkMatrixToLinearTransform
 * @brief   convert a matrix to a transform
 *
 * This is a very simple class which allows a vtkMatrix4x4 to be used in
 * place of a vtkLinearTransform or vtkAbstractTransform.  For example,
 * if you use it as a proxy between a matrix and vtkTransformPolyDataFilter
 * then any modifications to the matrix will automatically be reflected in
 * the output of the filter.
 * @sa
 * vtkTransform vtkMatrix4x4 vtkMatrixToHomogeneousTransform
 */

#ifndef vtkMatrixToLinearTransform_h
#define vtkMatrixToLinearTransform_h

#include "vtkCommonTransformsModule.h" // For export macro
#include "vtkLinearTransform.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix4x4;

class VTKCOMMONTRANSFORMS_EXPORT vtkMatrixToLinearTransform : public vtkLinearTransform
{
public:
  static vtkMatrixToLinearTransform* New();
  vtkTypeMacro(vtkMatrixToLinearTransform, vtkLinearTransform);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the input matrix.  Any modifications to the matrix will be
   * reflected in the transformation.
   */
  virtual void SetInput(vtkMatrix4x4*);
  vtkGetObjectMacro(Input, vtkMatrix4x4);
  ///@}

  /**
   * The input matrix is left as-is, but the transformation matrix
   * is inverted.
   */
  void Inverse() override;

  /**
   * Get the MTime: this is the bit of magic that makes everything work.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Make a new transform of the same type.
   */
  vtkAbstractTransform* MakeTransform() override;

protected:
  vtkMatrixToLinearTransform();
  ~vtkMatrixToLinearTransform() override;

  void InternalUpdate() override;
  void InternalDeepCopy(vtkAbstractTransform* transform) override;

  vtkTypeBool InverseFlag;
  vtkMatrix4x4* Input;

private:
  vtkMatrixToLinearTransform(const vtkMatrixToLinearTransform&) = delete;
  void operator=(const vtkMatrixToLinearTransform&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
