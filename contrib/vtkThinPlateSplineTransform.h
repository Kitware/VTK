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
// by a set of source and target landmarks. Any point on the mesh close to a
// source landmark will be moved to a place close to the corresponding target
// landmark. The points in between are interpolated smoothly using
// Bookstein's Thin Plate Spline algorithm.
// The inverse grid transform is calculated using an iterative method,
// and is several times more expensive than the forward transform.
// .SECTION see also
// vtkGridTransform vtkGeneralTransformConcatenation


#ifndef __vtkThinPlateSplineTransform_h
#define __vtkThinPlateSplineTransform_h

#include "vtkWarpTransform.h"
#include "vtkMutexLock.h"

#define VTK_RBF_R      0
#define VTK_RBF_R2LOGR 1

class VTK_EXPORT vtkThinPlateSplineTransform : public vtkWarpTransform
{
public:
  vtkTypeMacro(vtkThinPlateSplineTransform,vtkGeneralTransform);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkThinPlateSplineTransform *New();

  // Description: 
  // Specify the 'stiffness' of the spline. The default is 1.0.
  vtkGetMacro(Sigma,float);
  vtkSetMacro(Sigma,float);

  // Description:
  // Specify the radial basis function to use.
  void SetBasis(int basis);
  vtkGetMacro(Basis,int);
  void SetBasisToR() { this->SetBasis(VTK_RBF_R); };
  void SetBasisToR2LogR() { this->SetBasis(VTK_RBF_R2LOGR); };
  const char *GetBasisAsString();

  // Description:
  // Set the source landmarks for the warp.
  void SetSourceLandmarks(vtkPoints *source);
  vtkGetObjectMacro(SourceLandmarks,vtkPoints);

  // Description:
  // Set the target landmarks for the warp.
  void SetTargetLandmarks(vtkPoints *target);
  vtkGetObjectMacro(TargetLandmarks,vtkPoints);

  // Description:
  // Create an identity transformation.  This simply calls
  // SetSourceLandmarks(NULL), SetTargetLandmarks(NULL).
  void Identity();

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

  void ForwardTransformPoint(const float in[3], float out[3]);
  void ForwardTransformDerivative(const float in[3], float out[3],
					  float derivative[3][3]);
  void InverseTransformPoint(const float in[3], float out[3]);

  vtkThinPlateSplineTransform *ApproximateInverse;

  float Sigma;
  vtkPoints *SourceLandmarks;
  vtkPoints *TargetLandmarks;

//BTX
  // the radial basis function to use
  double (*RadialBasisFunction)(double r);
  double (*RadialBasisDerivative)(double r, double& dUdr);
//ETX
  int Basis;

  float InverseTolerance;

  int UpdateRequired;
  vtkTimeStamp UpdateTime;
  int NumberOfPoints;
  double **MatrixW;

  vtkMutexLock *UpdateMutex;
};

#endif





