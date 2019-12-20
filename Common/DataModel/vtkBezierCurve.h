/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierCurve.h

  Copyright (c) Kevin Tew
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBezierCurve
// .SECTION Description
// .SECTION See Also

#ifndef vtkBezierCurve_h
#define vtkBezierCurve_h

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
class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkBezierCurve : public vtkHigherOrderCurve
{
public:
  static vtkBezierCurve* New();
  vtkTypeMacro(vtkBezierCurve, vtkHigherOrderCurve);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetCellType() override { return VTK_BEZIER_CURVE; }
  void EvaluateLocationProjectedNode(
    int& subId, const vtkIdType point_id, double x[3], double* weights);
  void SetRationalWeightsFromPointData(vtkPointData* point_data, const vtkIdType numPts);
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;

  vtkDoubleArray* GetRationalWeights();

protected:
  vtkLine* GetApproximateLine(
    int subId, vtkDataArray* scalarsIn = nullptr, vtkDataArray* scalarsOut = nullptr) override;
  vtkBezierCurve();
  ~vtkBezierCurve() override;

  vtkNew<vtkDoubleArray> RationalWeights;

private:
  vtkBezierCurve(const vtkBezierCurve&) = delete;
  void operator=(const vtkBezierCurve&) = delete;
};

#endif // vtkBezierCurve_h
