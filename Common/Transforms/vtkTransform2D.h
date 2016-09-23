/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransform2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkTransform2D
 * @brief   describes linear transformations via a 3x3 matrix
 *
 * A vtkTransform2D can be used to describe the full range of linear (also
 * known as affine) coordinate transformations in two dimensions,
 * which are internally represented as a 3x3 homogeneous transformation
 * matrix.  When you create a new vtkTransform2D, it is always initialized
 * to the identity transformation.
 *
 * All multiplicitive operations (Translate, Rotate, Scale, etc) are
 * post-multiplied in this class (i.e. add them in the reverse of the order
 * that they should be applied).
 *
 * This class performs all of its operations in a right handed
 * coordinate system with right handed rotations. Some other graphics
 * libraries use left handed coordinate systems and rotations.
*/

#ifndef vtkTransform2D_h
#define vtkTransform2D_h

#include "vtkCommonTransformsModule.h" // For export macro
#include "vtkObject.h"

#include "vtkMatrix3x3.h" // Needed for inline methods

class vtkPoints2D;

class VTKCOMMONTRANSFORMS_EXPORT vtkTransform2D : public vtkObject
{
 public:
  static vtkTransform2D *New();
  vtkTypeMacro(vtkTransform2D,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set the transformation to the identity transformation.
   */
  void Identity();

  /**
   * Invert the transformation.
   */
  void Inverse();

  /**
   * Create a translation matrix and concatenate it with the current
   * transformation.
   */
  void Translate(double x, double y);
  void Translate(const double x[2]) { this->Translate(x[0], x[1]); }
  void Translate(const float x[2]) { this->Translate(x[0], x[1]); }

  /**
   * Create a rotation matrix and concatenate it with the current
   * transformation. The angle is in degrees.
   */
  void Rotate(double angle);

  /**
   * Create a scale matrix (i.e. set the diagonal elements to x, y)
   * and concatenate it with the current transformation.
   */
  void Scale(double x, double y);
  void Scale(const double s[2]) { this->Scale(s[0], s[1]); }
  void Scale(const float s[2]) { this->Scale(s[0], s[1]); }

  /**
   * Set the current matrix directly.
   */
  void SetMatrix(vtkMatrix3x3 *matrix) {
    this->SetMatrix(matrix->GetData()); }
  void SetMatrix(const double elements[9]);

  //@{
  /**
   * Get the underlying 3x3 matrix.
   */
  vtkGetObjectMacro(Matrix, vtkMatrix3x3);
  void GetMatrix(vtkMatrix3x3 *matrix);
  //@}

  //@{
  /**
   * Return the position from the current transformation matrix as an array
   * of two floating point numbers. This is simply returning the translation
   * component of the 3x3 matrix.
   */
  void GetPosition(double pos[2]);
  void GetPosition(float pos[2]) {
    double temp[2];
    this->GetPosition(temp);
    pos[0] = static_cast<float>(temp[0]);
    pos[1] = static_cast<float>(temp[1]); }
  //@}

  //@{
  /**
   * Return the x and y scale from the current transformation matrix as an array
   * of two floating point numbers. This is simply returning the scale
   * component of the 3x3 matrix.
   */
  void GetScale(double pos[2]);
  void GetScale(float pos[2]) {
    double temp[2];
    this->GetScale(temp);
    pos[0] = static_cast<float>(temp[0]);
    pos[1] = static_cast<float>(temp[1]); }
  //@}

  /**
   * Return a matrix which is the inverse of the current transformation
   * matrix.
   */
  void GetInverse(vtkMatrix3x3 *inverse);

  /**
   * Return a matrix which is the transpose of the current transformation
   * matrix.  This is equivalent to the inverse if and only if the
   * transformation is a pure rotation with no translation or scale.
   */
  void GetTranspose(vtkMatrix3x3 *transpose);

  /**
   * Override GetMTime to account for input and concatenation.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * Apply the transformation to a series of points, and append the
   * results to outPts. Where n is the number of points, and the float pointers
   * are of length 2*n.
   */
  void TransformPoints(const float *inPts, float *outPts, int n);

  /**
   * Apply the transformation to a series of points, and append the
   * results to outPts. Where n is the number of points, and the float pointers
   * are of length 2*n.
   */
  void TransformPoints(const double *inPts, double *outPts, int n);

  /**
   * Apply the transformation to a series of points, and append the
   * results to outPts.
   */
  void TransformPoints(vtkPoints2D *inPts, vtkPoints2D *outPts);

  /**
   * Apply the transformation to a series of points, and append the
   * results to outPts. Where n is the number of points, and the float pointers
   * are of length 2*n.
   */
  void InverseTransformPoints(const float *inPts, float *outPts, int n);

  /**
   * Apply the transformation to a series of points, and append the
   * results to outPts. Where n is the number of points, and the float pointers
   * are of length 2*n.
   */
  void InverseTransformPoints(const double *inPts, double *outPts, int n);

  /**
   * Apply the transformation to a series of points, and append the
   * results to outPts.
   */
  void InverseTransformPoints(vtkPoints2D *inPts, vtkPoints2D *outPts);

  //@{
  /**
   * Use this method only if you wish to compute the transformation in
   * homogeneous (x,y,w) coordinates, otherwise use TransformPoint().
   * This method calls this->GetMatrix()->MultiplyPoint().
   */
  void MultiplyPoint(const float in[3], float out[3]) {
    this->GetMatrix()->MultiplyPoint(in,out);};
  void MultiplyPoint(const double in[3], double out[3]) {
    this->GetMatrix()->MultiplyPoint(in,out);};
  //@}

protected:
  vtkTransform2D ();
  ~vtkTransform2D () VTK_OVERRIDE;

  void InternalDeepCopy(vtkTransform2D *t);

  vtkMatrix3x3 *Matrix;
  vtkMatrix3x3 *InverseMatrix;

private:
  vtkTransform2D (const vtkTransform2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTransform2D&) VTK_DELETE_FUNCTION;
};

#endif
