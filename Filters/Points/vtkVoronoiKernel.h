/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoronoiKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVoronoiKernel - a Voronoi interpolation kernel

// .SECTION Description
// vtkVoronoiKernel is an interpolation kernel that simply returns the
// closest point to a point to be interpolated. A single weight is returned
// with value=1.0.

// .SECTION Caveats
// In degenerate cases (where a point x is equidistance from more than one
// point) the kernel basis arbitrarily chooses one of the equidistant points.

// .SECTION See Also
// vtkInterpolationKernel vtkGaussianKernel vtkSPHKernel vtkShepardKernel


#ifndef vtkVoronoiKernel_h
#define vtkVoronoiKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkInterpolationKernel.h"

class vtkIdList;
class vtkDoubleArray;


class VTKFILTERSPOINTS_EXPORT vtkVoronoiKernel : public vtkInterpolationKernel
{
public:
  // Description:
  // Standard methods for instantiation, obtaining type information, and printing.
  static vtkVoronoiKernel *New();
  vtkTypeMacro(vtkVoronoiKernel,vtkInterpolationKernel);
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

protected:
  vtkVoronoiKernel();
  ~vtkVoronoiKernel();

private:
  vtkVoronoiKernel(const vtkVoronoiKernel&);  // Not implemented.
  void operator=(const vtkVoronoiKernel&);  // Not implemented.
};

#endif
