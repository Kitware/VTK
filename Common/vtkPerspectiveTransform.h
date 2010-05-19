/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerspectiveTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPerspectiveTransform - describes a 4x4 matrix transformation
// .SECTION Description
// A vtkPerspectiveTransform can be used to describe the full range of
// homogeneous transformations.  It was designed in particular
// to describe a camera-view of a scene.  
// <P>The order in which you set up the display coordinates (via 
// AdjustZBuffer() and AdjustViewport()), the projection (via Perspective(), 
// Frustum(), or Ortho()) and the camera view (via SetupCamera()) are
// important.  If the transform is in PreMultiply mode, which is the 
// default, set the Viewport and ZBuffer first, then the projection, and
// finally the camera view.  Once the view is set up, the Translate
// and Rotate methods can be used to move the camera around in world
// coordinates.  If the Oblique() or Stereo() methods are used, they 
// should be called just before SetupCamera().
// <P>In PostMultiply mode, you must perform all transformations
// in the opposite order.  This is necessary, for example, if you
// already have a perspective transformation set up but must adjust
// the viewport.  Another example is if you have a view transformation,
// and wish to perform translations and rotations in the camera's 
// coordinate system rather than in world coordinates.
// <P>The SetInput and Concatenate methods can be used to create 
// a transformation pipeline with vtkPerspectiveTransform.  See vtkTransform
// for more information on the transformation pipeline.
// .SECTION See Also
// vtkGeneralTransform vtkTransform vtkMatrix4x4 vtkCamera

#ifndef __vtkPerspectiveTransform_h
#define __vtkPerspectiveTransform_h

#include "vtkHomogeneousTransform.h"

#include "vtkMatrix4x4.h" // Needed for inline methods

class VTK_COMMON_EXPORT vtkPerspectiveTransform : public vtkHomogeneousTransform
{
 public:
  static vtkPerspectiveTransform *New();
  vtkTypeMacro(vtkPerspectiveTransform,vtkHomogeneousTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set this transformation to the identity transformation.  If 
  // the transform has an Input, then the transformation will be
  // reset so that it is the same as the Input.
  void Identity() { this->Concatenation->Identity(); this->Modified(); };

  // Description:
  // Invert the transformation.  This will also set a flag so that
  // the transformation will use the inverse of its Input, if an Input
  // has been set.
  void Inverse() { this->Concatenation->Inverse(); this->Modified(); };

  // Description:
  // Perform an adjustment to the viewport coordinates.  By default Ortho,
  // Frustum, and Perspective provide a window of ([-1,+1],[-1,+1]).
  // In PreMultiply mode, you call this method before calling Ortho, Frustum,
  // or Perspective.  In PostMultiply mode you can call it after.  Note
  // that if you must apply both AdjustZBuffer and AdjustViewport, it
  // makes no difference which order you apply them in.
  void AdjustViewport(double oldXMin, double oldXMax, 
                      double oldYMin, double oldYMax,
                      double newXMin, double newXMax, 
                      double newYMin, double newYMax);

  // Description:
  // Perform an adjustment to the Z-Buffer range that the near and far
  // clipping planes map to.  By default Ortho, Frustum, and Perspective
  // map the near clipping plane to -1 and the far clipping plane to +1.
  // In PreMultiply mode, you call this method before calling Ortho, Frustum,
  // or Perspective.  In PostMultiply mode you can call it after.
  void AdjustZBuffer(double oldNearZ, double oldFarZ,
                     double newNearZ, double newFarZ);

  // Description:
  // Create an orthogonal projection matrix and concatenate it by the
  // current transformation.  The matrix maps [xmin,xmax], [ymin,ymax], 
  // [-znear,-zfar] to [-1,+1], [-1,+1], [+1,-1]. 
  void Ortho(double xmin, double xmax, double ymin, double ymax, 
             double znear, double zfar);

  // Description:
  // Create an perspective projection matrix and concatenate it by the
  // current transformation.  The matrix maps a frustum with a back
  // plane at -zfar and a front plane at -znear with extent 
  // [xmin,xmax],[ymin,ymax] to [-1,+1], [-1,+1], [+1,-1].
  void Frustum(double xmin, double xmax, double ymin, double ymax, 
               double znear, double zfar);

  // Description:
  // Create a perspective projection matrix by specifying the view angle
  // (this angle is in the y direction), the aspect ratio, and the near 
  // and far clipping range.  The projection matrix is concatenated 
  // with the current transformation.  This method works via Frustum.
  void Perspective(double angle, double aspect, double znear, double zfar);

  // Description:
  // Create a shear transformation about a plane at distance z from
  // the camera.  The values dxdz (i.e. dx/dz) and dydz specify the
  // amount of shear in the x and y directions.  The 'zplane' specifies
  // the distance from the camera to the plane at which the shear
  // causes zero displacement.  Generally you want this plane to be the
  // focal plane.
  // This transformation can be used in combination with Ortho to create 
  // an oblique projection.  It can also be used in combination with
  // Perspective to provide correct stereo views when the eye is at
  // arbitrary but known positions relative to the center of a flat
  // viewing screen.
  void Shear(double dxdz, double dydz, double zplane);

  // Description:
  // Create a stereo shear matrix and concatenate it with the
  // current transformation.  This can be applied in conjunction with either a 
  // perspective transformation (via Frustum or Projection) or an
  // orthographic projection.  You must specify the distance from
  // the camera plane to the focal plane, and the angle between
  // the distance vector and the eye.  The angle should be negative
  // for the left eye, and positive for the right.  This method
  // works via Oblique.
  void Stereo(double angle, double focaldistance);

  // Description:
  // Set a view transformation matrix for the camera (this matrix does
  // not contain any perspective) and concatenate it with the current
  // transformation.
  void SetupCamera(const double position[3], const double focalpoint[3],
                   const double viewup[3]);

  void SetupCamera(double p0, double p1, double p2,
                   double fp0, double fp1, double fp2,
                   double vup0, double vup1, double vup2);

  // Description:
  // Create a translation matrix and concatenate it with the current
  // transformation according to PreMultiply or PostMultiply semantics.
  void Translate(double x, double y, double z) {
    this->Concatenation->Translate(x,y,z); };
  void Translate(const double x[3]) { this->Translate(x[0], x[1], x[2]); };
  void Translate(const float x[3]) { this->Translate(x[0], x[1], x[2]); };

  // Description:
  // Create a rotation matrix and concatenate it with the current
  // transformation according to PreMultiply or PostMultiply semantics.
  // The angle is in degrees, and (x,y,z) specifies the axis that the
  // rotation will be performed around. 
  void RotateWXYZ(double angle, double x, double y, double z) {
    this->Concatenation->Rotate(angle,x,y,z); };
  void RotateWXYZ(double angle, const double axis[3]) {
    this->RotateWXYZ(angle, axis[0], axis[1], axis[2]); };
  void RotateWXYZ(double angle, const float axis[3]) {
    this->RotateWXYZ(angle, axis[0], axis[1], axis[2]); };

  // Description:
  // Create a rotation matrix about the X, Y, or Z axis and concatenate
  // it with the current transformation according to PreMultiply or
  // PostMultiply semantics.  The angle is expressed in degrees.
  void RotateX(double angle) { this->RotateWXYZ(angle, 1, 0, 0); };
  void RotateY(double angle) { this->RotateWXYZ(angle, 0, 1, 0); };
  void RotateZ(double angle) { this->RotateWXYZ(angle, 0, 0, 1); };

  // Description:
  // Create a scale matrix (i.e. set the diagonal elements to x, y, z)
  // and concatenate it with the current transformation according to
  // PreMultiply or PostMultiply semantics.
  void Scale(double x, double y, double z) {
    this->Concatenation->Scale(x,y,z); };
  void Scale(const double s[3]) { this->Scale(s[0], s[1], s[2]); };
  void Scale(const float s[3]) { this->Scale(s[0], s[1], s[2]); };

  // Description:
  // Set the current matrix directly.  This actually calls Identity(),
  // followed by Concatenate(matrix).
  void SetMatrix(vtkMatrix4x4 *matrix) { 
    this->SetMatrix(*matrix->Element); };
  void SetMatrix(const double elements[16]) { 
    this->Identity(); this->Concatenate(elements); };

  // Description:
  // Concatenates the matrix with the current transformation according
  // to PreMultiply or PostMultiply semantics.
  void Concatenate(vtkMatrix4x4 *matrix) { 
    this->Concatenate(*matrix->Element); };
  void Concatenate(const double elements[16]) {
    this->Concatenation->Concatenate(elements); };

  // Description:
  // Concatenate the specified transform with the current transformation
  // according to PreMultiply or PostMultiply semantics.
  // The concatenation is pipelined, meaning that if any of the
  // transformations are changed, even after Concatenate() is called,
  // those changes will be reflected when you call TransformPoint().
  void Concatenate(vtkHomogeneousTransform *transform);

  // Description:
  // Sets the internal state of the transform to PreMultiply. All subsequent
  // operations will occur before those already represented in the
  // current transformation.  In homogeneous matrix notation, M = M*A where
  // M is the current transformation matrix and A is the applied matrix.
  // The default is PreMultiply.
  void PreMultiply() { 
    if (this->Concatenation->GetPreMultiplyFlag()) { return; }
    this->Concatenation->SetPreMultiplyFlag(1); this->Modified(); };

  // Description:
  // Sets the internal state of the transform to PostMultiply. All subsequent
  // operations will occur after those already represented in the
  // current transformation.  In homogeneous matrix notation, M = A*M where
  // M is the current transformation matrix and A is the applied matrix.
  // The default is PreMultiply.
  void PostMultiply()  { 
    if (!this->Concatenation->GetPreMultiplyFlag()) { return; }
    this->Concatenation->SetPreMultiplyFlag(0); this->Modified(); };

  // Description:
  // Get the total number of transformations that are linked into this
  // one via Concatenate() operations or via SetInput().
  int GetNumberOfConcatenatedTransforms() {
    return this->Concatenation->GetNumberOfTransforms() + 
      (this->Input == NULL ? 0 : 1); };

  // Description
  // Get one of the concatenated transformations as a vtkAbstractTransform.
  // These transformations are applied, in series, every time the 
  // transformation of a coordinate occurs.  This method is provided
  // to make it possible to decompose a transformation into its
  // constituents, for example to save a transformation to a file.
  vtkHomogeneousTransform *GetConcatenatedTransform(int i)
    {
      vtkAbstractTransform *t;
      if (this->Input == NULL)
        {
        t=this->Concatenation->GetTransform(i);
        }
      else if (i < this->Concatenation->GetNumberOfPreTransforms())
        {
        t=this->Concatenation->GetTransform(i);
        }
      else if (i > this->Concatenation->GetNumberOfPreTransforms())
        {
        t=this->Concatenation->GetTransform(i-1);
        }
      else if (this->GetInverseFlag())
        {
        t=this->Input->GetInverse();
        }
      else
        {
        t=this->Input;
        }
      return static_cast<vtkHomogeneousTransform *>(t);
    }

  // Description:
  // Set the input for this transformation.  This will be used as the
  // base transformation if it is set.  This method allows you to build
  // a transform pipeline: if the input is modified, then this transformation
  // will automatically update accordingly.  Note that the InverseFlag,
  // controlled via Inverse(), determines whether this transformation
  // will use the Input or the inverse of the Input.
  void SetInput(vtkHomogeneousTransform *input);
  vtkHomogeneousTransform *GetInput() { return this->Input; };

  // Description:
  // Get the inverse flag of the transformation.  This controls
  // whether it is the Input or the inverse of the Input that
  // is used as the base transformation.  The InverseFlag is
  // flipped every time Inverse() is called.  The InverseFlag
  // is off when a transform is first created.
  int GetInverseFlag() {
    return this->Concatenation->GetInverseFlag(); };

  // Description:
  // Pushes the current transformation onto the transformation stack.
  void Push() { if (this->Stack == NULL) { 
                    this->Stack = vtkTransformConcatenationStack::New(); }
                this->Stack->Push(&this->Concatenation); 
                this->Modified(); };

  // Description:
  // Deletes the transformation on the top of the stack and sets the top 
  // to the next transformation on the stack.
  void Pop() { if (this->Stack == NULL) { return; }
               this->Stack->Pop(&this->Concatenation);
               this->Modified(); };

  // Description:
  // Make a new transform of the same type -- you are responsible for
  // deleting the transform when you are done with it.
  vtkAbstractTransform *MakeTransform();

  // Description:
  // Check for self-reference.  Will return true if concatenating
  // with the specified transform, setting it to be our inverse,
  // or setting it to be our input will create a circular reference.
  // CircuitCheck is automatically called by SetInput(), SetInverse(),
  // and Concatenate(vtkXTransform *).  Avoid using this function,
  // it is experimental.
  int CircuitCheck(vtkAbstractTransform *transform);

  // Description:
  // Override GetMTime to account for input and concatenation.
  unsigned long GetMTime();

protected:
  vtkPerspectiveTransform();
  ~vtkPerspectiveTransform();

  void InternalDeepCopy(vtkAbstractTransform *t);
  void InternalUpdate();

  vtkHomogeneousTransform *Input;
  vtkTransformConcatenation *Concatenation;
  vtkTransformConcatenationStack *Stack;

private:
  vtkPerspectiveTransform(const vtkPerspectiveTransform&);  // Not implemented
  void operator=(const vtkPerspectiveTransform&);  // Not implemented
};


#endif
