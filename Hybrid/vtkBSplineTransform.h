/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkBSplineTransform.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBSplineTransform - a cubic b-spline deformation transformation
// .SECTION Description
// vtkBSplineTransform computes a cubic b-spline transformation from a
// grid of b-spline coefficients.
// .SECTION Caveats
// The inverse grid transform is calculated using an iterative method,
// and is several times more expensive than the forward transform.
// .SECTION see also
// vtkGeneralTransform vtkTransformToGrid vtkImageBSplineCoefficients
// .SECTION Thanks
// This class was written by David Gobbi at the Seaman Family MR Research
// Centre, Foothills Medical Centre, Calgary, Alberta.
// DG Gobbi and YP Starreveld,
// "Uniform B-Splines for the VTK Imaging Pipeline,"
// VTK Journal, 2011,
// http://hdl.handle.net/10380/3252

#ifndef __vtkBSplineTransform_h
#define __vtkBSplineTransform_h

#include "vtkWarpTransform.h"

class vtkImageData;

#define VTK_BSPLINE_EDGE 0
#define VTK_BSPLINE_ZERO 1
#define VTK_BSPLINE_ZERO_AT_BORDER 2

class VTK_HYBRID_EXPORT vtkBSplineTransform : public vtkWarpTransform
{
public:
  static vtkBSplineTransform *New();
  vtkTypeMacro(vtkBSplineTransform,vtkWarpTransform);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the coefficient grid for the b-spline transform.
  // The vtkBSplineTransform class will never modify the data.
  virtual void SetCoefficients(vtkImageData*);
  vtkGetObjectMacro(Coefficients,vtkImageData);

  // Description:
  // Set/Get a scale to apply to the transformation.
  vtkSetMacro(DisplacementScale,double);

  // Description:
  // Set/Get the border mode, to alter behavior at the edge of the grid.
  // The Edge mode allows the displacement to converge to the edge
  // coefficient past the boundary, which is similar to the behavior of
  // the vtkGridTransform. The Zero mode allows the displacement to
  // smoothly converge to zero two node-spacings past the boundary,
  // which is useful when you want to create a localized transform.
  // The ZeroAtBorder mode sacrifices smoothness to further localize
  // the transform to just one node-spacing past the boundary.
  vtkSetClampMacro(BorderMode, int,
    VTK_BSPLINE_EDGE, VTK_BSPLINE_ZERO_AT_BORDER);
  void SetBorderModeToEdge() {
    this->SetBorderMode(VTK_BSPLINE_EDGE); }
  void SetBorderModeToZero() {
    this->SetBorderMode(VTK_BSPLINE_ZERO); }
  void SetBorderModeToZeroAtBorder() {
    this->SetBorderMode(VTK_BSPLINE_ZERO_AT_BORDER); }
  vtkGetMacro(BorderMode, int);
  const char *GetBorderModeAsString();

  // Description:
  // Make another transform of the same type.
  vtkAbstractTransform *MakeTransform();

  // Description:
  // Get the MTime.
  unsigned long GetMTime();

protected:
  vtkBSplineTransform();
  ~vtkBSplineTransform();

  // Description:
  // Update the displacement grid.
  void InternalUpdate();

  // Description:
  // Copy this transform from another of the same type.
  void InternalDeepCopy(vtkAbstractTransform *transform);

  // Description:
  // Internal functions for calculating the transformation.
  void ForwardTransformPoint(const float in[3], float out[3]);
  void ForwardTransformPoint(const double in[3], double out[3]);

  void ForwardTransformDerivative(const float in[3], float out[3],
                                  float derivative[3][3]);
  void ForwardTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]);

  void InverseTransformPoint(const float in[3], float out[3]);
  void InverseTransformPoint(const double in[3], double out[3]);

  void InverseTransformDerivative(const float in[3], float out[3],
                                  float derivative[3][3]);
  void InverseTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]);

//BTX
  void (*CalculateSpline)(const double point[3], double displacement[3],
                          double derivatives[3][3],
                          void *gridPtr, int inExt[6], vtkIdType inInc[3],
                          int borderMode);
//ETX

  vtkImageData *Coefficients;
  double DisplacementScale;
  int BorderMode;

  void *GridPointer;
  double GridSpacing[3];
  double GridOrigin[3];
  int GridExtent[6];
  vtkIdType GridIncrements[3];

private:
  vtkBSplineTransform(const vtkBSplineTransform&);  // Not implemented.
  void operator=(const vtkBSplineTransform&);  // Not implemented.
};

#endif
