/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrixToPerspectiveTransform.h
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

// .NAME vtkMatrixToPerspectiveTransform - convert a matrix to a transform
// .SECTION Description
// This is a very simple class which allows a vtkMatrix4x4 to be used in
// place of a vtkPerspectiveTransform or vtkGeneralTransform.  For example,
// if you use it as a proxy between a matrix and vtkTransformPolyDataFilter
// then any modifications to the matrix will automatically be reflected in
// the output of the filter.
// .SECTION See Also
// vtkMatrix4x4 vtkMatrixToLinearTransform vtkPerspectiveTransform 
// vtkPerspectiveTransformConcatenation 

#ifndef __vtkMatrixToPerspectiveTransform_h
#define __vtkMatrixToPerspectiveTransform_h

#include "vtkPerspectiveTransform.h"
#include "vtkMatrix4x4.h"

class VTK_EXPORT vtkMatrixToPerspectiveTransform : 
  public vtkPerspectiveTransform
{
 public:
  static vtkMatrixToPerspectiveTransform *New();
  vtkTypeMacro(vtkMatrixToPerspectiveTransform,vtkPerspectiveTransform);
  void PrintSelf (ostream& os, vtkIndent indent);

  // Description:
  // Set the matrix.  Calls to Identity() and Inverse() will modify
  // this matrix, calls to GetInverse() will modify a copy of this
  // matrix.  
  void SetMatrix(vtkMatrix4x4 *m);
  vtkMatrix4x4 *GetMatrix() { return this->GetMatrixPointer(); };
  void GetMatrix(vtkMatrix4x4 *m) { 
    this->vtkPerspectiveTransform::GetMatrix(m); }; 

  // Description:
  // Make the matrix into the identity matrix.
  void Identity();

  // Description:
  // Invert the matrix.
  void Inverse();

  // Description:
  // Make a new transform of the same type.
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Make this transform into a copy of the specified transform.
  void DeepCopy(vtkGeneralTransform *transform);

  // Description:
  // Get the MTime: this is the bit of magic that makes everything work.
  unsigned long GetMTime();

protected:
  vtkMatrixToPerspectiveTransform() {};
  ~vtkMatrixToPerspectiveTransform() {};
  vtkMatrixToPerspectiveTransform(const vtkMatrixToPerspectiveTransform&) {};
  void operator=(const vtkMatrixToPerspectiveTransform&) {};
};

#endif
