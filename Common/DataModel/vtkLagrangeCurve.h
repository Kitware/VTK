/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeCurve.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLagrangeCurve
// .SECTION Description
// .SECTION See Also

#ifndef vtkLagrangeCurve_h
#define vtkLagrangeCurve_h

#include "vtkCellType.h"              // For GetCellType.
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHigherOrderCurve.h"
#include "vtkNew.h"          // For member variable.
#include "vtkSmartPointer.h" // For member variable.

class vtkCellData;
class vtkDoubleArray;
class vtkIdList;
class vtkLine;
class vtkPointData;
class vtkPoints;
class vtkVector3d;
class vtkVector3i;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeCurve : public vtkHigherOrderCurve
{
public:
  static vtkLagrangeCurve* New();
  vtkTypeMacro(vtkLagrangeCurve, vtkHigherOrderCurve);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetCellType() override { return VTK_LAGRANGE_CURVE; }

  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

protected:
  vtkLine* GetApproximateLine(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) override;
  vtkLagrangeCurve();
  ~vtkLagrangeCurve() override;

private:
  vtkLagrangeCurve(const vtkLagrangeCurve&) = delete;
  void operator=(const vtkLagrangeCurve&) = delete;
};

#endif // vtkLagrangeCurve_h
