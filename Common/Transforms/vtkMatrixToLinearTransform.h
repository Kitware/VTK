/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrixToLinearTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

class vtkMatrix4x4;

class VTKCOMMONTRANSFORMS_EXPORT vtkMatrixToLinearTransform : public vtkLinearTransform
{
 public:
  static vtkMatrixToLinearTransform *New();
  vtkTypeMacro(vtkMatrixToLinearTransform,vtkLinearTransform);
  void PrintSelf (ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the input matrix.  Any modifications to the matrix will be
   * reflected in the transformation.
   */
  virtual void SetInput(vtkMatrix4x4*);
  vtkGetObjectMacro(Input,vtkMatrix4x4);
  //@}

  /**
   * The input matrix is left as-is, but the transformation matrix
   * is inverted.
   */
  void Inverse() VTK_OVERRIDE;

  /**
   * Get the MTime: this is the bit of magic that makes everything work.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * Make a new transform of the same type.
   */
  vtkAbstractTransform *MakeTransform() VTK_OVERRIDE;

protected:
  vtkMatrixToLinearTransform();
  ~vtkMatrixToLinearTransform() VTK_OVERRIDE;

  void InternalUpdate() VTK_OVERRIDE;
  void InternalDeepCopy(vtkAbstractTransform *transform) VTK_OVERRIDE;

  int InverseFlag;
  vtkMatrix4x4 *Input;
private:
  vtkMatrixToLinearTransform(const vtkMatrixToLinearTransform&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMatrixToLinearTransform&) VTK_DELETE_FUNCTION;
};

#endif
