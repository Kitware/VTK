/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLandmarkTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Tim Hutton and David G. Gobbi who developed this class.

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
// .NAME vtkLandmarkTransform - a linear transform specified by two corresponding point sets
// .SECTION Description
// A vtkLandmarkTransform is defined by two sets of landmarks, the 
// transform computed gives the best fit mapping one onto the other, in a 
// least squares sense. The indices are taken to correspond, so point 1 
// in the first set will get mapped close to point 1 in the second set, 
// etc. Call SetSourceLandmarks and SetTargetLandmarks to specify the two
// sets of landmarks, ensure they have the same number of points.
// .SECTION Caveats
// Whenever you add, subtract, or set points you must call Modified()
// on the vtkPoints object, or the transformation might not update.
// .SECTION see also
// vtkLinearTransform

#ifndef __vtkLandmarkTransform_h
#define __vtkLandmarkTransform_h

#include "vtkLinearTransform.h"
#include "vtkMutexLock.h"

#define VTK_LANDMARK_RIGIDBODY 6
#define VTK_LANDMARK_SIMILARITY 7
#define VTK_LANDMARK_AFFINE 12

class VTK_EXPORT vtkLandmarkTransform : public vtkLinearTransform
{
public:
  static vtkLandmarkTransform *New();

  vtkTypeMacro(vtkLandmarkTransform,vtkLinearTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the source and target landmark sets. The two sets must have 
  // the same number of points.  If you add or change points in these objects,
  // you must call Modified() on them or the transformation might not update.
  void SetSourceLandmarks(vtkPoints *points);
  void SetTargetLandmarks(vtkPoints *points);
  vtkGetObjectMacro(SourceLandmarks, vtkPoints);
  vtkGetObjectMacro(TargetLandmarks, vtkPoints);

  // Description:
  // Set the number of degrees of freedom to constrain the solution to.
  // Rigidbody: rotation and translation only.  
  // Similarity: rotation, translation and isotropic scaling.
  // The default is similarity.
  vtkSetMacro(Mode,int);
  void SetModeToRigidBody() { this->SetMode(VTK_LANDMARK_RIGIDBODY); };
  void SetModeToSimilarity() { this->SetMode(VTK_LANDMARK_SIMILARITY); };
  void SetModeToAffine() { this->SetMode(VTK_LANDMARK_AFFINE); };
  vtkGetMacro(Mode,int);
  const char *GetModeAsString();

  // Description:
  // Invert the transformation.  This is done by switching the
  // source and target landmarks.
  void Inverse();

  // Description:
  // Get the MTime.
  unsigned long GetMTime();

  // Description:
  // Make another transform of the same type.
  vtkAbstractTransform *MakeTransform();

protected:
  vtkLandmarkTransform();
  ~vtkLandmarkTransform();
  vtkLandmarkTransform(const vtkLandmarkTransform&) {};
  void operator=(const vtkLandmarkTransform&) {};

  // Update the matrix from the quaternion.
  void InternalUpdate();

  // Description:
  // This method does no type checking, use DeepCopy instead.
  void InternalDeepCopy(vtkAbstractTransform *transform);

  vtkPoints* SourceLandmarks;
  vtkPoints* TargetLandmarks;

  int Mode;
};
 
//BTX
inline const char *vtkLandmarkTransform::GetModeAsString()
{
  switch (this->Mode)
    {
    case VTK_LANDMARK_RIGIDBODY:
      return "RigidBody";
    case VTK_LANDMARK_SIMILARITY:
      return "Similarity";
    case VTK_LANDMARK_AFFINE:
      return "Affine";
    default:
      return "Unrecognized";
    }
}
//ETX
#endif
