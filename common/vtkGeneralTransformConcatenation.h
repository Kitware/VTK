/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransformConcatenation.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkGeneralTransformConcatenation - concatenation of general transforms
// .SECTION Description
// vtkGeneralTransformConcatenation is a special GeneralTransform which
// allows concatenation of heterogenous transform types.  The transforms
// are not actually concatenated, but this is simulated by passing each
// input point through each transform in turn.
// .SECTION see also
// vtkGeneralTransform


#ifndef __vtkGeneralTransformConcatenation_h
#define __vtkGeneralTransformConcatenation_h

#include "vtkGeneralTransform.h"

class VTK_EXPORT vtkGeneralTransformConcatenation : public vtkGeneralTransform
{
public:
  static vtkGeneralTransformConcatenation *New();

  vtkTypeMacro(vtkGeneralTransformConcatenation,vtkGeneralTransform);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Concatenate the current transform with the specified transform,
  // taking the PreMultiply flag into consideration.
  void Concatenate(vtkGeneralTransform *transform);

  // Description:
  // Set the order in which subsequent concatenations will be
  // applied.
  void PreMultiply();
  void PostMultiply();

//BTX
  // Description:
  // Apply the transformation to a coordinate.  You can use the same 
  // array to store both the input and output point.
  void TransformPoint(const float in[3], float out[3]);
  void TransformPoint(const double in[3], double out[3]);
//ETX

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
  // Create an identity transformation.
  void Identity();

  // Description:
  // Invert the transformation.
  void Inverse();

  // Description:
  // Make another transform of the same type.
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Copy another transformation into this one.
  void DeepCopy(vtkGeneralTransform *transform);

  // Description:
  // Return modified time of transformation.
  unsigned long GetMTime();

  // Description:
  // Update the concatenated transform.
  void Update();

protected:
  vtkGeneralTransformConcatenation();
  ~vtkGeneralTransformConcatenation();
  vtkGeneralTransformConcatenation(const vtkGeneralTransformConcatenation&) {};
  void operator=(const vtkGeneralTransformConcatenation&) {};

  int PreMultiplyFlag;
  int InverseFlag;

  int NumberOfTransforms;
  int MaxNumberOfTransforms;
  vtkGeneralTransform **TransformList;
  vtkGeneralTransform **InverseTransformList;
};

//BTX
//----------------------------------------------------------------------------
inline void vtkGeneralTransformConcatenation::PreMultiply()
{
  if (this->PreMultiplyFlag)
    {
    return;
    }
  this->PreMultiplyFlag = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
inline void vtkGeneralTransformConcatenation::PostMultiply()
{
  if (!this->PreMultiplyFlag)
    {
    return;
    }
  this->PreMultiplyFlag = 0;
  this->Modified();
}
//ETX

#endif





