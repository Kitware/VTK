/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerspectiveTransformInverse.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkPerspectiveTransformInverse - inverse of a perspective transform
// .SECTION Description
// The vtkPerspectiveTransformInverse is a helper class for 
// vtkPerspectiveTransform, you should avoid using it directly.   
// .SECTION see also
// vtkGeneralTransformInverse vtkLinearTransformInverse


#ifndef __vtkPerspectiveTransformInverse_h
#define __vtkPerspectiveTransformInverse_h

#include "vtkPerspectiveTransform.h"
#include "vtkMutexLock.h"

class VTK_EXPORT vtkPerspectiveTransformInverse : public vtkPerspectiveTransform
{
public:
  static vtkPerspectiveTransformInverse *New();

  vtkTypeMacro(vtkPerspectiveTransformInverse,vtkPerspectiveTransform);
  void PrintSelf(vtkOstream& os, vtkIndent indent);

  // Description:
  // Set the transform that you want this to be an inverse of.
  void SetInverse(vtkPerspectiveTransform *transform);
  vtkGeneralTransform *GetInverse();
 
  // Description:
  // Get the cached copy of the forward transform (i.e. the inverse
  // of the Inverse transform).  
  vtkPerspectiveTransform *GetTransform();

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

protected:
  vtkPerspectiveTransformInverse();
  ~vtkPerspectiveTransformInverse();
  vtkPerspectiveTransformInverse(const vtkPerspectiveTransformInverse&) {};
  void operator=(const vtkPerspectiveTransformInverse&) {};

  int UpdateRequired;
  vtkMutexLock *UpdateMutex;
  vtkPerspectiveTransform *Transform;
};

#endif





