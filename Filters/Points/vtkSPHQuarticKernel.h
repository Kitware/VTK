/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSPHQuarticKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSPHQuarticKernel - a quartic SPH interpolation kernel

// .SECTION Description
// vtkSPHQuarticKernel is an smooth particle hydrodynamics interpolation kernel as
// described by D.J. Price. This is a quartic formulation.
//
// .SECTION Caveats
// For more information see D.J. Price, Smoothed particle hydrodynamics and
// magnetohydrodynamics, J. Comput. Phys. 231:759-794, 2012. Especially
// equation 49.

// .SECTION Acknowledgments
// The following work has been generously supported by Altair Engineering
// and FluiDyna GmbH, Please contact Steve Cosgrove or Milos Stanic for
// more information.

// .SECTION See Also
// vtkSPHKernel vtkSPHInterpolator


#ifndef vtkSPHQuarticKernel_h
#define vtkSPHQuarticKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkSPHKernel.h"
#include <algorithm> // For std::min()

class vtkIdList;
class vtkDoubleArray;


class VTKFILTERSPOINTS_EXPORT vtkSPHQuarticKernel : public vtkSPHKernel
{
public:
  // Description:
  // Standard methods for instantiation, obtaining type information, and printing.
  static vtkSPHQuarticKernel *New();
  vtkTypeMacro(vtkSPHQuarticKernel,vtkSPHKernel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Compute weighting factor given a normalized distance from a sample point.
  virtual double ComputeFunctionWeight(const double d)
  {
    double tmp1 = 2.5 - std::min(d,2.5);
    double tmp2 = 1.5 - std::min(d,1.5);
    double tmp3 = 0.5 - std::min(d,0.5);
    return (tmp1*tmp1*tmp1*tmp1 - 5.0*tmp2*tmp2*tmp2*tmp2 +
            10.0*tmp3*tmp3*tmp3*tmp3);
  }

  // Description:
  // Compute weighting factor for derivative quantities given a normalized
  // distance from a sample point.
  virtual double ComputeGradientWeight(const double d)
  {
    double tmp1 = 2.5 - std::min(d,2.5);
    double tmp2 = 1.5 - std::min(d,1.5);
    double tmp3 = 0.5 - std::min(d,0.5);
    return (tmp1*tmp1*tmp1 - 5.0*tmp2*tmp2*tmp2 + 10.0*tmp3*tmp3*tmp3);
  }

protected:
  vtkSPHQuarticKernel();
  ~vtkSPHQuarticKernel();

private:
  vtkSPHQuarticKernel(const vtkSPHQuarticKernel&);  // Not implemented.
  void operator=(const vtkSPHQuarticKernel&);  // Not implemented.
};

#endif
