/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEllipsoidalGaussianKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkEllipsoidalGaussianKernel - an ellipsoidal Gaussian interpolation kernel

// .SECTION Description
// vtkEllipsoidalGaussianKernel is an interpolation kernel that returns the
// weights for all points found in the ellipsoid defined by radius R in
// combination with local data (normals and/or scalars). For example, "pancake"
// weightings (the local normal parallel to the minimum ellisoidal axis); or
// "needle" weightings (the local normal parallel to the maximum ellipsoidal
// axis) are possible. (Note that spherical Gaussian weightings are more
// efficiently computed using vtkGaussianKernel.)
//
// The ellipsoidal Gaussian can be described by:
//
//     W(x) = S * exp( -( Sharpness^2 * ((rxy/E)**2 + z**2)/R**2) )
//
// where S is the local scalar value; E is a user-defined eccentricity factor
// that controls the elliptical shape of the splat; z is the distance of the
// current voxel sample point along the local normal N; and rxy is the
// distance to neigbor point x in the direction prependicular to N.

// .SECTION Caveats
// The weights are normalized so that SUM(Wi) = 1. If a neighbor point p
// precisely lies on the point to be interpolated, then the interpolated
// point takes on the values associated with p.

// .SECTION See Also
// vtkPointInterpolator vtkInterpolationKernel vtkGaussianKernel
// vtkVoronoiKernel vtkSPHKernel vtkShepardKernel


#ifndef vtkEllipsoidalGaussianKernel_h
#define vtkEllipsoidalGaussianKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkInterpolationKernel.h"
#include "vtkStdString.h" // For vtkStdString ivars

class vtkIdList;
class vtkDataArray;
class vtkDoubleArray;


class VTKFILTERSPOINTS_EXPORT vtkEllipsoidalGaussianKernel : public vtkInterpolationKernel
{
public:
  // Description:
  // Standard methods for instantiation, obtaining type information, and printing.
  static vtkEllipsoidalGaussianKernel *New();
  vtkTypeMacro(vtkEllipsoidalGaussianKernel,vtkInterpolationKernel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the kernel. Overload the superclass to set up scalars and
  // vectors.
  virtual void Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds,
                          vtkPointData *pd);

  // Description:
  // Given a point x, determine the points around x which form an
  // interpolation basis. The user must provide the vtkIdList pids, which will
  // be dynamically resized as necessary. The method returns the number of
  // points in the basis. Typically this method is called before
  // ComputeWeights().
  virtual vtkIdType ComputeBasis(double x[3], vtkIdList *pIds);

  // Description:
  // Given a point x, and a list of basis points pIds, compute interpolation
  // weights associated with these basis points.  Note that both the nearby
  // basis points list pIds and the weights array are of length numPts, are
  // provided by the caller of the method, and may be dynamically resized as
  // necessary. Typically this method is called after ComputeBasis(),
  // although advanced users can invoke ComputeWeights() and provide the
  // interpolation basis points pIds directly.
  virtual vtkIdType ComputeWeights(double x[3], vtkIdList *pIds,
                                   vtkDoubleArray *weights);

  // Description:
  // Specify whether vector values should be used to affect the shape
  // of the Gaussian distribution. By default this is on.
  vtkSetMacro(UseNormals,bool);
  vtkGetMacro(UseNormals,bool);
  vtkBooleanMacro(UseNormals,bool);

  // Description:
  // Specify the normals array name. Used to orient the ellipsoid.
  vtkSetMacro(NormalsArrayName,vtkStdString);
  vtkGetMacro(NormalsArrayName,vtkStdString);

  // Description:
  // Specify whether scalar values should be used to scale the weights.
  // By default this is off.
  vtkSetMacro(UseScalars,bool);
  vtkGetMacro(UseScalars,bool);
  vtkBooleanMacro(UseScalars,bool);

  // Description:
  // Specify the normals array name. Used to orient the ellipsoid.
  vtkSetMacro(ScalarsArrayName,vtkStdString);
  vtkGetMacro(ScalarsArrayName,vtkStdString);

  // Description:
  // Multiply the Gaussian splat distribution by this value. If UseScalars is
  // on and a scalar aray is provided, then the scalar value will be
  // multiplied by the ScaleFactor times the Gaussian function.
  vtkSetClampMacro(ScaleFactor,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(ScaleFactor,double);

  // Description:
  // Specify the radius of the kernel. Points within this radius will be
  // used for interpolation. If no point is found, then the closest point
  // will be used.
  vtkSetClampMacro(Radius,double,0.000001,VTK_FLOAT_MAX);
  vtkGetMacro(Radius,double);

  // Description:
  // Set / Get the sharpness (i.e., falloff) of the Gaussian. By default
  // Sharpness=2. As the sharpness increases the effects of distant points
  // are reduced.
  vtkSetClampMacro(Sharpness,double,1,VTK_FLOAT_MAX);
  vtkGetMacro(Sharpness,double);

  // Description:
  // Set / Get the eccentricity of the ellipsoidal Gaussian. A value=1.0
  // produces a spherical distribution. Values < 1 produce a needle like
  // distribution (in the direction of the normal); values > 1 produce a
  // pancake like distribution (orthogonal to the normal).
  vtkSetClampMacro(Eccentricity,double,0.000001,VTK_FLOAT_MAX);
  vtkGetMacro(Eccentricity,double);

protected:
  vtkEllipsoidalGaussianKernel();
  ~vtkEllipsoidalGaussianKernel();

  bool UseNormals;
  bool UseScalars;

  vtkStdString NormalsArrayName;
  vtkStdString ScalarsArrayName;

  double ScaleFactor;
  double Radius;
  double Sharpness;
  double Eccentricity;

  // Internal structure to reduce computation
  double F2, E2;
  vtkDataArray *NormalsArray;
  vtkDataArray *ScalarsArray;

  virtual void FreeStructures();

private:
  vtkEllipsoidalGaussianKernel(const vtkEllipsoidalGaussianKernel&);  // Not implemented.
  void operator=(const vtkEllipsoidalGaussianKernel&);  // Not implemented.
};

#endif
