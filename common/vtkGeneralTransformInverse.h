/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransformInverse.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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
// .NAME vtkGeneralTransformInverse - inverse of a general transform
// .SECTION Description
// The vtkGeneralTransformInverse is a helper class for vtkGeneralTransform,
// you should avoid using it directly.   
// .SECTION see also
// vtkGeneralTransform


#ifndef __vtkGeneralTransformInverse_h
#define __vtkGeneralTransformInverse_h

#include "vtkGeneralTransform.h"
#include "vtkMutexLock.h"

class VTK_EXPORT vtkGeneralTransformInverse : public vtkGeneralTransform
{
public:
  static vtkGeneralTransformInverse *New();

  vtkTypeMacro(vtkGeneralTransformInverse,vtkGeneralTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the transform that you want this to be an inverse of.
  void SetInverse(vtkGeneralTransform *transform);
  vtkGeneralTransform *GetInverse();
 
  // Description:
  // Get the cached copy of the forward transform (i.e. the inverse
  // of the Inverse transform).  
  vtkGeneralTransform *GetTransform();

  // Description:
  // Set this transform to the identity transform.  Warning: this modifies
  // the OriginalTransform.
  void Identity();

  // Description:
  // Set this transform to its own inverse.  Warning: this modifies
  // the OriginalTransform.
  void Inverse();

  // Description:
  // Copy another transform into this one.  Warning: this modifies
  // the OriginalTransform.
  void DeepCopy(vtkGeneralTransform *transform);

  // Description:
  // Make another transform of the same type as the stored transform.
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Return the modified time of this transform.
  unsigned long GetMTime();

  // Description:
  // Update the inverse transform from the original.
  void Update();

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformPoint(const float in[3], float out[3]);

  // Description:
  // This will calculate the transformation as well as its derivative
  // without calling Update.  Meant for use only within other VTK
  // classes.
  void InternalTransformDerivative(const float in[3], float out[3],
				   float derivative[3][3]);

protected:
  vtkGeneralTransformInverse();
  ~vtkGeneralTransformInverse();
  vtkGeneralTransformInverse(const vtkGeneralTransformInverse&) {};
  void operator=(const vtkGeneralTransformInverse&) {};

  vtkGeneralTransform *VirtualGetInverse();

  int UpdateRequired;
  vtkMutexLock *UpdateMutex;

  vtkGeneralTransform *Transform;
};

#endif





