/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThinPlateSplineTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class 
             based on code from vtkThinPlateSplineMeshWarp.cxx
	     written Tim Hutton.

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
// .NAME vtkThinPlateSplineTransform - a nonlinear warp transformation
// .SECTION Description
// vtkThinPlateSplineTransform describes a nonlinear warp transform defined
// by two sets of landmarks (vtkPoints). Any point on the mesh close to a
// source landmark will be moved to a place close to the corresponding target
// landmark. The points in between are interpolated smoothly using
// Bookstein's Thin Plate Spline algorithm.
//
// The filter takes three inputs: the polygonal mesh to be warped (use
// SetInput), the source landmarks (SetSourceLandmarks) and the target
// Landmarks (SetTargetLandmarks).  There is one parameter (Sigma) that
// controls the 'stiffness' of the spline (default is 1.0).
//
// The topology of the mesh is not altered, only the geometry (the location
// of the points). 
// .SECTION see also
// vtkGridTransform


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
  // Set the target landmarks for the warp
  void SetTargetLandmarks(vtkPoints *target);
  vtkGetObjectMacro(TargetLandmarks,vtkPoints);

  // Description:
  // Apply the transformation.
  void TransformPoint(const float in[3], float out[3]);
  void TransformPoint(const double in[3], double out[3])
    {
    vtkGeneralTransform::TransformPoint(in, out);  
    }
  ;

  // Description:
  // Apply the transform to a series of points.
  void TransformPoints(vtkPoints *inPts, vtkPoints *outPts);

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
  vtkMutexLock *MatrixWMutex;
};

#endif





