/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectionTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

// .NAME vtkProjectionTransform - describes a 4x4 matrix transformation
// .SECTION Description
// A vtkProjectionTransform can be used to describe the full range of
// perspective transformations.  It was designed in particular
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
// .SECTION See Also
// vtkPerspectiveTransformConcatenation vtkTransform vtkMatrix4x4 vtkCamera

#ifndef __vtkProjectionTransform_h
#define __vtkProjectionTransform_h

#include "vtkPerspectiveTransform.h"

class VTK_EXPORT vtkProjectionTransform : public vtkPerspectiveTransform
{
 public:
  static vtkProjectionTransform *New();
  vtkTypeMacro(vtkProjectionTransform,vtkPerspectiveTransform);
  void PrintSelf (ostream& os, vtkIndent indent);

  // Description:
  // Set this tranformation to the identity transformation.
  void Identity() { this->Concatenation->Identity(); this->Modified(); };

  // Description:
  // Invert the transformation.
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
  // Create an orthogonal projection matrix and multiply it by the
  // current matrix.  The matrix maps [xmin,xmax], [ymin,ymax], 
  // [-znear,-zfar] to [-1,+1], [-1,+1], [+1,-1]. 
  void Ortho(double xmin, double xmax, double ymin, double ymax, 
	     double znear, double zfar);

  // Description:
  // Create an perspective projection matrix and multiply it by the
  // current matrix.  The matrix maps a frustum with a back plane at -zfar,
  // and a front plane at -znear with extent [xmin,xmax],[ymin,ymax] 
  // to [-1,+1], [-1,+1], [+1,-1].
  void Frustum(double xmin, double xmax, double ymin, double ymax, 
	       double znear, double zfar);

  // Description:
  // Create a perspective projection matrix by specifying the view angle
  // (this angle is in the y direction), the aspect ratio, and the near 
  // and far clipping range.  The projection matrix is concatenated 
  // with the current matrix.  This method works via Frustum.
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
  // current matrix.  This can be applied in conjunction with either a 
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
  // matrix.
  void SetupCamera(const double position[3], const double focalpoint[3],
		   const double viewup[3]);

  // Description:
  // Create a translation matrix and concatenate it with the current
  // matrix according to PreMultiply or PostMultiply semantics.
  void Translate(double x, double y, double z) {
    this->Concatenation->Translate(x,y,z); };
  void Translate(const double x[3]) { this->Translate(x[0], x[1], x[2]); };
  void Translate(const float x[3]) { this->Translate(x[0], x[1], x[2]); };

  // Description:
  // Create a rotation matrix and concatenate it with the current matrix
  // according to PreMultiply or PostMultiply semantics.
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
  // it with the current matrix according to PreMultiply or PostMultiply
  // semantics.  The angle is expressed in degrees.
  void RotateX(double angle) { this->RotateWXYZ(angle, 1, 0, 0); };
  void RotateY(double angle) { this->RotateWXYZ(angle, 0, 1, 0); };
  void RotateZ(double angle) { this->RotateWXYZ(angle, 0, 0, 1); };

  // Description:
  // Create a scale matrix (i.e. set the diagonal elements to x, y, z)
  // and concatenate it with the current matrix according to PreMultiply 
  // or PostMultiply semantics.
  void Scale(double x, double y, double z) {
    this->Concatenation->Scale(x,y,z); };
  void Scale(const double s[3]) { this->Scale(s[0], s[1], s[2]); };
  void Scale(const float s[3]) { this->Scale(s[0], s[1], s[2]); };

  // Description:
  // Set the current matrix directly.  This can be used in combination
  // with vtkCamera::GetPerspectiveTransformMatrix() to copy a camera's
  // perspective transform.
  void SetMatrix(vtkMatrix4x4 *matrix) { 
    this->SetMatrix(*matrix->Element); };
  void SetMatrix(const double elements[16]) {
    this->Identity(); this->Concatenate(elements); };

  // Description:
  // Concatenates the input matrix with the current transformation matrix.
  // The resulting matrix becomes the new current transformation matrix.
  // The setting of the PreMultiply flag determines whether the matrix
  // is PreConcatenated or PostConcatenated.
  void Concatenate(vtkMatrix4x4 *matrix) { 
    this->Concatenate(*matrix->Element); };
  void Concatenate(const double elements[16]) {
    this->Concatenation->Concatenate(elements); };

  // Description:
  // Concatenates the current transformation with the specified matrix.
  // The setting of the PreMultiply flag determines whether the matrix
  // is PreConcatenated or PostConcatenated.
  void Concatenate(vtkPerspectiveTransform *transform);

  // Description:
  // Sets the internal state of the transform to pre multiply. All subsequent
  // matrix operations will occur before those already represented in the
  // current transformation matrix.  The default is PreMultiply.
  void PreMultiply() { 
    if (this->Concatenation->GetPreMultiplyFlag()) { return; }
    this->Concatenation->SetPreMultiplyFlag(1); this->Modified(); };

  // Description:
  // Sets the internal state of the transform to post multiply. All
  // subsequent matrix operations will occur after those already represented
  // in the current transformation matrix.  The default is PreMultiply.
  void PostMultiply()  { 
    if (!this->Concatenation->GetPreMultiplyFlag()) { return; }
    this->Concatenation->SetPreMultiplyFlag(0); this->Modified(); };

  // Description:
  // Pushes the current transformation matrix onto the transformation stack.
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
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Set the input for this transformation.  Any transformations you
  // apply will be concatenated with the input.
  void SetInput(vtkPerspectiveTransform *input);
  vtkPerspectiveTransform *GetInput() { return this->Input; };

  // Description:
  // Check for self-reference.  Will return true if concatenating
  // with the specified transform, setting it to be our inverse,
  // or setting it to be our input will create a circular reference.
  // CircuitCheck is automatically called by SetInput(), SetInverse(),
  // and Concatenate(vtkXTransform *).  Avoid using this function,
  // it is experimental.
  int CircuitCheck(vtkGeneralTransform *transform);

  // Description:
  // Override GetMTime to account for input and concatenation.
  unsigned long GetMTime();

protected:
  vtkProjectionTransform();
  ~vtkProjectionTransform();
  vtkProjectionTransform(const vtkProjectionTransform& t);
  void operator=(const vtkProjectionTransform&) {};

  void InternalDeepCopy(vtkGeneralTransform *t);
  void InternalUpdate();

  vtkPerspectiveTransform *Input;
  vtkTransformConcatenation *Concatenation;
  vtkTransformConcatenationStack *Stack;
};


#endif
