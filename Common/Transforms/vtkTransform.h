// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkTransform
 * @brief   describes linear transformations via a 4x4 matrix
 *
 * A vtkTransform can be used to describe the full range of linear (also
 * known as affine) coordinate transformations in three dimensions,
 * which are internally represented as a 4x4 homogeneous transformation
 * matrix.  When you create a new vtkTransform, it is always initialized
 * to the identity transformation.
 * <P>The SetInput() method allows you to set another transform,
 * instead of the identity transform, to be the base transformation.
 * There is a pipeline mechanism to ensure that when the input is
 * modified, the current transformation will be updated accordingly.
 * This pipeline mechanism is also supported by the Concatenate() method.
 * <P>Most of the methods for manipulating this transformation,
 * e.g. Translate, Rotate, and Concatenate, can operate in either
 * PreMultiply (the default) or PostMultiply mode.  In PreMultiply
 * mode, the translation, concatenation, etc. will occur before any
 * transformations which are represented by the current matrix.  In
 * PostMultiply mode, the additional transformation will occur after
 * any transformations represented by the current matrix.
 * <P>This class performs all of its operations in a right handed
 * coordinate system with right handed rotations. Some other graphics
 * libraries use left handed coordinate systems and rotations.
 * @sa
 * vtkPerspectiveTransform vtkGeneralTransform vtkMatrix4x4
 * vtkTransformCollection vtkTransformFilter vtkTransformPolyDataFilter
 * vtkImageReslice
 */

#ifndef vtkTransform_h
#define vtkTransform_h

#include "vtkCommonTransformsModule.h" // For export macro
#include "vtkLinearTransform.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include "vtkMatrix4x4.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONTRANSFORMS_EXPORT VTK_MARSHALAUTO vtkTransform : public vtkLinearTransform
{
public:
  static vtkTransform* New();
  vtkTypeMacro(vtkTransform, vtkLinearTransform);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the transformation to the identity transformation.  If
   * the transform has an Input, then the transformation will be
   * reset so that it is the same as the Input.
   */
  void Identity();

  /**
   * Invert the transformation.  This will also set a flag so that
   * the transformation will use the inverse of its Input, if an Input
   * has been set.
   */
  void Inverse() override;

  ///@{
  /**
   * Create a translation matrix and concatenate it with the current
   * transformation according to PreMultiply or PostMultiply semantics.
   */
  void Translate(double x, double y, double z) { this->Concatenation->Translate(x, y, z); }
  void Translate(const double x[3]) { this->Translate(x[0], x[1], x[2]); }
  void Translate(const float x[3]) { this->Translate(x[0], x[1], x[2]); }
  ///@}

  ///@{
  /**
   * Create a rotation matrix and concatenate it with the current
   * transformation according to PreMultiply or PostMultiply semantics.
   * The angle is in degrees, and (x,y,z) specifies the axis that the
   * rotation will be performed around.
   */
  void RotateWXYZ(double angle, double x, double y, double z)
  {
    this->Concatenation->Rotate(angle, x, y, z);
  }
  void RotateWXYZ(double angle, const double axis[3])
  {
    this->RotateWXYZ(angle, axis[0], axis[1], axis[2]);
  }
  void RotateWXYZ(double angle, const float axis[3])
  {
    this->RotateWXYZ(angle, axis[0], axis[1], axis[2]);
  }
  ///@}

  ///@{
  /**
   * Create a rotation matrix about the X, Y, or Z axis and concatenate
   * it with the current transformation according to PreMultiply or
   * PostMultiply semantics.  The angle is expressed in degrees.
   */
  void RotateX(double angle) { this->RotateWXYZ(angle, 1, 0, 0); }
  void RotateY(double angle) { this->RotateWXYZ(angle, 0, 1, 0); }
  void RotateZ(double angle) { this->RotateWXYZ(angle, 0, 0, 1); }
  ///@}

  ///@{
  /**
   * Create a scale matrix (i.e. set the diagonal elements to x, y, z)
   * and concatenate it with the current transformation according to
   * PreMultiply or PostMultiply semantics.
   */
  void Scale(double x, double y, double z) { this->Concatenation->Scale(x, y, z); }
  void Scale(const double s[3]) { this->Scale(s[0], s[1], s[2]); }
  void Scale(const float s[3]) { this->Scale(s[0], s[1], s[2]); }
  ///@}

  ///@{
  /**
   * Set the current matrix directly. Note: First, the current
   * matrix is set to the identity, then the input matrix is concatenated.
   */
  void SetMatrix(vtkMatrix4x4* matrix) { this->SetMatrix(*matrix->Element); }
  void SetMatrix(const double elements[16])
  {
    this->Concatenation->Identity();
    this->Concatenate(elements);
  }
  ///@}

  ///@{
  /**
   * Concatenates the matrix with the current transformation according
   * to PreMultiply or PostMultiply semantics.
   */
  void Concatenate(vtkMatrix4x4* matrix) { this->Concatenate(*matrix->Element); }
  void Concatenate(const double elements[16]) { this->Concatenation->Concatenate(elements); }
  ///@}

  /**
   * Concatenate the specified transform with the current transformation
   * according to PreMultiply or PostMultiply semantics.
   * The concatenation is pipelined, meaning that if any of the
   * transformations are changed, even after Concatenate() is called,
   * those changes will be reflected when you call TransformPoint().
   */
  void Concatenate(vtkLinearTransform* transform);

  /**
   * Sets the internal state of the transform to PreMultiply. All subsequent
   * operations will occur before those already represented in the
   * current transformation.  In homogeneous matrix notation, M = M*A where
   * M is the current transformation matrix and A is the applied matrix.
   * The default is PreMultiply.
   */
  void PreMultiply()
  {
    if (this->Concatenation->GetPreMultiplyFlag())
    {
      return;
    }
    this->Concatenation->SetPreMultiplyFlag(1);
    this->Modified();
  }

  /**
   * Sets the internal state of the transform to PostMultiply. All subsequent
   * operations will occur after those already represented in the
   * current transformation.  In homogeneous matrix notation, M = A*M where
   * M is the current transformation matrix and A is the applied matrix.
   * The default is PreMultiply.
   */
  void PostMultiply()
  {
    if (!this->Concatenation->GetPreMultiplyFlag())
    {
      return;
    }
    this->Concatenation->SetPreMultiplyFlag(0);
    this->Modified();
  }

  /**
   * Get the total number of transformations that are linked into this
   * one via Concatenate() operations or via SetInput().
   */
  int GetNumberOfConcatenatedTransforms()
  {
    return this->Concatenation->GetNumberOfTransforms() + (this->Input == nullptr ? 0 : 1);
  }

  ///@{
  /**
   * Get one of the concatenated transformations as a vtkAbstractTransform.
   * These transformations are applied, in series, every time the
   * transformation of a coordinate occurs.  This method is provided
   * to make it possible to decompose a transformation into its
   * constituents, for example to save a transformation to a file.
   */
  vtkLinearTransform* GetConcatenatedTransform(int i)
  {
    vtkAbstractTransform* t;
    if (this->Input == nullptr)
    {
      t = this->Concatenation->GetTransform(i);
    }
    else if (i < this->Concatenation->GetNumberOfPreTransforms())
    {
      t = this->Concatenation->GetTransform(i);
    }
    else if (i > this->Concatenation->GetNumberOfPreTransforms())
    {
      t = this->Concatenation->GetTransform(i - 1);
    }
    else if (this->GetInverseFlag())
    {
      t = this->Input->GetInverse();
    }
    else
    {
      t = this->Input;
    }
    return static_cast<vtkLinearTransform*>(t);
  }
  ///@}

  ///@{
  /**
   * Get the x, y, z orientation angles from the transformation matrix as an
   * array of three floating point values.
   */
  void GetOrientation(double orient[3]);
  void GetOrientation(float orient[3])
  {
    double temp[3];
    this->GetOrientation(temp);
    orient[0] = static_cast<float>(temp[0]);
    orient[1] = static_cast<float>(temp[1]);
    orient[2] = static_cast<float>(temp[2]);
  }
  double* GetOrientation() VTK_SIZEHINT(3)
  {
    this->GetOrientation(this->ReturnValue);
    return this->ReturnValue;
  }
  ///@}

  /**
   * Convenience function to get the x, y, z orientation angles from
   * a transformation matrix as an array of three floating point values.
   */
  static void GetOrientation(double orient[3], vtkMatrix4x4* matrix);

  ///@{
  /**
   * Return the wxyz angle+axis representing the current orientation.
   * The angle is in degrees and the axis is a unit vector.
   */
  void GetOrientationWXYZ(double wxyz[4]);
  void GetOrientationWXYZ(float wxyz[4])
  {
    double temp[4];
    this->GetOrientationWXYZ(temp);
    wxyz[0] = static_cast<float>(temp[0]);
    wxyz[1] = static_cast<float>(temp[1]);
    wxyz[2] = static_cast<float>(temp[2]);
    wxyz[3] = static_cast<float>(temp[3]);
  }
  double* GetOrientationWXYZ() VTK_SIZEHINT(4)
  {
    this->GetOrientationWXYZ(this->ReturnValue);
    return this->ReturnValue;
  }
  ///@}

  ///@{
  /**
   * Return the position from the current transformation matrix as an array
   * of three floating point numbers. This is simply returning the translation
   * component of the 4x4 matrix.
   */
  void GetPosition(double pos[3]);
  void GetPosition(float pos[3])
  {
    double temp[3];
    this->GetPosition(temp);
    pos[0] = static_cast<float>(temp[0]);
    pos[1] = static_cast<float>(temp[1]);
    pos[2] = static_cast<float>(temp[2]);
  }
  double* GetPosition() VTK_SIZEHINT(3)
  {
    this->GetPosition(this->ReturnValue);
    return this->ReturnValue;
  }
  ///@}

  ///@{
  /**
   * Return the scale factors of the current transformation matrix as
   * an array of three float numbers.  These scale factors are not necessarily
   * about the x, y, and z axes unless unless the scale transformation was
   * applied before any rotations.
   */
  void GetScale(double scale[3]);
  void GetScale(float scale[3])
  {
    double temp[3];
    this->GetScale(temp);
    scale[0] = static_cast<float>(temp[0]);
    scale[1] = static_cast<float>(temp[1]);
    scale[2] = static_cast<float>(temp[2]);
  }
  double* GetScale() VTK_SIZEHINT(3)
  {
    this->GetScale(this->ReturnValue);
    return this->ReturnValue;
  }
  ///@}

  /**
   * Return a matrix which is the inverse of the current transformation
   * matrix.
   */
  void GetInverse(vtkMatrix4x4* inverse);

  /**
   * Return a matrix which is the transpose of the current transformation
   * matrix.  This is equivalent to the inverse if and only if the
   * transformation is a pure rotation with no translation or scale.
   */
  void GetTranspose(vtkMatrix4x4* transpose);

  ///@{
  /**
   * Set the input for this transformation.  This will be used as the
   * base transformation if it is set.  This method allows you to build
   * a transform pipeline: if the input is modified, then this transformation
   * will automatically update accordingly.  Note that the InverseFlag,
   * controlled via Inverse(), determines whether this transformation
   * will use the Input or the inverse of the Input.
   */
  void SetInput(vtkLinearTransform* input);
  vtkLinearTransform* GetInput() { return this->Input; }
  ///@}

  /**
   * Get the inverse flag of the transformation.  This controls
   * whether it is the Input or the inverse of the Input that
   * is used as the base transformation.  The InverseFlag is
   * flipped every time Inverse() is called.  The InverseFlag
   * is off when a transform is first created.
   */
  vtkTypeBool GetInverseFlag() { return this->Concatenation->GetInverseFlag(); }

  ///@{
  /**
   * Pushes the current transformation onto the transformation stack.
   */
  void Push()
  {
    if (this->Stack == nullptr)
    {
      this->Stack = vtkTransformConcatenationStack::New();
    }
    this->Stack->Push(&this->Concatenation);
    this->Modified();
  }
  ///@}

  ///@{
  /**
   * Deletes the transformation on the top of the stack and sets the top
   * to the next transformation on the stack.
   */
  void Pop()
  {
    if (this->Stack == nullptr)
    {
      return;
    }
    this->Stack->Pop(&this->Concatenation);
    this->Modified();
  }
  ///@}

  /**
   * Check for self-reference.  Will return true if concatenating
   * with the specified transform, setting it to be our inverse,
   * or setting it to be our input will create a circular reference.
   * CircuitCheck is automatically called by SetInput(), SetInverse(),
   * and Concatenate(vtkXTransform *).  Avoid using this function,
   * it is experimental.
   */
  int CircuitCheck(vtkAbstractTransform* transform) override;

  // Return an inverse transform which will always update itself
  // to match this transform.
  vtkAbstractTransform* GetInverse() { return vtkLinearTransform::GetInverse(); }

  /**
   * Make a new transform of the same type.
   */
  vtkAbstractTransform* MakeTransform() override;

  /**
   * Override GetMTime to account for input and concatenation.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Use this method only if you wish to compute the transformation in
   * homogeneous (x,y,z,w) coordinates, otherwise use TransformPoint().
   * This method calls this->GetMatrix()->MultiplyPoint().
   */
  void MultiplyPoint(const float in[4], float out[4]) { this->GetMatrix()->MultiplyPoint(in, out); }
  void MultiplyPoint(const double in[4], double out[4])
  {
    this->GetMatrix()->MultiplyPoint(in, out);
  }
  ///@}

protected:
  vtkTransform();
  ~vtkTransform() override;

  void InternalDeepCopy(vtkAbstractTransform* t) override;

  void InternalUpdate() override;

  vtkLinearTransform* Input;
  vtkTransformConcatenation* Concatenation;
  vtkTransformConcatenationStack* Stack;

  // this allows us to check whether people have been fooling
  // around with our matrix
  vtkMTimeType MatrixUpdateMTime;

  float Point[4];
  double DoublePoint[4];
  double ReturnValue[4];

private:
  vtkTransform(const vtkTransform&) = delete;
  void operator=(const vtkTransform&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
