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
// vtkProjectionTransform describes the full range of linear and
// perspective transformations.  It is internally represented as
// a 4x4 matrix.  This class is useful for describing a camera-view
// of a scene.
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
  // Make a new transform of the same type.
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Creates an identity matrix and makes it the current transformation matrix.
  void Identity();

  // Description:
  // Invert the current transformation matrix.
  void Inverse();

  // Description:
  // Sets the internal state of the transform to post multiply. All
  // subsequent matrix operations will occur after those already represented
  // in the current transformation matrix.  The default is PreMultiply.
  void PostMultiply();

  // Description:
  // Sets the internal state of the transform to pre multiply. All subsequent
  // matrix operations will occur before those already represented in the
  // current transformation matrix.  The default is PreMultiply.
  void PreMultiply();

  // Description:
  // Set the current matrix directly.  This can be used in combination
  // with vtkCamera::GetPerspectiveTransformMatrix() to set up a
  // perspective transformation. 
  void SetMatrix(vtkMatrix4x4 *m);
  void SetMatrix(double Elements[16]);

  // Description:
  // Create a pipelined concatenation of two transforms.  
  static vtkPerspectiveTransform *Concatenate(vtkPerspectiveTransform *t1,
					      vtkPerspectiveTransform *t2) {
    return vtkPerspectiveTransform::Concatenate(t1,t2,0,0); };
  static vtkGeneralTransform *Concatenate(vtkGeneralTransform *t1,
					  vtkGeneralTransform *t2) {
    return vtkGeneralTransform::Concatenate(t1,t2,0,0); };

  // Description:
  // Create a pipelined concatenation of three transforms.  
  static vtkPerspectiveTransform *Concatenate(vtkPerspectiveTransform *t1,
					      vtkPerspectiveTransform *t2,
					      vtkPerspectiveTransform *t3) {
    return vtkPerspectiveTransform::Concatenate(t1,t2,t3,0); };
  static vtkGeneralTransform *Concatenate(vtkGeneralTransform *t1,
					  vtkGeneralTransform *t2,
					  vtkGeneralTransform *t3) {
    return vtkGeneralTransform::Concatenate(t1,t2,t3,0); };

  // Description:
  // Create a pipelined concatenation of four transforms.  
  static vtkPerspectiveTransform *Concatenate(vtkPerspectiveTransform *t1,
					      vtkPerspectiveTransform *t2,
					      vtkPerspectiveTransform *t3,
					      vtkPerspectiveTransform *t4) {
    return vtkPerspectiveTransform::Concatenate(t1,t2,t3,t4); };
  static vtkGeneralTransform *Concatenate(vtkGeneralTransform *t1,
					  vtkGeneralTransform *t2,
					  vtkGeneralTransform *t3,
					  vtkGeneralTransform *t4) {
    return vtkGeneralTransform::Concatenate(t1,t2,t3,t4); };

  // Description:
  // Concatenates the input matrix with the current transformation matrix.
  // The resulting matrix becomes the new current transformation matrix.
  // The setting of the PreMultiply flag determines whether the matrix
  // is PreConcatenated or PostConcatenated.
  void Concatenate(vtkMatrix4x4 *matrix);
  void Concatenate(double Elements[16]);

  // Description:
  // Make this transform a copy of the specified transform.
  void DeepCopy(vtkGeneralTransform *t);

protected:
  vtkProjectionTransform();
  ~vtkProjectionTransform();
  vtkProjectionTransform(const vtkProjectionTransform& t);
  void operator=(const vtkProjectionTransform&) {};

  int PreMultiplyFlag;
};


#endif
