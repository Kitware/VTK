/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkGeneralTransform - allows operations on any transforms
// .SECTION Description
// vtkGeneralTransform is like vtkTransform and vtkPerspectiveTransform,
// but it will work with any vtkAbstractTransform as input.  It is
// not as efficient as the other two, however, because arbitrary
// transformations cannot be concatenated by matrix multiplication.
// Transform concatenation is simulated by passing each input point
// through each transform in turn.  
// .SECTION see also
// vtkTransform vtkPerspectiveTransform

#include "vtkAbstractTransform.h"

#ifndef __vtkGeneralTransform_h
#define __vtkGeneralTransform_h

class VTK_COMMON_EXPORT vtkGeneralTransform : public vtkAbstractTransform
{
public:
  static vtkGeneralTransform *New();

  vtkTypeMacro(vtkGeneralTransform,vtkAbstractTransform);
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
  void Concatenate(vtkAbstractTransform *transform);

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
  vtkAbstractTransform *GetConcatenatedTransform(int i) {
    if (this->Input == NULL) {
      return this->Concatenation->GetTransform(i); }
    else if (i < this->Concatenation->GetNumberOfPreTransforms()) {
      return this->Concatenation->GetTransform(i); }
    else if (i > this->Concatenation->GetNumberOfPreTransforms()) {
      return this->Concatenation->GetTransform(i-1); } 
    else if (this->GetInverseFlag()) {
      return this->Input->GetInverse(); }
    else {
      return this->Input; } };

  // Description:
  // Set the input for this transformation.  This will be used as the
  // base transformation if it is set.  This method allows you to build
  // a transform pipeline: if the input is modified, then this transformation
  // will automatically update accordingly.  Note that the InverseFlag,
  // controlled via Inverse(), determines whether this transformation
  // will use the Input or the inverse of the Input.
  void SetInput(vtkAbstractTransform *input);
  vtkAbstractTransform *GetInput() { return this->Input; };

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
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformPoint(const float in[3], float out[3]);
  void InternalTransformPoint(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation as well as its derivative
  // without calling Update.  Meant for use only within other VTK
  // classes.
  void InternalTransformDerivative(const float in[3], float out[3],
				   float derivative[3][3]);
  void InternalTransformDerivative(const double in[3], double out[3],
				   double derivative[3][3]);

  // Description:
  // Check for self-reference.  Will return true if concatenating
  // with the specified transform, setting it to be our inverse,
  // or setting it to be our input will create a circular reference.
  // CircuitCheck is automatically called by SetInput(), SetInverse(),
  // and Concatenate(vtkXTransform *).  Avoid using this function,
  // it is experimental.
  int CircuitCheck(vtkAbstractTransform *transform);

  // Description:
  // Make another transform of the same type.
  vtkAbstractTransform *MakeTransform();

  // Description:
  // Override GetMTime to account for input and concatenation.
  unsigned long GetMTime();

protected:
  vtkGeneralTransform();
  ~vtkGeneralTransform();
  vtkGeneralTransform(const vtkGeneralTransform&);
  void operator=(const vtkGeneralTransform&);

  void InternalDeepCopy(vtkAbstractTransform *t);
  void InternalUpdate();

  vtkAbstractTransform *Input;
  vtkTransformConcatenation *Concatenation;
  vtkTransformConcatenationStack *Stack;
};


#endif





