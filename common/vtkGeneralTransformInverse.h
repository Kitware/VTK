/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransformInverse.h
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
// .NAME vtkGeneralTransformInverse - inverse of a general transform
// .SECTION Description
// The vtkGeneralTransformInverse is a helper class for vtkGeneralTransform,
// you should avoid using it directly.   
// .SECTION see also
// vtkGeneralTransform


#ifndef __vtkGeneralTransformInverse_h
#define __vtkGeneralTransformInverse_h

#include "vtkGeneralTransform.h"

class VTK_EXPORT vtkGeneralTransformInverse : public vtkGeneralTransform
{
public:
  static vtkGeneralTransformInverse *New();

  vtkTypeMacro(vtkGeneralTransformInverse,vtkGeneralTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the transform that you want this to be an inverse of.
  virtual void SetOriginalTransform(vtkGeneralTransform *transform);
  vtkGetObjectMacro(OriginalTransform, vtkGeneralTransform);
 
  // Description:
  // Get the internal copy of the inverse transform, which will
  // be of the same type as the OriginalTransform.
  vtkGetObjectMacro(InverseTransform, vtkGeneralTransform);

  // Description:
  // Apply the transformation to a coordinate.  You can use the same 
  // array to store both the input and output point.
  void TransformPoint(const float in[3], float out[3]);
  void TransformPoint(const double in[3], double out[3]);

  // Description:
  // Apply the transformation to a series of points, and append the
  // results to outPts.  
  void TransformPoints(vtkPoints *inPts, vtkPoints *outPts);

  // Description:
  // Apply the transformation to a series of normals, and append the
  // results to outNms.  The outPts must have been calculated beforehand.
  // The inPts and outPts are required in order for nonlinear transformations
  // to be properly supported.
  void TransformNormals(vtkPoints *inPts, vtkPoints *outPts, 
			vtkNormals *inNms, vtkNormals *outNms);

  // Description:
  // Apply the transformation to a series of vectors, and append the
  // results to outVrs.  The outPts must have been calculated beforehand.
  // The inPts and outPts are required in order for nonlinear transformations
  // to be properly supported.
  void TransformVectors(vtkPoints *inPts, vtkPoints *outPts, 
			vtkVectors *inVrs, vtkVectors *outVrs);

  // Description:
  // Get the inverse of this transform (this returns the OriginalTransform).
  vtkGeneralTransform *GetInverse();

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
  // Make another transform of the same type as the OriginalTransform.
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Return the modified time of this transform.
  unsigned long GetMTime();

  // Description:
  // Update the inverse transform from the original.
  void Update();

  // Description:
  // Needs a special UnRegister() implementation to avoid
  // circular references.
  void UnRegister(vtkObject * o);

protected:
  vtkGeneralTransformInverse();
  ~vtkGeneralTransformInverse();
  vtkGeneralTransformInverse(const vtkGeneralTransformInverse&) {};
  void operator=(const vtkGeneralTransformInverse&) {};

  int UpdateRequired;
  vtkGeneralTransform *OriginalTransform;
  vtkGeneralTransform *InverseTransform;
};

#endif





