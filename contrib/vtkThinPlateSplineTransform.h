/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThinPlateSplineTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class 
             based on code from vtkThinPlateSplineMeshWarp.cxx
	     written by Tim Hutton.

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
// .NAME vtkThinPlateSplineTransform - a nonlinear warp transformation
// .SECTION Description
// vtkThinPlateSplineTransform describes a nonlinear warp transform defined
// by two sets of landmarks (vtkPoints). Any point on the mesh close to a
// source landmark will be moved to a place close to the corresponding target
// landmark. The points in between are interpolated smoothly using
// Bookstein's Thin Plate Spline algorithm.
//
// .SECTION see also
// vtkGeneralTransform


#ifndef __vtkThinPlateSplineTransform_h
#define __vtkThinPlateSplineTransform_h

#include "vtkGeneralTransform.h"
#include "vtkMutexLock.h"

class VTK_EXPORT vtkThinPlateSplineTransform : public vtkGeneralTransform
{
public:
  vtkTypeMacro(vtkThinPlateSplineTransform,vtkGeneralTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with Sigma=1.0
  static vtkThinPlateSplineTransform *New();

  // Description: 
  // Specify the 'stiffness' of the spline. The default of 1.0
  // should usually be fine.
  vtkGetMacro(Sigma,float);
  vtkSetMacro(Sigma,float);

  // Description:
  // Set the source landmarks for the warp.
  void SetSourceLandmarks(vtkPoints *source);
  vtkGetObjectMacro(SourceLandmarks,vtkPoints);

  // Description:
  // Set the target landmarks for the warp.
  void SetTargetLandmarks(vtkPoints *target);
  vtkGetObjectMacro(TargetLandmarks,vtkPoints);

  // Description:
  // Apply the transformation.
  void TransformPoint(const float in[3], float out[3]);
  void TransformPoint(const double in[3], double out[3]) {
    vtkGeneralTransform::TransformPoint(in, out); };

  // Description:
  // Apply the transform to a series of points.
  void TransformPoints(vtkPoints *inPts, vtkPoints *outPts);

  // Description:
  // Apply the transform to a series of normals (you must call
  // TransformPoints first to get the outPts).
  void TransformNormals(vtkPoints *inPts, vtkPoints *outPts,
			vtkNormals *inNms, vtkNormals *outNms);

  // Description:
  // Apply the transform to a series of vectors (you must call
  // TransformPoints first to get the outPts).
  void TransformVectors(vtkPoints *inPts, vtkPoints *outPts,
			vtkVectors *inVrs, vtkVectors *outVrs);

  // Description:
  // Create an identity transformation.  This simply calls
  // SetSourceLandmarks(NULL), SetTargetLandmarks(NULL).
  void Identity();

  // Description:
  // Invert the transformation.  The inverse transform is
  // calculated using an iterative method.
  void Inverse();

  // Description:
  // Set the tolerance for inverse transformation.
  // The default is 0.001.
  vtkSetMacro(InverseTolerance,float);
  vtkGetMacro(InverseTolerance,float);

  // Description:
  // Make another transform of the same type.
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Copy this transform from another of the same type.
  void DeepCopy(vtkGeneralTransform *transform);

  // Description:
  // Get the MTime.
  unsigned long GetMTime();

  // Description:
  // Prepare the transformation for application.
  void Update();

protected:
  vtkThinPlateSplineTransform();
  ~vtkThinPlateSplineTransform();
  vtkThinPlateSplineTransform(const vtkThinPlateSplineTransform&) {};
  void operator=(const vtkThinPlateSplineTransform&) {};

//BTX
  void ForwardTransformPoint(const float in[3], float out[3]);
  void TransformDerivatives(const float in[3], float out[3], float der[3][3]);
  void InverseTransformPoint(const float in[3], float out[3]);
//ETX

  vtkThinPlateSplineTransform *ApproximateInverse;

  float Sigma;
  vtkPoints *SourceLandmarks;
  vtkPoints *TargetLandmarks;

  int InverseFlag;
  float InverseTolerance;

  int UpdateRequired;
  vtkTimeStamp UpdateTime;
  int NumberOfPoints;
  double **MatrixW;

  vtkMutexLock *UpdateMutex;
};

#endif





