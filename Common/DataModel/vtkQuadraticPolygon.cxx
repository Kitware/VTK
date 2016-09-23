/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticPolygon.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuadraticPolygon.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkQuadraticEdge.h"

vtkStandardNewMacro(vtkQuadraticPolygon);

//----------------------------------------------------------------------------
// Instantiate quadratic polygon.
vtkQuadraticPolygon::vtkQuadraticPolygon()
{
  this->Polygon = vtkPolygon::New();
  this->Edge = vtkQuadraticEdge::New();
  this->UseMVCInterpolation = true;
}

//----------------------------------------------------------------------------
vtkQuadraticPolygon::~vtkQuadraticPolygon()
{
  this->Polygon->Delete();
  this->Edge->Delete();
}

//----------------------------------------------------------------------------
vtkCell *vtkQuadraticPolygon::GetEdge(int edgeId)
{
  int numEdges = this->GetNumberOfEdges();

  edgeId = (edgeId < 0 ? 0 : (edgeId > numEdges - 1 ? numEdges - 1 : edgeId ));
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

//----------------------------------------------------------------------------
int vtkQuadraticPolygon::EvaluatePosition(double* x,
                                          double* closestPoint,
                                          int& subId, double pcoords[3],
                                          double& minDist2, double *weights)
{
  this->InitializePolygon();
  int result = this->Polygon->EvaluatePosition(x, closestPoint, subId, pcoords,
                                               minDist2, weights);
  vtkQuadraticPolygon::PermuteFromPolygon(this->GetNumberOfPoints(), weights);
  return result;
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::EvaluateLocation(int& subId, double pcoords[3],
                                           double x[3], double *weights)
{
  this->InitializePolygon();
  this->Polygon->EvaluateLocation(subId, pcoords, x, weights);
  vtkQuadraticPolygon::PermuteFromPolygon(this->GetNumberOfPoints(), weights);
}

//----------------------------------------------------------------------------
int vtkQuadraticPolygon::CellBoundary(int subId, double pcoords[3],
                                      vtkIdList *pts)
{
  this->InitializePolygon();
  return this->Polygon->CellBoundary(subId, pcoords, pts);
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::Contour(double value,
                                  vtkDataArray* cellScalars,
                                  vtkIncrementalPointLocator* locator,
                                  vtkCellArray *verts,
                                  vtkCellArray* lines,
                                  vtkCellArray* polys,
                                  vtkPointData* inPd,
                                  vtkPointData* outPd,
                                  vtkCellData* inCd,
                                  vtkIdType cellId,
                                  vtkCellData* outCd)
{
  this->InitializePolygon();

  vtkDataArray *convertedCellScalars = cellScalars->NewInstance();
  vtkQuadraticPolygon::PermuteToPolygon(cellScalars, convertedCellScalars);

  this->Polygon->Contour(value, convertedCellScalars, locator, verts, lines,
                         polys, inPd, outPd, inCd, cellId, outCd);

  convertedCellScalars->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::Clip(double value, vtkDataArray* cellScalars,
                               vtkIncrementalPointLocator* locator,
                               vtkCellArray* polys,
                               vtkPointData* inPd, vtkPointData* outPd,
                               vtkCellData* inCd, vtkIdType cellId,
                               vtkCellData* outCd, int insideOut)
{
  this->InitializePolygon();

  vtkDataArray *convertedCellScalars = cellScalars->NewInstance();
  vtkQuadraticPolygon::PermuteToPolygon(cellScalars, convertedCellScalars);

  this->Polygon->Clip(value, convertedCellScalars, locator, polys, inPd, outPd,
                      inCd, cellId, outCd, insideOut);

  convertedCellScalars->Delete();
}

//----------------------------------------------------------------------------
int vtkQuadraticPolygon::IntersectWithLine(double* p1,
                                           double* p2,
                                           double tol,
                                           double& t,
                                           double* x,
                                           double* pcoords,
                                           int& subId)
{
  this->InitializePolygon();
  return this->Polygon->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId);
}

//----------------------------------------------------------------------------
int vtkQuadraticPolygon::Triangulate(vtkIdList *outTris)
{
  this->InitializePolygon();
  int result = this->Polygon->Triangulate(outTris);
  vtkQuadraticPolygon::ConvertFromPolygon(outTris);
  return result;
}

//----------------------------------------------------------------------------
int vtkQuadraticPolygon::Triangulate(int index, vtkIdList *ptIds,
                                     vtkPoints *pts)
{
  this->InitializePolygon();
  return this->Polygon->Triangulate(index, ptIds, pts);
}

//----------------------------------------------------------------------------
int vtkQuadraticPolygon::NonDegenerateTriangulate(vtkIdList *outTris)
{
  this->InitializePolygon();
  int result = this->Polygon->NonDegenerateTriangulate(outTris);
  vtkQuadraticPolygon::ConvertFromPolygon(outTris);
  return result;
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::InterpolateFunctions(double x[3],
                                               double *weights)
{
  this->InitializePolygon();
  this->Polygon->SetUseMVCInterpolation(UseMVCInterpolation);
  this->Polygon->InterpolateFunctions(x, weights);
  vtkQuadraticPolygon::PermuteFromPolygon(this->GetNumberOfPoints(), weights);
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "UseMVCInterpolation: " <<
    this->UseMVCInterpolation << "\n";
  os << indent << "Edge:\n";
    this->Edge->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Polygon:\n";
    this->Polygon->PrintSelf(os,indent.GetNextIndent());
}

//----------------------------------------------------------------------------
double vtkQuadraticPolygon::DistanceToPolygon(double x[3], int numPts,
                                              double *pts, double bounds[6],
                                              double closest[3])
{
  double *convertedPts = new double[numPts * 3];
  vtkQuadraticPolygon::PermuteToPolygon(numPts, pts, convertedPts);

  double result = vtkPolygon::DistanceToPolygon(x, numPts, convertedPts,
                                                bounds, closest);

  delete[] convertedPts;

  return result;
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::ComputeCentroid(vtkIdTypeArray *ids,
                                          vtkPoints *p,
                                          double c[3])
{
  vtkPoints *convertedPts = vtkPoints::New();
  vtkQuadraticPolygon::PermuteToPolygon(p, convertedPts);

  vtkIdTypeArray *convertedIds = vtkIdTypeArray::New();
  vtkQuadraticPolygon::PermuteToPolygon(ids, convertedIds);

  vtkPolygon::ComputeCentroid(convertedIds, convertedPts, c);

  convertedPts->Delete();
  convertedIds->Delete();
}

//----------------------------------------------------------------------------
int vtkQuadraticPolygon::ParameterizePolygon(double *p0, double *p10,
                                             double& l10, double *p20,
                                             double &l20, double *n)
{
  this->InitializePolygon();
  return this->Polygon->ParameterizePolygon(p0, p10, l10, p20, l20, n);
}

//----------------------------------------------------------------------------
int vtkQuadraticPolygon::IntersectPolygonWithPolygon(int npts, double *pts,
                                                     double bounds[6],
                                                     int npts2, double *pts2,
                                                     double bounds2[6],
                                                     double tol2, double x[3])
{
  double *convertedPts = new double[npts*3];
  vtkQuadraticPolygon::PermuteToPolygon(npts, pts, convertedPts);

  double *convertedPts2 = new double[npts2*3];
  vtkQuadraticPolygon::PermuteToPolygon(npts2, pts2, convertedPts2);

  int result = vtkPolygon::IntersectPolygonWithPolygon(
    npts, convertedPts, bounds,
    npts2, convertedPts2, bounds2,
    tol2, x);

  delete[] convertedPts;
  delete[] convertedPts2;

  return result;
}

//----------------------------------------------------------------------------
int vtkQuadraticPolygon::IntersectConvex2DCells(vtkCell *cell1, vtkCell *cell2,
                                                double tol, double p0[3],
                                                double p1[3])
{
  vtkPolygon *convertedCell1 = 0;
  vtkPolygon *convertedCell2 = 0;

  vtkQuadraticPolygon *qp1 = dynamic_cast<vtkQuadraticPolygon*>(cell1);
  if (qp1)
  {
      convertedCell1 = vtkPolygon::New();
      vtkQuadraticPolygon::PermuteToPolygon(cell1, convertedCell1);
  }

  vtkQuadraticPolygon *qp2 = dynamic_cast<vtkQuadraticPolygon*>(cell2);
  if (qp2)
  {
      convertedCell2 = vtkPolygon::New();
      vtkQuadraticPolygon::PermuteToPolygon(cell2, convertedCell2);
  }

  int result = vtkPolygon::IntersectConvex2DCells(
    (convertedCell1 ? convertedCell1 : cell1),
    (convertedCell2 ? convertedCell2 : cell2),
    tol, p0, p1);

  if (convertedCell1)
  {
    convertedCell1->Delete();
  }
  if (convertedCell2)
  {
    convertedCell2->Delete();
  }

  return result;
}

//----------------------------------------------------------------------------
int vtkQuadraticPolygon::PointInPolygon (double x[3], int numPts, double *pts,
                                         double bounds[6], double *n)
{
  double *convertedPts = new double[numPts * 3];
  vtkQuadraticPolygon::PermuteToPolygon(numPts, pts, convertedPts);

  int result = vtkPolygon::PointInPolygon(x, numPts, convertedPts, bounds, n);

  delete[] convertedPts;

  return result;
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::GetPermutationFromPolygon(vtkIdType nb,
                                                    vtkIdList *permutation)
{
  permutation->SetNumberOfIds(nb);
  for (vtkIdType i = 0; i < nb; i++)
  {
    permutation->SetId(i, ((i % 2) ? (i + nb)/2 : i/2));
  }
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteToPolygon(vtkIdType nbPoints,
                                           double *inPoints,
                                           double *outPoints)
{
  vtkIdList *permutation = vtkIdList::New();
  vtkQuadraticPolygon::GetPermutationFromPolygon(nbPoints, permutation);

  for (vtkIdType i = 0; i < nbPoints; i++)
  {
    for (int j = 0; j < 3; j++)
    {
        outPoints[3 * i + j] = inPoints[3 * permutation->GetId(i) + j];
    }
  }

  permutation->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteToPolygon(vtkPoints *inPoints,
                                           vtkPoints *outPoints)
{
  vtkIdType nbPoints = inPoints->GetNumberOfPoints();

  vtkIdList *permutation = vtkIdList::New();
  vtkQuadraticPolygon::GetPermutationFromPolygon(nbPoints, permutation);

  outPoints->SetNumberOfPoints(nbPoints);
  for (vtkIdType i = 0; i < nbPoints; i++)
  {
    outPoints->SetPoint(i, inPoints->GetPoint(permutation->GetId(i)));
  }

  permutation->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteToPolygon(vtkIdTypeArray *inIds,
                                           vtkIdTypeArray *outIds)
{
  vtkIdType nbIds = inIds->GetNumberOfTuples();

  vtkIdList *permutation = vtkIdList::New();
  vtkQuadraticPolygon::GetPermutationFromPolygon(nbIds, permutation);

  outIds->SetNumberOfTuples(nbIds);
  for (vtkIdType i = 0; i < nbIds; i++)
  {
    outIds->SetValue(i, inIds->GetValue(permutation->GetId(i)));
  }

  permutation->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteToPolygon(vtkDataArray *inDataArray,
                                           vtkDataArray *outDataArray)
{
  vtkIdType nb = inDataArray->GetNumberOfTuples();

  vtkIdList *permutation = vtkIdList::New();
  vtkQuadraticPolygon::GetPermutationFromPolygon(nb, permutation);

  outDataArray->SetNumberOfComponents(inDataArray->GetNumberOfComponents());
  outDataArray->SetNumberOfTuples(nb);
  inDataArray->GetTuples(permutation, outDataArray);

  permutation->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteToPolygon(vtkCell *inCell,
                                           vtkCell *outCell)
{
  vtkIdType nbPoints = inCell->GetNumberOfPoints();

  vtkIdList *permutation = vtkIdList::New();
  vtkQuadraticPolygon::GetPermutationFromPolygon(nbPoints, permutation);

  outCell->Points->SetNumberOfPoints(nbPoints);
  outCell->PointIds->SetNumberOfIds(nbPoints);

  for (vtkIdType i = 0; i < nbPoints; i++)
  {
    outCell->PointIds->SetId(i, inCell->PointIds->GetId(permutation->GetId(i)));
    outCell->Points->SetPoint(i, inCell->Points->GetPoint(permutation->GetId(i)));
  }

  permutation->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::InitializePolygon()
{
  vtkQuadraticPolygon::PermuteToPolygon(this, this->Polygon);
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::GetPermutationToPolygon(vtkIdType nb,
                                                  vtkIdList *permutation)
{
  permutation->SetNumberOfIds(nb);
  for (vtkIdType i = 0; i < nb; i++)
  {
    permutation->SetId(i, (i < nb / 2) ? (i * 2) : (i * 2 + 1 - nb));
  }
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::PermuteFromPolygon(vtkIdType nb,
                                             double *values)
{
  vtkIdList *permutation = vtkIdList::New();
  vtkQuadraticPolygon::GetPermutationToPolygon(nb, permutation);

  double *save = new double[nb];
  for (vtkIdType i = 0; i < nb; i++)
  {
    save[i] = values[i];
  }
  for (vtkIdType i = 0; i < nb; i++)
  {
    values[i] = save[permutation->GetId(i)];
  }

  permutation->Delete();
  delete[] save;
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::ConvertFromPolygon(vtkIdList *ids)
{
  vtkIdType nbIds = ids->GetNumberOfIds();

  vtkIdList *permutation = vtkIdList::New();
  vtkQuadraticPolygon::GetPermutationFromPolygon(nbIds, permutation);

  vtkIdList *saveList = vtkIdList::New();
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

  permutation->Delete();
  saveList->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadraticPolygon::Derivatives(int vtkNotUsed(subId),
                                      double vtkNotUsed(pcoords)[3],
                                      double *vtkNotUsed(values),
                                      int vtkNotUsed(dim),
                                      double *vtkNotUsed(derivs))
{
}
