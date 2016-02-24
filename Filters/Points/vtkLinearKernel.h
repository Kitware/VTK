/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLinearKernel - a linear interpolation kernel

// .SECTION Description
// vtkLinearKernel is an interpolation kernel that averages the contributions
// of all points in the basis.

// .SECTION See Also
// vtkInterpolationKernel vtkGaussianKernel vtkLinearKernel vtkShepardKernel


#ifndef vtkLinearKernel_h
#define vtkLinearKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkInterpolationKernel.h"

class vtkIdList;
class vtkDoubleArray;


class VTKFILTERSPOINTS_EXPORT vtkLinearKernel : public vtkInterpolationKernel
{
public:
  // Description:
  // Standard methods for instantiation, obtaining type information, and printing.
  static vtkLinearKernel *New();
  vtkTypeMacro(vtkLinearKernel,vtkInterpolationKernel);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // basis points list pIds and the weights array are provided by the caller
  // of the method, and may be dynamically resized as necessary. Typically
  // this method is called after ComputeBasis(), although advanced users can
  // invoke ComputeWeights() and provide the interpolation basis points pIds
  // directly.
  virtual vtkIdType ComputeWeights(double x[3], vtkIdList *pIds,
                                   vtkDoubleArray *weights);

  // Description:
  // Specify the radius of the kernel. Points within this radius will be used
  // for interpolation (if ComputeBasis() is used).
  vtkSetClampMacro(Radius,double,0.000001,VTK_FLOAT_MAX);
  vtkGetMacro(Radius,double);

protected:
  vtkLinearKernel();
  ~vtkLinearKernel();

  double Radius;

private:
  vtkLinearKernel(const vtkLinearKernel&);  // Not implemented.
  void operator=(const vtkLinearKernel&);  // Not implemented.
};

#endif
