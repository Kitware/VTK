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
// .NAME vtkSPHKernel - a Voronoi interpolation kernel

// .SECTION Description
// vtkSPHKernel is an interpolation kernel that simply returns the
// closest point to a point to be interpolated. A single weight ise returned
// with value=1.0.

// .SECTION Caveats
// In degenerate cases (where a point x is equidistance from more than one
// point) the kernel basis arbitrarily chooses one of the equidistant points.

// .SECTION See Also
// vtkInterpolationKernel vtkGaussianKernel vtkSPHKernel vtkShepardKernel


#ifndef vtkSPHKernel_h
#define vtkSPHKernel_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkInterpolationKernel.h"

class vtkIdList;
class vtkDoubleArray;


class VTKFILTERSCORE_EXPORT vtkSPHKernel : public vtkInterpolationKernel
{
public:
  // Description:
  // Standard methods for instantiation, obtaining type information, and printing.
  static vtkSPHKernel *New();
  vtkTypeMacro(vtkSPHKernel,vtkInterpolationKernel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Given a point x, compute interpolation weights associated with nearby
  // points. The method returns the number of nearby points N (i.e., the
  // neighborhood). Note that both the nearby points list pIds and the
  // weights array are of length N, are provided by the caller of the method,
  // and may be dynamically resized as necessary.
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
