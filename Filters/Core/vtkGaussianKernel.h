/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGaussianKernel - a spherical Gaussian interpolation kernel

// .SECTION Description
// vtkGaussianKernel is an interpolation kernel that simply returns the
// weights for all points found in the sphere defined by radius R. The weights
// are computed as: exp(-(s*r/R)^2) where r is the distance from the point to be
// interpolated to a neighboring point within R. The sharpness s simply affects
// the rate of fall off of the Gaussian.

// .SECTION Caveats
// The weights are normalized sp that SUM(Wi) = 1. If a neighbor point p
// precisely lies on the point to be interpolated, then the interpolated
// point takes on the values associated with p.

// .SECTION See Also
// vtkPointInterpolator vtkInterpolationKernel vtkVoronoiKernel vtkSPHKernel
// vtkShepardKernel


#ifndef vtkGaussianKernel_h
#define vtkGaussianKernel_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkInterpolationKernel.h"

class vtkIdList;
class vtkDoubleArray;


class VTKFILTERSCORE_EXPORT vtkGaussianKernel : public vtkInterpolationKernel
{
public:
  // Description:
  // Standard methods for instantiation, obtaining type information, and printing.
  static vtkGaussianKernel *New();
  vtkTypeMacro(vtkGaussianKernel,vtkInterpolationKernel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Given a point x, compute interpolation weights associated with nearby
  // points. The method returns the number of nearby points N (i.e., the
  // neighborhood). Note that both the nearby points list pIds and the
  // weights array are of length N, are provided by the caller of the method,
  // and may be dynamically resized as necessary.
  virtual vtkIdType ComputeWeights(double x[3], vtkIdList *pIds,
                                   vtkDoubleArray *weights);

  // Description:
  // Specify the radius of the kernel. Points within this radius will be
  // used for interpolation. If no point is found, then the closest point
  // will be used.
  vtkSetClampMacro(Radius,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(Radius,double);

  // Description:
  // Set / Get the sharpness (i.e., falloff) of the Gaussian. By default
  // Sharpness=2. As the sharpness increases the effects of distant points
  // are reduced.
  vtkSetClampMacro(Sharpness,double,1,VTK_FLOAT_MAX);
  vtkGetMacro(Sharpness,double);

protected:
  vtkGaussianKernel();
  ~vtkGaussianKernel();

  double Radius;
  double Sharpness;

private:
  vtkGaussianKernel(const vtkGaussianKernel&);  // Not implemented.
  void operator=(const vtkGaussianKernel&);  // Not implemented.
};

#endif
