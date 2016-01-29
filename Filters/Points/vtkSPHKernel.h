/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSPHKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSPHKernel - a SPH interpolation kernel

// .SECTION Description
// vtkSPHKernel is an interpolation kernel that uses a spherical h
// closest point to a point to be interpolated. A single weight ise returned
// with value=1.0.

// .SECTION Caveats
// In degenerate cases (where a point x is equidistance from more than one
// point) the kernel basis arbitrarily chooses one of the equidistant points.

// .SECTION See Also
// vtkInterpolationKernel vtkGaussianKernel vtkSPHKernel vtkShepardKernel


#ifndef vtkSPHKernel_h
#define vtkSPHKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkInterpolationKernel.h"

class vtkIdList;
class vtkDoubleArray;


class VTKFILTERSPOINTS_EXPORT vtkSPHKernel : public vtkInterpolationKernel
{
public:
  // Description:
  // Standard methods for instantiation, obtaining type information, and printing.
  static vtkSPHKernel *New();
  vtkTypeMacro(vtkSPHKernel,vtkInterpolationKernel);
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
  // basis points list pIds and the weights array are of length numPts, are
  // provided by the caller of the method, and may be dynamically resized as
  // necessary. Typically this method is called after ComputeBasis(),
  // although advanced users can invoke ComputeWeights() and provide the
  // interpolation basis points pIds directly.
  virtual vtkIdType ComputeWeights(double x[3], vtkIdList *pIds,
                                   vtkDoubleArray *weights);

protected:
  vtkSPHKernel();
  ~vtkSPHKernel();

private:
  vtkSPHKernel(const vtkSPHKernel&);  // Not implemented.
  void operator=(const vtkSPHKernel&);  // Not implemented.
};

#endif
