/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericCell.h"

#include "vtkBezierCurve.h"
#include "vtkBezierHexahedron.h"
#include "vtkBezierQuadrilateral.h"
#include "vtkBezierTetra.h"
#include "vtkBezierTriangle.h"
#include "vtkBezierWedge.h"
#include "vtkBiQuadraticQuad.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkConvexPointSet.h"
#include "vtkCubicLine.h"
#include "vtkEmptyCell.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeHexahedron.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLagrangeTetra.h"
#include "vtkLagrangeTriangle.h"
#include "vtkLagrangeWedge.h"
#include "vtkLine.h"
#include "vtkObjectFactory.h"
#include "vtkPentagonalPrism.h"
#include "vtkPixel.h"
#include "vtkPoints.h"
#include "vtkPolyLine.h"
#include "vtkPolyVertex.h"
#include "vtkPolygon.h"
#include "vtkPolyhedron.h"
#include "vtkPyramid.h"
#include "vtkQuad.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkQuadraticPolygon.h"
#include "vtkQuadraticPyramid.h"
#include "vtkQuadraticQuad.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticTriangle.h"
#include "vtkQuadraticWedge.h"
#include "vtkTetra.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

vtkStandardNewMacro(vtkGenericCell);

//----------------------------------------------------------------------------
// Construct cell.
vtkGenericCell::vtkGenericCell()
{
  for (int i = 0; i < VTK_NUMBER_OF_CELL_TYPES; ++i)
  {
    this->CellStore[i] = nullptr;
  }
  this->CellStore[VTK_EMPTY_CELL] = vtkEmptyCell::New();
  this->Cell = this->CellStore[VTK_EMPTY_CELL];
  this->Points->Delete();
  this->Points = this->Cell->Points;
  this->Points->Register(this);
  this->PointIds->Delete();
  this->PointIds = this->Cell->PointIds;
  this->PointIds->Register(this);
}

//----------------------------------------------------------------------------
vtkGenericCell::~vtkGenericCell()
{
  for (int i = 0; i < VTK_NUMBER_OF_CELL_TYPES; ++i)
  {
    if (this->CellStore[i] != nullptr)
    {
      this->CellStore[i]->Delete();
    }
  }
}

//----------------------------------------------------------------------------
void vtkGenericCell::ShallowCopy(vtkCell* c)
{
  this->Cell->ShallowCopy(c);
}

//----------------------------------------------------------------------------
void vtkGenericCell::DeepCopy(vtkCell* c)
{
  this->Cell->DeepCopy(c);
}

//----------------------------------------------------------------------------
int vtkGenericCell::GetCellType()
{
  return this->Cell->GetCellType();
}

//----------------------------------------------------------------------------
int vtkGenericCell::GetCellDimension()
{
  return this->Cell->GetCellDimension();
}

//----------------------------------------------------------------------------
int vtkGenericCell::IsLinear()
{
  return this->Cell->IsLinear();
}

//----------------------------------------------------------------------------
int vtkGenericCell::RequiresInitialization()
{
  return this->Cell->RequiresInitialization();
}

//----------------------------------------------------------------------------
int vtkGenericCell::RequiresExplicitFaceRepresentation()
{
  return this->Cell->RequiresExplicitFaceRepresentation();
}

//----------------------------------------------------------------------------
void vtkGenericCell::SetFaces(vtkIdType* faces)
{
  this->Cell->SetFaces(faces);
}

//----------------------------------------------------------------------------
vtkIdType* vtkGenericCell::GetFaces()
{
  return this->Cell->GetFaces();
}

//----------------------------------------------------------------------------
void vtkGenericCell::Initialize()
{
  this->Cell->Initialize();
}

//----------------------------------------------------------------------------
int vtkGenericCell::GetNumberOfEdges()
{
  return this->Cell->GetNumberOfEdges();
}

//----------------------------------------------------------------------------
int vtkGenericCell::GetNumberOfFaces()
{
  return this->Cell->GetNumberOfFaces();
}

//----------------------------------------------------------------------------
vtkCell* vtkGenericCell::GetEdge(int edgeId)
{
  return this->Cell->GetEdge(edgeId);
}

//----------------------------------------------------------------------------
vtkCell* vtkGenericCell::GetFace(int faceId)
{
  return this->Cell->GetFace(faceId);
}

//----------------------------------------------------------------------------
int vtkGenericCell::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  return this->Cell->CellBoundary(subId, pcoords, pts);
}

//----------------------------------------------------------------------------
int vtkGenericCell::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& dist2, double weights[])
{
  return this->Cell->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights);
}

//----------------------------------------------------------------------------
void vtkGenericCell::EvaluateLocation(
  int& subId, const double pcoords[3], double x[3], double* weights)
{
  this->Cell->EvaluateLocation(subId, pcoords, x, weights);
}

//----------------------------------------------------------------------------
void vtkGenericCell::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  this->Cell->Contour(
    value, cellScalars, locator, verts, lines, polys, inPd, outPd, inCd, cellId, outCd);
}

//----------------------------------------------------------------------------
void vtkGenericCell::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* connectivity, vtkPointData* inPd,
  vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  this->Cell->Clip(
    value, cellScalars, locator, connectivity, inPd, outPd, inCd, cellId, outCd, insideOut);
}

//----------------------------------------------------------------------------
int vtkGenericCell::IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
  double x[3], double pcoords[3], int& subId)
{
  return this->Cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId);
}

//----------------------------------------------------------------------------
int vtkGenericCell::Triangulate(int index, vtkIdList* ptIds, vtkPoints* pts)
{
  return this->Cell->Triangulate(index, ptIds, pts);
}

//----------------------------------------------------------------------------
void vtkGenericCell::Derivatives(
  int subId, const double pcoords[3], const double* values, int dim, double* derivs)
{
  this->Cell->Derivatives(subId, pcoords, values, dim, derivs);
}

//----------------------------------------------------------------------------
int vtkGenericCell::GetParametricCenter(double pcoords[3])
{
  return this->Cell->GetParametricCenter(pcoords);
}

//----------------------------------------------------------------------------
double* vtkGenericCell::GetParametricCoords()
{
  return this->Cell->GetParametricCoords();
}

//----------------------------------------------------------------------------
int vtkGenericCell::IsPrimaryCell()
{
  return this->Cell->IsPrimaryCell();
}

//----------------------------------------------------------------------------
vtkCell* vtkGenericCell::InstantiateCell(int cellType)
{
  vtkCell* cell = nullptr;
  switch (cellType)
  {
    case VTK_EMPTY_CELL:
      cell = vtkEmptyCell::New();
      break;
    case VTK_VERTEX:
      cell = vtkVertex::New();
      break;
    case VTK_POLY_VERTEX:
      cell = vtkPolyVertex::New();
      break;
    case VTK_LINE:
      cell = vtkLine::New();
      break;
    case VTK_POLY_LINE:
      cell = vtkPolyLine::New();
      break;
    case VTK_TRIANGLE:
      cell = vtkTriangle::New();
      break;
    case VTK_TRIANGLE_STRIP:
      cell = vtkTriangleStrip::New();
      break;
    case VTK_POLYGON:
      cell = vtkPolygon::New();
      break;
    case VTK_PIXEL:
      cell = vtkPixel::New();
      break;
    case VTK_QUAD:
      cell = vtkQuad::New();
      break;
    case VTK_TETRA:
      cell = vtkTetra::New();
      break;
    case VTK_VOXEL:
      cell = vtkVoxel::New();
      break;
    case VTK_HEXAHEDRON:
      cell = vtkHexahedron::New();
      break;
    case VTK_WEDGE:
      cell = vtkWedge::New();
      break;
    case VTK_PYRAMID:
      cell = vtkPyramid::New();
      break;
    case VTK_PENTAGONAL_PRISM:
      cell = vtkPentagonalPrism::New();
      break;
    case VTK_HEXAGONAL_PRISM:
      cell = vtkHexagonalPrism::New();
      break;
    case VTK_QUADRATIC_EDGE:
      cell = vtkQuadraticEdge::New();
      break;
    case VTK_QUADRATIC_TRIANGLE:
      cell = vtkQuadraticTriangle::New();
      break;
    case VTK_QUADRATIC_QUAD:
      cell = vtkQuadraticQuad::New();
      break;
    case VTK_QUADRATIC_POLYGON:
      cell = vtkQuadraticPolygon::New();
      break;
    case VTK_QUADRATIC_TETRA:
      cell = vtkQuadraticTetra::New();
      break;
    case VTK_QUADRATIC_HEXAHEDRON:
      cell = vtkQuadraticHexahedron::New();
      break;
    case VTK_QUADRATIC_WEDGE:
      cell = vtkQuadraticWedge::New();
      break;
    case VTK_QUADRATIC_PYRAMID:
      cell = vtkQuadraticPyramid::New();
      break;
    case VTK_QUADRATIC_LINEAR_QUAD:
      cell = vtkQuadraticLinearQuad::New();
      break;
    case VTK_BIQUADRATIC_QUAD:
      cell = vtkBiQuadraticQuad::New();
      break;
    case VTK_TRIQUADRATIC_HEXAHEDRON:
      cell = vtkTriQuadraticHexahedron::New();
      break;
    case VTK_QUADRATIC_LINEAR_WEDGE:
      cell = vtkQuadraticLinearWedge::New();
      break;
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
      cell = vtkBiQuadraticQuadraticWedge::New();
      break;
    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
      cell = vtkBiQuadraticQuadraticHexahedron::New();
      break;
    case VTK_BIQUADRATIC_TRIANGLE:
      cell = vtkBiQuadraticTriangle::New();
      break;
    case VTK_CUBIC_LINE:
      cell = vtkCubicLine::New();
      break;
    case VTK_CONVEX_POINT_SET:
      cell = vtkConvexPointSet::New();
      break;
    case VTK_POLYHEDRON:
      cell = vtkPolyhedron::New();
      break;
    case VTK_LAGRANGE_TRIANGLE:
      cell = vtkLagrangeTriangle::New();
      break;
    case VTK_LAGRANGE_TETRAHEDRON:
      cell = vtkLagrangeTetra::New();
      break;
    case VTK_LAGRANGE_CURVE:
      cell = vtkLagrangeCurve::New();
      break;
    case VTK_LAGRANGE_QUADRILATERAL:
      cell = vtkLagrangeQuadrilateral::New();
      break;
    case VTK_LAGRANGE_HEXAHEDRON:
      cell = vtkLagrangeHexahedron::New();
      break;
    case VTK_LAGRANGE_WEDGE:
      cell = vtkLagrangeWedge::New();
      break;
    case VTK_BEZIER_TRIANGLE:
      cell = vtkBezierTriangle::New();
      break;
    case VTK_BEZIER_TETRAHEDRON:
      cell = vtkBezierTetra::New();
      break;
    case VTK_BEZIER_CURVE:
      cell = vtkBezierCurve::New();
      break;
    case VTK_BEZIER_QUADRILATERAL:
      cell = vtkBezierQuadrilateral::New();
      break;
    case VTK_BEZIER_HEXAHEDRON:
      cell = vtkBezierHexahedron::New();
      break;
    case VTK_BEZIER_WEDGE:
      cell = vtkBezierWedge::New();
      break;
  }
  return cell;
}

//----------------------------------------------------------------------------
// Set the type of dereferenced cell. Checks to see whether cell type
// has changed and creates a new cell only if necessary.
void vtkGenericCell::SetCellType(int cellType)
{
  if (this->Cell->GetCellType() != cellType)
  {
    if (cellType < 0 || cellType >= VTK_NUMBER_OF_CELL_TYPES)
    {
      this->Cell = nullptr;
    }
    else if (this->CellStore[cellType] == nullptr)
    {
      this->CellStore[cellType] = vtkGenericCell::InstantiateCell(cellType);
      this->Cell = this->CellStore[cellType];
    }
    else
    {
      this->Cell = this->CellStore[cellType];
    }
    if (this->Cell == nullptr)
    {
      vtkErrorMacro(<< "Unsupported cell type: " << cellType << " Setting to vtkEmptyCell");
      this->Cell = this->CellStore[VTK_EMPTY_CELL];
    }

    this->Points->UnRegister(this);
    this->Points = this->Cell->Points;
    this->Points->Register(this);
    this->PointIds->UnRegister(this);
    this->PointIds = this->Cell->PointIds;
    this->PointIds->Register(this);
  }
}

//----------------------------------------------------------------------------
void vtkGenericCell::InterpolateFunctions(const double pcoords[3], double* weights)
{
  this->Cell->InterpolateFunctions(pcoords, weights);
}

//----------------------------------------------------------------------------
void vtkGenericCell::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  this->Cell->InterpolateDerivs(pcoords, derivs);
}

//----------------------------------------------------------------------------
void vtkGenericCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Cell:\n";
  this->Cell->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void vtkGenericCell::SetPoints(vtkPoints* points)
{
  if (points != this->Points)
  {
    this->Points->Delete();
    this->Points = points;
    this->Points->Register(this);
    this->Cell->Points->Delete();
    this->Cell->Points = points;
    this->Cell->Points->Register(this);
  }
}

//----------------------------------------------------------------------------
void vtkGenericCell::SetPointIds(vtkIdList* pointIds)
{
  if (pointIds != this->PointIds)
  {
    this->PointIds->Delete();
    this->PointIds = pointIds;
    this->PointIds->Register(this);
    this->Cell->PointIds->Delete();
    this->Cell->PointIds = pointIds;
    this->Cell->PointIds->Register(this);
  }
}
