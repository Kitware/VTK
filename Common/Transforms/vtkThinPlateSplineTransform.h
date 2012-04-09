/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThinPlateSplineTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkThinPlateSplineTransform - a nonlinear warp transformation
// .SECTION Description
// vtkThinPlateSplineTransform describes a nonlinear warp transform defined
// by a set of source and target landmarks. Any point on the mesh close to a
// source landmark will be moved to a place close to the corresponding target
// landmark. The points in between are interpolated smoothly using
// Bookstein's Thin Plate Spline algorithm.
//
// To obtain a correct TPS warp, use the R2LogR kernel if your data is 2D, and
// the R kernel if your data is 3D. Or you can specify your own RBF. (Hence this
// class is more general than a pure TPS transform.) 
// .SECTION Caveats
// 1) The inverse transform is calculated using an iterative method,
// and is several times more expensive than the forward transform.
// 2) Whenever you add, subtract, or set points you must call Modified()
// on the vtkPoints object, or the transformation might not update.
// 3) Collinear point configurations (except those that lie in the XY plane)
// result in an unstable transformation.
// .SECTION see also
// vtkGridTransform vtkGeneralTransform


#ifndef __vtkThinPlateSplineTransform_h
#define __vtkThinPlateSplineTransform_h

#include "vtkWarpTransform.h"

#define VTK_RBF_CUSTOM 0
#define VTK_RBF_R      1
#define VTK_RBF_R2LOGR 2

class VTK_HYBRID_EXPORT vtkThinPlateSplineTransform : public vtkWarpTransform
{
public:
  vtkTypeMacro(vtkThinPlateSplineTransform,vtkWarpTransform);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkThinPlateSplineTransform *New();

  // Description: 
  // Specify the 'stiffness' of the spline. The default is 1.0.
  vtkGetMacro(Sigma,double);
  vtkSetMacro(Sigma,double);

  // Description:
  // Specify the radial basis function to use.  The default is
  // R2LogR which is appropriate for 2D. Use |R| (SetBasisToR) 
  // if your data is 3D. Alternatively specify your own basis function, 
  // however this will mean that the transform will no longer be a true 
  // thin-plate spline.
  void SetBasis(int basis);
  vtkGetMacro(Basis,int);
  void SetBasisToR() { this->SetBasis(VTK_RBF_R); };
  void SetBasisToR2LogR() { this->SetBasis(VTK_RBF_R2LOGR); };
  const char *GetBasisAsString();

//BTX
  // Description:
  // Set the radial basis function to a custom function.  You must
  // supply both the function and its derivative with respect to r.
  void SetBasisFunction(double (*U)(double r)) {
    if (this->BasisFunction == U) { return; }
    this->SetBasis(VTK_RBF_CUSTOM);
    this->BasisFunction = U; 
    this->Modified(); };
  void SetBasisDerivative(double (*dUdr)(double r, double &dU)) {
    this->BasisDerivative = dUdr; 
    this->Modified(); };    
//ETX

  // Description:
  // Set the source landmarks for the warp.  If you add or change the
  // vtkPoints object, you must call Modified() on it or the transformation
  // might not update.
  void SetSourceLandmarks(vtkPoints *source);
  vtkGetObjectMacro(SourceLandmarks,vtkPoints);

  // Description:
  // Set the target landmarks for the warp.  If you add or change the
  // vtkPoints object, you must call Modified() on it or the transformation
  // might not update.
  void SetTargetLandmarks(vtkPoints *target);
  vtkGetObjectMacro(TargetLandmarks,vtkPoints);

  // Description:
  // Get the MTime.
  unsigned long GetMTime();

  // Description:
  // Make another transform of the same type.
  vtkAbstractTransform *MakeTransform();

protected:
  vtkThinPlateSplineTransform();
  ~vtkThinPlateSplineTransform();

  // Description:
  // Prepare the transformation for application.
  void InternalUpdate();

  // Description:
  // This method does no type checking, use DeepCopy instead.
  void InternalDeepCopy(vtkAbstractTransform *transform);

  void ForwardTransformPoint(const float in[3], float out[3]);
  void ForwardTransformPoint(const double in[3], double out[3]);

  void ForwardTransformDerivative(const float in[3], float out[3],
                                  float derivative[3][3]);
  void ForwardTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]);

  double Sigma;
  vtkPoints *SourceLandmarks;
  vtkPoints *TargetLandmarks;

//BTX
  // the radial basis function to use
  double (*BasisFunction)(double r);
  double (*BasisDerivative)(double r, double& dUdr);
//ETX
  int Basis;

  int NumberOfPoints;
  double **MatrixW;
private:
  vtkThinPlateSplineTransform(const vtkThinPlateSplineTransform&);  // Not implemented.
  void operator=(const vtkThinPlateSplineTransform&);  // Not implemented.
};

#endif





