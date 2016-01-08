/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShepardKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkShepardKernel - a Shepard method interpolation kernel

// .SECTION Description
// vtkShepardKernel is an interpolation kernel that uses the method of
// Shepard to perform interpolation. The weights are computed as 1/r^p, where
// r is the distance to a neighbor point within the kernal radius R; and p
// (the power parameter) is a positive exponent (typically p=2).

// .SECTION Caveats
// The weights are normalized sp that SUM(Wi) = 1. If a neighbor point p
// precisely lies on the point to be interpolated, then the interpolated
// point takes on the values associated with p.

// .SECTION See Also
// vtkPointInterpolator vtkInterpolationKernel vtkGaussianKernel vtkSPHKernel
// vtkShepardKernel


#ifndef vtkShepardKernel_h
#define vtkShepardKernel_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkInterpolationKernel.h"

class vtkIdList;
class vtkDoubleArray;


class VTKFILTERSCORE_EXPORT vtkShepardKernel : public vtkInterpolationKernel
{
public:
  // Description:
  // Standard methods for instantiation, obtaining type information, and printing.
  static vtkShepardKernel *New();
  vtkTypeMacro(vtkShepardKernel,vtkInterpolationKernel);
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
  // Set / Get the power parameter p. By default p=2. Values (which must be
  // a positive, real value) != 2 may affect performance significantly.
  vtkSetClampMacro(PowerParameter,double,0.001,100);
  vtkGetMacro(PowerParameter,double);

protected:
  vtkShepardKernel();
  ~vtkShepardKernel();

  double Radius;
  double PowerParameter;

private:
  vtkShepardKernel(const vtkShepardKernel&);  // Not implemented.
  void operator=(const vtkShepardKernel&);  // Not implemented.
};

#endif
