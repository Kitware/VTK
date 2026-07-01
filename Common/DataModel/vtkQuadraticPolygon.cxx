// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkQuadraticPolygon.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkQuadraticEdge.h"

#include <algorithm>
#include <vector>

namespace
{
//------------------------------------------------------------------------------
[[maybe_unused]] constexpr const char* Topology = R"(
   QuadraticPolygon topology:

              ...----n+4----4
              /              \
            ...               n+3
            /                   \
          ...                    3
           |                     |
          ...                   n+2
           |                     |
          n-1                    2
            \                   /
            2n-1              n+1
              \               /
               0------n------1
)";
}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkQuadraticPolygon);

//------------------------------------------------------------------------------
// Instantiate quadratic polygon.
vtkQuadraticPolygon::vtkQuadraticPolygon()
{
  this->Polygon = vtkSmartPointer<vtkPolygon>::New();
  this->Edge = vtkSmartPointer<vtkQuadraticEdge>::New();
  this->UseMVCInterpolation = true;
}

//------------------------------------------------------------------------------
vtkCell* vtkQuadraticPolygon::GetEdge(int edgeId)
{
  int numEdges = this->GetNumberOfEdges();

  edgeId = std::clamp(edgeId, 0, numEdges - 1);
  int p = (edgeId + 1) % numEdges;

  // load point id's
  this->Edge->PointIds->SetId(0, this->PointIds->GetId(edgeId));
  this->Edge->PointIds->SetId(1, this->PointIds->GetId(p));
  this->Edge->PointIds->SetId(2, this->PointIds->GetId(edgeId + numEdges));

  // load coordinates
  this->Edge->Points->SetPoint(0, this->Points->GetPoint(edgeId));
  this->Edge->Points->SetPoint(1, this->Points->GetPoint(p));
  this->Edge->Points->SetPoint(2, this->Points->GetPoint(edgeId + numEdges));

  return this->Edge;
}

//------------------------------------------------------------------------------
int vtkQuadraticPolygon::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& minDist2, double weights[])
{
  this->InitializePolygon();
  int result = this->Polygon->EvaluatePosition(x, closestPoint, subId, pcoords, minDist2, weights);
  vtkQuadraticPolygon::PermuteFromPolygon(this->GetNumberOfPoints(), weights);
  return result;
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::EvaluateLocation(
  int& subId, const double pcoords[3], double x[3], double* weights)
{
  this->InitializePolygon();
  this->Polygon->EvaluateLocation(subId, pcoords, x, weights);
  vtkQuadraticPolygon::PermuteFromPolygon(this->GetNumberOfPoints(), weights);
}

//------------------------------------------------------------------------------
int vtkQuadraticPolygon::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  this->InitializePolygon();
  return this->Polygon->CellBoundary(subId, pcoords, pts);
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  this->InitializePolygon();

  vtkDataArray* convertedCellScalars = cellScalars->NewInstance();
  vtkQuadraticPolygon::PermuteToPolygon(cellScalars, convertedCellScalars);

  this->Polygon->Contour(
    value, convertedCellScalars, locator, verts, lines, polys, inPd, outPd, inCd, cellId, outCd);

  convertedCellScalars->Delete();
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  this->InitializePolygon();

  vtkDataArray* convertedCellScalars = cellScalars->NewInstance();
  vtkQuadraticPolygon::PermuteToPolygon(cellScalars, convertedCellScalars);

  this->Polygon->Clip(
    value, convertedCellScalars, locator, polys, inPd, outPd, inCd, cellId, outCd, insideOut);

  convertedCellScalars->Delete();
}

//------------------------------------------------------------------------------
int vtkQuadraticPolygon::IntersectWithLine(
  const double* p1, const double* p2, double tol, double& t, double* x, double* pcoords, int& subId)
{
  this->InitializePolygon();
  return this->Polygon->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId);
}

//------------------------------------------------------------------------------
int vtkQuadraticPolygon::TriangulateLocalIds(int index, vtkIdList* ptIds)
{
  this->InitializePolygon();
  int result = this->Polygon->TriangulateLocalIds(index, ptIds);
  vtkQuadraticPolygon::ConvertFromPolygon(this->GetNumberOfPoints(), ptIds);
  return result;
}

//------------------------------------------------------------------------------
int vtkQuadraticPolygon::NonDegenerateTriangulate(vtkIdList* outTris)
{
  this->InitializePolygon();
  int result = this->Polygon->NonDegenerateTriangulate(outTris);
  vtkQuadraticPolygon::ConvertFromPolygon(this->GetNumberOfPoints(), outTris);
  return result;
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::InterpolateFunctions(const double x[3], double* weights)
{
  this->InitializePolygon();
  this->Polygon->SetUseMVCInterpolation(UseMVCInterpolation);
  this->Polygon->InterpolateFunctions(x, weights);
  vtkQuadraticPolygon::PermuteFromPolygon(this->GetNumberOfPoints(), weights);
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "UseMVCInterpolation: " << this->UseMVCInterpolation << "\n";
  os << indent << "Edge:\n";
  this->Edge->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Polygon:\n";
  this->Polygon->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
double vtkQuadraticPolygon::DistanceToPolygon(
  double x[3], int numPts, double* pts, double bounds[6], double closest[3])
{
  std::vector<double> convertedPts(numPts * 3);
  vtkQuadraticPolygon::PermuteToPolygon(numPts, pts, convertedPts.data());

  double result = vtkPolygon::DistanceToPolygon(x, numPts, convertedPts.data(), bounds, closest);

  return result;
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::ComputeCentroid(vtkIdTypeArray* ids, vtkPoints* p, double c[3])
{
  vtkNew<vtkPoints> convertedPts;
  vtkQuadraticPolygon::PermuteToPolygon(p, convertedPts);

  vtkNew<vtkIdTypeArray> convertedIds;
  vtkQuadraticPolygon::PermuteToPolygon(ids, convertedIds);

  vtkPolygon::ComputeCentroid(convertedIds, convertedPts, c);
}

//------------------------------------------------------------------------------
int vtkQuadraticPolygon::ParameterizePolygon(
  double* p0, double* p10, double& l10, double* p20, double& l20, double* n)
{
  this->InitializePolygon();
  return this->Polygon->ParameterizePolygon(p0, p10, l10, p20, l20, n);
}

//------------------------------------------------------------------------------
int vtkQuadraticPolygon::IntersectPolygonWithPolygon(int npts, double* pts, double bounds[6],
  int npts2, double* pts2, double bounds2[6], double tol2, double x[3])
{
  std::vector<double> convertedPts(npts * 3);

  vtkQuadraticPolygon::PermuteToPolygon(npts, pts, convertedPts.data());

  std::vector<double> convertedPts2(npts2 * 3);
  vtkQuadraticPolygon::PermuteToPolygon(npts2, pts2, convertedPts2.data());

  return vtkPolygon::IntersectPolygonWithPolygon(
    npts, convertedPts.data(), bounds, npts2, convertedPts2.data(), bounds2, tol2, x);
}

//------------------------------------------------------------------------------
int vtkQuadraticPolygon::IntersectConvex2DCells(
  vtkCell* cell1, vtkCell* cell2, double tol, double p0[3], double p1[3])
{
  vtkSmartPointer<vtkPolygon> convertedCell1 = nullptr;
  vtkSmartPointer<vtkPolygon> convertedCell2 = nullptr;

  vtkQuadraticPolygon* qp1 = vtkQuadraticPolygon::SafeDownCast(cell1);
  if (qp1)
  {
    convertedCell1 = vtkSmartPointer<vtkPolygon>::New();
    vtkQuadraticPolygon::PermuteToPolygon(cell1, convertedCell1);
  }

  vtkQuadraticPolygon* qp2 = vtkQuadraticPolygon::SafeDownCast(cell2);
  if (qp2)
  {
    convertedCell2 = vtkSmartPointer<vtkPolygon>::New();
    vtkQuadraticPolygon::PermuteToPolygon(cell2, convertedCell2);
  }

  return vtkPolygon::IntersectConvex2DCells((convertedCell1 ? convertedCell1 : cell1),
    (convertedCell2 ? convertedCell2 : cell2), tol, p0, p1);
}

//------------------------------------------------------------------------------
int vtkQuadraticPolygon::PointInPolygon(
  double x[3], int numPts, double* pts, double bounds[6], double* n)
{
  std::vector<double> convertedPts(numPts * 3);

  vtkQuadraticPolygon::PermuteToPolygon(numPts, pts, convertedPts.data());

  return vtkPolygon::PointInPolygon(x, numPts, convertedPts.data(), bounds, n);
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::GetPermutationFromPolygon(vtkIdType nb, vtkIdList* permutation)
{
  permutation->SetNumberOfIds(nb);
  for (vtkIdType i = 0; i < nb; i++)
  {
    permutation->SetId(i, ((i % 2) ? (i + nb) / 2 : i / 2));
  }
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteToPolygon(vtkIdType nbPoints, double* inPoints, double* outPoints)
{
  vtkNew<vtkIdList> permutation;
  vtkQuadraticPolygon::GetPermutationFromPolygon(nbPoints, permutation);

  for (vtkIdType i = 0; i < nbPoints; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      outPoints[3 * i + j] = inPoints[3 * permutation->GetId(i) + j];
    }
  }
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteToPolygon(vtkPoints* inPoints, vtkPoints* outPoints)
{
  vtkIdType nbPoints = inPoints->GetNumberOfPoints();

  vtkNew<vtkIdList> permutation;
  vtkQuadraticPolygon::GetPermutationFromPolygon(nbPoints, permutation);

  outPoints->SetNumberOfPoints(nbPoints);
  for (vtkIdType i = 0; i < nbPoints; i++)
  {
    outPoints->SetPoint(i, inPoints->GetPoint(permutation->GetId(i)));
  }
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteToPolygon(vtkIdTypeArray* inIds, vtkIdTypeArray* outIds)
{
  vtkIdType nbIds = inIds->GetNumberOfTuples();

  vtkNew<vtkIdList> permutation;
  vtkQuadraticPolygon::GetPermutationFromPolygon(nbIds, permutation);

  outIds->SetNumberOfTuples(nbIds);
  for (vtkIdType i = 0; i < nbIds; i++)
  {
    outIds->SetValue(i, inIds->GetValue(permutation->GetId(i)));
  }
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteToPolygon(vtkDataArray* inDataArray, vtkDataArray* outDataArray)
{
  vtkIdType nb = inDataArray->GetNumberOfTuples();

  vtkNew<vtkIdList> permutation;
  vtkQuadraticPolygon::GetPermutationFromPolygon(nb, permutation);

  outDataArray->SetNumberOfComponents(inDataArray->GetNumberOfComponents());
  outDataArray->SetNumberOfTuples(nb);
  inDataArray->GetTuples(permutation, outDataArray);
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteToPolygon(vtkCell* inCell, vtkCell* outCell)
{
  vtkIdType nbPoints = inCell->GetNumberOfPoints();

  vtkNew<vtkIdList> permutation;
  vtkQuadraticPolygon::GetPermutationFromPolygon(nbPoints, permutation);

  outCell->Points->SetNumberOfPoints(nbPoints);
  outCell->PointIds->SetNumberOfIds(nbPoints);

  for (vtkIdType i = 0; i < nbPoints; i++)
  {
    outCell->PointIds->SetId(i, inCell->PointIds->GetId(permutation->GetId(i)));
    outCell->Points->SetPoint(i, inCell->Points->GetPoint(permutation->GetId(i)));
  }
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::InitializePolygon()
{
  vtkQuadraticPolygon::PermuteToPolygon(this, this->Polygon);
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::GetPermutationToPolygon(vtkIdType nb, vtkIdList* permutation)
{
  permutation->SetNumberOfIds(nb);
  for (vtkIdType i = 0; i < nb; i++)
  {
    permutation->SetId(i, (i < nb / 2) ? (i * 2) : (i * 2 + 1 - nb));
  }
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteFromPolygon(vtkIdType nb, double* values)
{
  vtkNew<vtkIdList> permutation;
  vtkQuadraticPolygon::GetPermutationToPolygon(nb, permutation);

  std::vector<double> save(nb);
  for (vtkIdType i = 0; i < nb; i++)
  {
    save[i] = values[i];
  }
  for (vtkIdType i = 0; i < nb; i++)
  {
    values[i] = save[permutation->GetId(i)];
  }
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::ConvertFromPolygon(vtkIdType nb, vtkIdList* ids)
{
  vtkIdType nbIds = ids->GetNumberOfIds();

  vtkNew<vtkIdList> permutation;
  vtkQuadraticPolygon::GetPermutationFromPolygon(nb, permutation);

  vtkNew<vtkIdList> saveList;
  saveList->SetNumberOfIds(nbIds);
  ids->SetNumberOfIds(nbIds);

  for (vtkIdType i = 0; i < nbIds; i++)
  {
    saveList->SetId(i, ids->GetId(i));
  }
  for (vtkIdType i = 0; i < nbIds; i++)
  {
    ids->SetId(i, permutation->GetId(saveList->GetId(i)));
  }
}

//------------------------------------------------------------------------------
void vtkQuadraticPolygon::Derivatives(int vtkNotUsed(subId), const double vtkNotUsed(pcoords)[3],
  const double* vtkNotUsed(values), int vtkNotUsed(dim), double* vtkNotUsed(derivs))
{
}
VTK_ABI_NAMESPACE_END
