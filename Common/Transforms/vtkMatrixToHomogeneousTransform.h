/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrixToHomogeneousTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMatrixToHomogeneousTransform - convert a matrix to a transform
// .SECTION Description
// This is a very simple class which allows a vtkMatrix4x4 to be used in
// place of a vtkHomogeneousTransform or vtkAbstractTransform.  For example,
// if you use it as a proxy between a matrix and vtkTransformPolyDataFilter
// then any modifications to the matrix will automatically be reflected in
// the output of the filter.
// .SECTION See Also
// vtkPerspectiveTransform vtkMatrix4x4 vtkMatrixToLinearTransform

#ifndef __vtkMatrixToHomogeneousTransform_h
#define __vtkMatrixToHomogeneousTransform_h

#include "vtkCommonTransformsModule.h" // For export macro
#include "vtkHomogeneousTransform.h"

class vtkMatrix4x4;

class VTKCOMMONTRANSFORMS_EXPORT vtkMatrixToHomogeneousTransform : public vtkHomogeneousTransform
{
 public:
  static vtkMatrixToHomogeneousTransform *New();
  vtkTypeMacro(vtkMatrixToHomogeneousTransform,vtkHomogeneousTransform);
  void PrintSelf (ostream& os, vtkIndent indent);

  // Set the input matrix.  Any modifications to the matrix will be
  // reflected in the transformation.
  virtual void SetInput(vtkMatrix4x4*);
  vtkGetObjectMacro(Input,vtkMatrix4x4);

  // Description:
  // The input matrix is left as-is, but the transformation matrix
  // is inverted.
  void Inverse();

  // Description:
  // Get the MTime: this is the bit of magic that makes everything work.
  unsigned long GetMTime();

  // Description:
  // Make a new transform of the same type.
  vtkAbstractTransform *MakeTransform();

protected:
  vtkMatrixToHomogeneousTransform();
  ~vtkMatrixToHomogeneousTransform();

  void InternalUpdate();
  void InternalDeepCopy(vtkAbstractTransform *transform);

  int InverseFlag;
  vtkMatrix4x4 *Input;
private:
  vtkMatrixToHomogeneousTransform(const vtkMatrixToHomogeneousTransform&);  // Not implemented.
  void operator=(const vtkMatrixToHomogeneousTransform&);  // Not implemented.
};

#endif
