/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdentityTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkIdentityTransform
 * @brief   a transform that doesn't do anything
 *
 * vtkIdentityTransform is a transformation which will simply pass coordinate
 * data unchanged.  All other transform types can also do this, however,
 * the vtkIdentityTransform does so with much greater efficiency.
 * @sa
 * vtkLinearTransform
*/

#ifndef vtkIdentityTransform_h
#define vtkIdentityTransform_h

#include "vtkCommonTransformsModule.h" // For export macro
#include "vtkLinearTransform.h"

class VTKCOMMONTRANSFORMS_EXPORT vtkIdentityTransform : public vtkLinearTransform
{
public:
  static vtkIdentityTransform *New();

  vtkTypeMacro(vtkIdentityTransform,vtkLinearTransform);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Apply the transformation to a series of points, and append the
   * results to outPts.
   */
  void TransformPoints(vtkPoints *inPts, vtkPoints *outPts) VTK_OVERRIDE;

  /**
   * Apply the transformation to a series of normals, and append the
   * results to outNms.
   */
  void TransformNormals(vtkDataArray *inNms, vtkDataArray *outNms) VTK_OVERRIDE;

  /**
   * Apply the transformation to a series of vectors, and append the
   * results to outVrs.
   */
  void TransformVectors(vtkDataArray *inVrs, vtkDataArray *outVrs) VTK_OVERRIDE;

  /**
   * Apply the transformation to a combination of points, normals
   * and vectors.
   */
  void TransformPointsNormalsVectors(vtkPoints *inPts,
                                     vtkPoints *outPts,
                                     vtkDataArray *inNms,
                                     vtkDataArray *outNms,
                                     vtkDataArray *inVrs,
                                     vtkDataArray *outVrs) VTK_OVERRIDE;

  // Invert the transformation.  This doesn't do anything to the
  // identity transformation.
  void Inverse() VTK_OVERRIDE {}

  //@{
  /**
   * This will calculate the transformation without calling Update.
   * Meant for use only within other VTK classes.
   */
  void InternalTransformPoint(const float in[3], float out[3]) VTK_OVERRIDE;
  void InternalTransformPoint(const double in[3], double out[3]) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * This will calculate the transformation without calling Update.
   * Meant for use only within other VTK classes.
   */
  void InternalTransformNormal(const float in[3], float out[3]) VTK_OVERRIDE;
  void InternalTransformNormal(const double in[3], double out[3]) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * This will calculate the transformation without calling Update.
   * Meant for use only within other VTK classes.
   */
  void InternalTransformVector(const float in[3], float out[3]) VTK_OVERRIDE;
  void InternalTransformVector(const double in[3], double out[3]) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * This will calculate the transformation as well as its derivative
   * without calling Update.  Meant for use only within other VTK
   * classes.
   */
  void InternalTransformDerivative(const float in[3], float out[3],
                                   float derivative[3][3]) VTK_OVERRIDE;
  void InternalTransformDerivative(const double in[3], double out[3],
                                   double derivative[3][3]) VTK_OVERRIDE;
  //@}

  /**
   * Make a transform of the same type.  This will actually
   * return the same transform.
   */
  vtkAbstractTransform *MakeTransform() VTK_OVERRIDE;

protected:
  vtkIdentityTransform();
  ~vtkIdentityTransform() VTK_OVERRIDE;

  void InternalDeepCopy(vtkAbstractTransform *t) VTK_OVERRIDE;

private:
  vtkIdentityTransform(const vtkIdentityTransform&) VTK_DELETE_FUNCTION;
  void operator=(const vtkIdentityTransform&) VTK_DELETE_FUNCTION;
};

#endif





