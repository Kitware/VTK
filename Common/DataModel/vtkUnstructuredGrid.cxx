/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGrid.h"

#include "vtkArrayDispatch.h"
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
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkCellLinks.h"
#include "vtkCellTypes.h"
#include "vtkConvexPointSet.h"
#include "vtkCubicLine.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkEmptyCell.h"
#include "vtkGenericCell.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeHexahedron.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLagrangeTetra.h"
#include "vtkLagrangeTriangle.h"
#include "vtkLagrangeWedge.h"
#include "vtkLine.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPentagonalPrism.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
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
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLinks.h"
#include "vtkTetra.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGridCellIterator.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

#include "vtkSMPTools.h"
#include "vtkTimerLog.h"

#include <algorithm>
#include <limits>
#include <set>

vtkStandardNewMacro(vtkUnstructuredGrid);

//----------------------------------------------------------------------------
vtkIdTypeArray* vtkUnstructuredGrid::GetCellLocationsArray()
{
  if (!this->CellLocations)
  {
    this->CellLocations = vtkSmartPointer<vtkIdTypeArray>::New();
  }
  this->CellLocations->DeepCopy(this->Connectivity->GetOffsetsArray());
  this->CellLocations->SetNumberOfValues(this->GetNumberOfCells());

  return this->CellLocations;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(
  vtkUnsignedCharArray* cellTypes, vtkIdTypeArray*, vtkCellArray* cells)
{
  this->SetCells(cellTypes, cells);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(vtkUnsignedCharArray* cellTypes, vtkIdTypeArray*,
  vtkCellArray* cells, vtkIdTypeArray* faceLocations, vtkIdTypeArray* faces)
{
  this->SetCells(cellTypes, cells, faceLocations, faces);
}

vtkUnstructuredGrid::vtkUnstructuredGrid()
{
  this->Vertex = nullptr;
  this->PolyVertex = nullptr;
  this->BezierCurve = nullptr;
  this->BezierQuadrilateral = nullptr;
  this->BezierHexahedron = nullptr;
  this->BezierTriangle = nullptr;
  this->BezierTetra = nullptr;
  this->BezierWedge = nullptr;
  this->LagrangeCurve = nullptr;
  this->LagrangeQuadrilateral = nullptr;
  this->LagrangeHexahedron = nullptr;
  this->LagrangeTriangle = nullptr;
  this->LagrangeTetra = nullptr;
  this->LagrangeWedge = nullptr;
  this->Line = nullptr;
  this->PolyLine = nullptr;
  this->Triangle = nullptr;
  this->TriangleStrip = nullptr;
  this->Pixel = nullptr;
  this->Quad = nullptr;
  this->Polygon = nullptr;
  this->Tetra = nullptr;
  this->Voxel = nullptr;
  this->Hexahedron = nullptr;
  this->Wedge = nullptr;
  this->Pyramid = nullptr;
  this->PentagonalPrism = nullptr;
  this->HexagonalPrism = nullptr;
  this->QuadraticEdge = nullptr;
  this->QuadraticTriangle = nullptr;
  this->QuadraticQuad = nullptr;
  this->QuadraticPolygon = nullptr;
  this->QuadraticTetra = nullptr;
  this->QuadraticHexahedron = nullptr;
  this->QuadraticWedge = nullptr;
  this->QuadraticPyramid = nullptr;
  this->QuadraticLinearQuad = nullptr;
  this->BiQuadraticQuad = nullptr;
  this->TriQuadraticHexahedron = nullptr;
  this->QuadraticLinearWedge = nullptr;
  this->BiQuadraticQuadraticWedge = nullptr;
  this->BiQuadraticQuadraticHexahedron = nullptr;
  this->BiQuadraticTriangle = nullptr;
  this->CubicLine = nullptr;

  this->ConvexPointSet = nullptr;
  this->Polyhedron = nullptr;
  this->EmptyCell = nullptr;

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);

  this->DistinctCellTypesUpdateMTime = 0;

  this->AllocateExact(1024, 1024);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid::~vtkUnstructuredGrid()
{
  if (this->Vertex)
  {
    this->Vertex->Delete();
  }
  if (this->PolyVertex)
  {
    this->PolyVertex->Delete();
  }
  if (this->BezierCurve)
  {
    this->BezierCurve->Delete();
  }
  if (this->BezierQuadrilateral)
  {
    this->BezierQuadrilateral->Delete();
  }
  if (this->BezierHexahedron)
  {
    this->BezierHexahedron->Delete();
  }
  if (this->BezierTriangle)
  {
    this->BezierTriangle->Delete();
  }
  if (this->BezierTetra)
  {
    this->BezierTetra->Delete();
  }
  if (this->BezierWedge)
  {
    this->BezierWedge->Delete();
  }
  if (this->LagrangeCurve)
  {
    this->LagrangeCurve->Delete();
  }
  if (this->LagrangeQuadrilateral)
  {
    this->LagrangeQuadrilateral->Delete();
  }
  if (this->LagrangeHexahedron)
  {
    this->LagrangeHexahedron->Delete();
  }
  if (this->LagrangeTriangle)
  {
    this->LagrangeTriangle->Delete();
  }
  if (this->LagrangeTetra)
  {
    this->LagrangeTetra->Delete();
  }
  if (this->LagrangeWedge)
  {
    this->LagrangeWedge->Delete();
  }
  if (this->Line)
  {
    this->Line->Delete();
  }
  if (this->PolyLine)
  {
    this->PolyLine->Delete();
  }
  if (this->Triangle)
  {
    this->Triangle->Delete();
  }
  if (this->TriangleStrip)
  {
    this->TriangleStrip->Delete();
  }
  if (this->Pixel)
  {
    this->Pixel->Delete();
  }
  if (this->Quad)
  {
    this->Quad->Delete();
  }
  if (this->Polygon)
  {
    this->Polygon->Delete();
  }
  if (this->Tetra)
  {
    this->Tetra->Delete();
  }
  if (this->Voxel)
  {
    this->Voxel->Delete();
  }
  if (this->Hexahedron)
  {
    this->Hexahedron->Delete();
  }
  if (this->Wedge)
  {
    this->Wedge->Delete();
  }
  if (this->Pyramid)
  {
    this->Pyramid->Delete();
  }
  if (this->PentagonalPrism)
  {
    this->PentagonalPrism->Delete();
  }
  if (this->HexagonalPrism)
  {
    this->HexagonalPrism->Delete();
  }
  if (this->QuadraticEdge)
  {
    this->QuadraticEdge->Delete();
  }
  if (this->QuadraticTriangle)
  {
    this->QuadraticTriangle->Delete();
  }
  if (this->QuadraticQuad)
  {
    this->QuadraticQuad->Delete();
  }
  if (this->QuadraticPolygon)
  {
    this->QuadraticPolygon->Delete();
  }
  if (this->QuadraticTetra)
  {
    this->QuadraticTetra->Delete();
  }
  if (this->QuadraticHexahedron)
  {
    this->QuadraticHexahedron->Delete();
  }
  if (this->QuadraticWedge)
  {
    this->QuadraticWedge->Delete();
  }
  if (this->QuadraticPyramid)
  {
    this->QuadraticPyramid->Delete();
  }
  if (this->QuadraticLinearQuad)
  {
    this->QuadraticLinearQuad->Delete();
  }
  if (this->BiQuadraticQuad)
  {
    this->BiQuadraticQuad->Delete();
  }
  if (this->TriQuadraticHexahedron)
  {
    this->TriQuadraticHexahedron->Delete();
  }
  if (this->QuadraticLinearWedge)
  {
    this->QuadraticLinearWedge->Delete();
  }
  if (this->BiQuadraticQuadraticWedge)
  {
    this->BiQuadraticQuadraticWedge->Delete();
  }
  if (this->BiQuadraticQuadraticHexahedron)
  {
    this->BiQuadraticQuadraticHexahedron->Delete();
  }
  if (this->BiQuadraticTriangle)
  {
    this->BiQuadraticTriangle->Delete();
  }
  if (this->CubicLine)
  {
    this->CubicLine->Delete();
  }

  if (this->ConvexPointSet)
  {
    this->ConvexPointSet->Delete();
  }
  if (this->Polyhedron)
  {
    this->Polyhedron->Delete();
  }
  if (this->EmptyCell)
  {
    this->EmptyCell->Delete();
  }
}

//----------------------------------------------------------------------------
int vtkUnstructuredGrid::GetPiece()
{
  return this->Information->Get(vtkDataObject::DATA_PIECE_NUMBER());
}

//----------------------------------------------------------------------------
int vtkUnstructuredGrid::GetNumberOfPieces()
{
  return this->Information->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
}

//----------------------------------------------------------------------------
int vtkUnstructuredGrid::GetGhostLevel()
{
  return this->Information->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
}

//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input unstructured grid.
void vtkUnstructuredGrid::CopyStructure(vtkDataSet* ds)
{
  // If ds is a vtkUnstructuredGrid, do a shallow copy of the cell data.
  if (vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(ds))
  {
    this->Connectivity = ug->Connectivity;
    this->Links = ug->Links;
    this->Types = ug->Types;
    this->DistinctCellTypes = nullptr;
    this->DistinctCellTypesUpdateMTime = 0;
    this->Faces = ug->Faces;
    this->FaceLocations = ug->FaceLocations;
  }

  this->Superclass::CopyStructure(ds);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::Cleanup()
{
  this->Connectivity = nullptr;
  this->Links = nullptr;
  this->Types = nullptr;
  this->DistinctCellTypes = nullptr;
  this->DistinctCellTypesUpdateMTime = 0;
  this->Faces = nullptr;
  this->FaceLocations = nullptr;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::Initialize()
{
  vtkPointSet::Initialize();

  this->Cleanup();

  if (this->Information)
  {
    this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
    this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 0);
    this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
  }
}

//----------------------------------------------------------------------------
int vtkUnstructuredGrid::GetCellType(vtkIdType cellId)
{
  vtkDebugMacro(<< "Returning cell type " << static_cast<int>(this->Types->GetValue(cellId)));
  return static_cast<int>(this->Types->GetValue(cellId));
}

//----------------------------------------------------------------------------
vtkCell* vtkUnstructuredGrid::GetCell(vtkIdType cellId)
{
  vtkIdType numPts;
  const vtkIdType* pts;
  this->Connectivity->GetCellAtId(cellId, numPts, pts);

  vtkCell* cell = nullptr;
  switch (this->Types->GetValue(cellId))
  {
    case VTK_VERTEX:
      if (!this->Vertex)
      {
        this->Vertex = vtkVertex::New();
      }
      cell = this->Vertex;
      break;

    case VTK_POLY_VERTEX:
      if (!this->PolyVertex)
      {
        this->PolyVertex = vtkPolyVertex::New();
      }
      cell = this->PolyVertex;
      break;

    case VTK_LINE:
      if (!this->Line)
      {
        this->Line = vtkLine::New();
      }
      cell = this->Line;
      break;

    case VTK_LAGRANGE_CURVE:
      if (!this->LagrangeCurve)
      {
        this->LagrangeCurve = vtkLagrangeCurve::New();
      }
      cell = this->LagrangeCurve;
      break;

    case VTK_LAGRANGE_QUADRILATERAL:
      if (!this->LagrangeQuadrilateral)
      {
        this->LagrangeQuadrilateral = vtkLagrangeQuadrilateral::New();
      }
      if (GetCellData()->SetActiveAttribute(
            "HigherOrderDegrees", vtkDataSetAttributes::AttributeTypes::HIGHERORDERDEGREES) != -1)
      {
        double degs[3];
        vtkDataArray* v = GetCellData()->GetHigherOrderDegrees();
        v->GetTuple(cellId, degs);
        this->LagrangeQuadrilateral->SetOrder(degs[0], degs[1]);
      }
      else
      {
        this->LagrangeQuadrilateral->SetUniformOrderFromNumPoints(numPts);
      }
      cell = this->LagrangeQuadrilateral;
      break;

    case VTK_LAGRANGE_HEXAHEDRON:
      if (!this->LagrangeHexahedron)
      {
        this->LagrangeHexahedron = vtkLagrangeHexahedron::New();
      }
      if (GetCellData()->SetActiveAttribute(
            "HigherOrderDegrees", vtkDataSetAttributes::AttributeTypes::HIGHERORDERDEGREES) != -1)
      {
        double degs[3];
        vtkDataArray* v = GetCellData()->GetHigherOrderDegrees();
        v->GetTuple(cellId, degs);
        this->LagrangeHexahedron->SetOrder(degs[0], degs[1], degs[2]);
      }
      else
      {
        this->LagrangeHexahedron->SetUniformOrderFromNumPoints(numPts);
      }
      cell = this->LagrangeHexahedron;
      break;

    case VTK_LAGRANGE_TRIANGLE:
      if (!this->LagrangeTriangle)
      {
        this->LagrangeTriangle = vtkLagrangeTriangle::New();
      }
      cell = this->LagrangeTriangle;
      break;

    case VTK_LAGRANGE_TETRAHEDRON:
      if (!this->LagrangeTetra)
      {
        this->LagrangeTetra = vtkLagrangeTetra::New();
      }
      cell = this->LagrangeTetra;
      break;

    case VTK_LAGRANGE_WEDGE:
      if (!this->LagrangeWedge)
      {
        this->LagrangeWedge = vtkLagrangeWedge::New();
      }
      if (GetCellData()->SetActiveAttribute(
            "HigherOrderDegrees", vtkDataSetAttributes::AttributeTypes::HIGHERORDERDEGREES) != -1)
      {
        double degs[3];
        vtkDataArray* v = GetCellData()->GetHigherOrderDegrees();
        v->GetTuple(cellId, degs);
        this->LagrangeWedge->SetOrder(degs[0], degs[1], degs[2], numPts);
      }
      else
      {
        this->LagrangeWedge->SetUniformOrderFromNumPoints(numPts);
      }
      cell = this->LagrangeWedge;
      break;

    case VTK_BEZIER_CURVE:
      if (!this->BezierCurve)
      {
        this->BezierCurve = vtkBezierCurve::New();
      }
      if (GetPointData()->SetActiveAttribute(
            "RationalWeights", vtkDataSetAttributes::AttributeTypes::RATIONALWEIGHTS) != -1)
      {
        vtkDataArray* v = GetPointData()->GetRationalWeights();
        this->BezierCurve->GetRationalWeights()->SetNumberOfTuples(numPts);
        for (int i = 0; i < numPts; i++)
        {
          this->BezierCurve->GetRationalWeights()->SetValue(i, v->GetTuple1(pts[i]));
        }
      }
      else
        this->BezierCurve->GetRationalWeights()->Reset();
      cell = this->BezierCurve;
      break;

    case VTK_BEZIER_QUADRILATERAL:
      if (!this->BezierQuadrilateral)
      {
        this->BezierQuadrilateral = vtkBezierQuadrilateral::New();
      }
      if (GetCellData()->SetActiveAttribute(
            "HigherOrderDegrees", vtkDataSetAttributes::AttributeTypes::HIGHERORDERDEGREES) != -1)
      {
        double degs[3];
        vtkDataArray* v = GetCellData()->GetHigherOrderDegrees();
        v->GetTuple(cellId, degs);
        this->BezierQuadrilateral->SetOrder(degs[0], degs[1]);
      }
      else
      {
        this->BezierQuadrilateral->SetUniformOrderFromNumPoints(numPts);
      }
      if (GetPointData()->SetActiveAttribute(
            "RationalWeights", vtkDataSetAttributes::AttributeTypes::RATIONALWEIGHTS) != -1)
      {
        vtkDataArray* v = GetPointData()->GetRationalWeights();
        this->BezierQuadrilateral->GetRationalWeights()->SetNumberOfTuples(numPts);
        for (int i = 0; i < numPts; i++)
        {
          this->BezierQuadrilateral->GetRationalWeights()->SetValue(i, v->GetTuple1(pts[i]));
        }
      }
      else
        this->BezierQuadrilateral->GetRationalWeights()->Reset();
      cell = this->BezierQuadrilateral;
      break;

    case VTK_BEZIER_HEXAHEDRON:
      if (!this->BezierHexahedron)
      {
        this->BezierHexahedron = vtkBezierHexahedron::New();
      }
      if (GetCellData()->SetActiveAttribute(
            "HigherOrderDegrees", vtkDataSetAttributes::AttributeTypes::HIGHERORDERDEGREES) != -1)
      {
        double degs[3];
        vtkDataArray* v = GetCellData()->GetHigherOrderDegrees();
        v->GetTuple(cellId, degs);
        this->BezierHexahedron->SetOrder(degs[0], degs[1], degs[2]);
      }
      else
      {
        this->BezierHexahedron->SetUniformOrderFromNumPoints(numPts);
      }
      if (GetPointData()->SetActiveAttribute(
            "RationalWeights", vtkDataSetAttributes::AttributeTypes::RATIONALWEIGHTS) != -1)
      {
        vtkDataArray* v = GetPointData()->GetRationalWeights();
        this->BezierHexahedron->GetRationalWeights()->SetNumberOfTuples(numPts);
        for (int i = 0; i < numPts; i++)
        {
          this->BezierHexahedron->GetRationalWeights()->SetValue(i, v->GetTuple1(pts[i]));
        }
      }
      else
        this->BezierHexahedron->GetRationalWeights()->Reset();
      cell = this->BezierHexahedron;
      break;

    case VTK_BEZIER_TRIANGLE:
      if (!this->BezierTriangle)
      {
        this->BezierTriangle = vtkBezierTriangle::New();
      }
      if (GetPointData()->SetActiveAttribute(
            "RationalWeights", vtkDataSetAttributes::AttributeTypes::RATIONALWEIGHTS) != -1)
      {
        vtkDataArray* v = GetPointData()->GetRationalWeights();
        this->BezierTriangle->GetRationalWeights()->SetNumberOfTuples(numPts);
        for (int i = 0; i < numPts; i++)
        {
          this->BezierTriangle->GetRationalWeights()->SetValue(i, v->GetTuple1(pts[i]));
        }
      }
      else
        this->BezierTriangle->GetRationalWeights()->Reset();
      cell = this->BezierTriangle;
      break;

    case VTK_BEZIER_TETRAHEDRON:
      if (!this->BezierTetra)
      {
        this->BezierTetra = vtkBezierTetra::New();
      }
      if (GetPointData()->SetActiveAttribute(
            "RationalWeights", vtkDataSetAttributes::AttributeTypes::RATIONALWEIGHTS) != -1)
      {
        vtkDataArray* v = GetPointData()->GetRationalWeights();
        this->BezierTetra->GetRationalWeights()->SetNumberOfTuples(numPts);
        for (int i = 0; i < numPts; i++)
        {
          this->BezierTetra->GetRationalWeights()->SetValue(i, v->GetTuple1(pts[i]));
        }
      }
      else
        this->BezierTetra->GetRationalWeights()->Reset();
      cell = this->BezierTetra;
      break;

    case VTK_BEZIER_WEDGE:
      if (!this->BezierWedge)
      {
        this->BezierWedge = vtkBezierWedge::New();
      }
      if (GetCellData()->SetActiveAttribute(
            "HigherOrderDegrees", vtkDataSetAttributes::AttributeTypes::HIGHERORDERDEGREES) != -1)
      {
        double degs[3];
        vtkDataArray* v = GetCellData()->GetHigherOrderDegrees();
        v->GetTuple(cellId, degs);
        this->BezierWedge->SetOrder(degs[0], degs[1], degs[2], numPts);
      }
      else
      {
        this->BezierWedge->SetUniformOrderFromNumPoints(numPts);
      }
      if (GetPointData()->SetActiveAttribute(
            "RationalWeights", vtkDataSetAttributes::AttributeTypes::RATIONALWEIGHTS) != -1)
      {
        vtkDataArray* v = GetPointData()->GetRationalWeights();
        this->BezierWedge->GetRationalWeights()->SetNumberOfTuples(numPts);
        for (int i = 0; i < numPts; i++)
        {
          this->BezierWedge->GetRationalWeights()->SetValue(i, v->GetTuple1(pts[i]));
        }
      }
      else
        this->BezierWedge->GetRationalWeights()->Reset();
      cell = this->BezierWedge;
      break;

    case VTK_POLY_LINE:
      if (!this->PolyLine)
      {
        this->PolyLine = vtkPolyLine::New();
      }
      cell = this->PolyLine;
      break;

    case VTK_TRIANGLE:
      if (!this->Triangle)
      {
        this->Triangle = vtkTriangle::New();
      }
      cell = this->Triangle;
      break;

    case VTK_TRIANGLE_STRIP:
      if (!this->TriangleStrip)
      {
        this->TriangleStrip = vtkTriangleStrip::New();
      }
      cell = this->TriangleStrip;
      break;

    case VTK_PIXEL:
      if (!this->Pixel)
      {
        this->Pixel = vtkPixel::New();
      }
      cell = this->Pixel;
      break;

    case VTK_QUAD:
      if (!this->Quad)
      {
        this->Quad = vtkQuad::New();
      }
      cell = this->Quad;
      break;

    case VTK_POLYGON:
      if (!this->Polygon)
      {
        this->Polygon = vtkPolygon::New();
      }
      cell = this->Polygon;
      break;

    case VTK_TETRA:
      if (!this->Tetra)
      {
        this->Tetra = vtkTetra::New();
      }
      cell = this->Tetra;
      break;

    case VTK_VOXEL:
      if (!this->Voxel)
      {
        this->Voxel = vtkVoxel::New();
      }
      cell = this->Voxel;
      break;

    case VTK_HEXAHEDRON:
      if (!this->Hexahedron)
      {
        this->Hexahedron = vtkHexahedron::New();
      }
      cell = this->Hexahedron;
      break;

    case VTK_WEDGE:
      if (!this->Wedge)
      {
        this->Wedge = vtkWedge::New();
      }
      cell = this->Wedge;
      break;

    case VTK_PYRAMID:
      if (!this->Pyramid)
      {
        this->Pyramid = vtkPyramid::New();
      }
      cell = this->Pyramid;
      break;

    case VTK_PENTAGONAL_PRISM:
      if (!this->PentagonalPrism)
      {
        this->PentagonalPrism = vtkPentagonalPrism::New();
      }
      cell = this->PentagonalPrism;
      break;

    case VTK_HEXAGONAL_PRISM:
      if (!this->HexagonalPrism)
      {
        this->HexagonalPrism = vtkHexagonalPrism::New();
      }
      cell = this->HexagonalPrism;
      break;

    case VTK_QUADRATIC_EDGE:
      if (!this->QuadraticEdge)
      {
        this->QuadraticEdge = vtkQuadraticEdge::New();
      }
      cell = this->QuadraticEdge;
      break;

    case VTK_QUADRATIC_TRIANGLE:
      if (!this->QuadraticTriangle)
      {
        this->QuadraticTriangle = vtkQuadraticTriangle::New();
      }
      cell = this->QuadraticTriangle;
      break;

    case VTK_QUADRATIC_QUAD:
      if (!this->QuadraticQuad)
      {
        this->QuadraticQuad = vtkQuadraticQuad::New();
      }
      cell = this->QuadraticQuad;
      break;

    case VTK_QUADRATIC_POLYGON:
      if (!this->QuadraticPolygon)
      {
        this->QuadraticPolygon = vtkQuadraticPolygon::New();
      }
      cell = this->QuadraticPolygon;
      break;

    case VTK_QUADRATIC_TETRA:
      if (!this->QuadraticTetra)
      {
        this->QuadraticTetra = vtkQuadraticTetra::New();
      }
      cell = this->QuadraticTetra;
      break;

    case VTK_QUADRATIC_HEXAHEDRON:
      if (!this->QuadraticHexahedron)
      {
        this->QuadraticHexahedron = vtkQuadraticHexahedron::New();
      }
      cell = this->QuadraticHexahedron;
      break;

    case VTK_QUADRATIC_WEDGE:
      if (!this->QuadraticWedge)
      {
        this->QuadraticWedge = vtkQuadraticWedge::New();
      }
      cell = this->QuadraticWedge;
      break;

    case VTK_QUADRATIC_PYRAMID:
      if (!this->QuadraticPyramid)
      {
        this->QuadraticPyramid = vtkQuadraticPyramid::New();
      }
      cell = this->QuadraticPyramid;
      break;

    case VTK_QUADRATIC_LINEAR_QUAD:
      if (!this->QuadraticLinearQuad)
      {
        this->QuadraticLinearQuad = vtkQuadraticLinearQuad::New();
      }
      cell = this->QuadraticLinearQuad;
      break;

    case VTK_BIQUADRATIC_QUAD:
      if (!this->BiQuadraticQuad)
      {
        this->BiQuadraticQuad = vtkBiQuadraticQuad::New();
      }
      cell = this->BiQuadraticQuad;
      break;

    case VTK_TRIQUADRATIC_HEXAHEDRON:
      if (!this->TriQuadraticHexahedron)
      {
        this->TriQuadraticHexahedron = vtkTriQuadraticHexahedron::New();
      }
      cell = this->TriQuadraticHexahedron;
      break;

    case VTK_QUADRATIC_LINEAR_WEDGE:
      if (!this->QuadraticLinearWedge)
      {
        this->QuadraticLinearWedge = vtkQuadraticLinearWedge::New();
      }
      cell = this->QuadraticLinearWedge;
      break;

    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
      if (!this->BiQuadraticQuadraticWedge)
      {
        this->BiQuadraticQuadraticWedge = vtkBiQuadraticQuadraticWedge::New();
      }
      cell = this->BiQuadraticQuadraticWedge;
      break;

    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
      if (!this->BiQuadraticQuadraticHexahedron)
      {
        this->BiQuadraticQuadraticHexahedron = vtkBiQuadraticQuadraticHexahedron::New();
      }
      cell = this->BiQuadraticQuadraticHexahedron;
      break;
    case VTK_BIQUADRATIC_TRIANGLE:
      if (!this->BiQuadraticTriangle)
      {
        this->BiQuadraticTriangle = vtkBiQuadraticTriangle::New();
      }
      cell = this->BiQuadraticTriangle;
      break;
    case VTK_CUBIC_LINE:
      if (!this->CubicLine)
      {
        this->CubicLine = vtkCubicLine::New();
      }
      cell = this->CubicLine;
      break;

    case VTK_CONVEX_POINT_SET:
      if (!this->ConvexPointSet)
      {
        this->ConvexPointSet = vtkConvexPointSet::New();
      }
      cell = this->ConvexPointSet;
      break;

    case VTK_POLYHEDRON:
      if (!this->Polyhedron)
      {
        this->Polyhedron = vtkPolyhedron::New();
      }
      this->Polyhedron->SetFaces(this->GetFaces(cellId));
      cell = this->Polyhedron;
      break;

    case VTK_EMPTY_CELL:
      if (!this->EmptyCell)
      {
        this->EmptyCell = vtkEmptyCell::New();
      }
      cell = this->EmptyCell;
      break;
  }

  if (!cell)
  {
    return nullptr;
  }

  // Copy the points over to the cell.
  cell->PointIds->SetNumberOfIds(numPts);
  cell->Points->SetNumberOfPoints(numPts);
  for (vtkIdType i = 0; i < numPts; i++)
  {
    cell->PointIds->SetId(i, pts[i]);
    cell->Points->SetPoint(i, this->Points->GetPoint(pts[i]));
  }

  // Some cells require special initialization to build data structures
  // and such.
  if (cell->RequiresInitialization())
  {
    cell->Initialize();
  }

  return cell;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetCell(vtkIdType cellId, vtkGenericCell* cell)
{

  int cellType = static_cast<int>(this->Types->GetValue(cellId));
  cell->SetCellType(cellType);

  vtkIdType numPts;
  const vtkIdType* pts;
  this->Connectivity->GetCellAtId(cellId, numPts, pts);

  cell->PointIds->SetNumberOfIds(numPts);

  std::copy(pts, pts + numPts, cell->PointIds->GetPointer(0));
  this->Points->GetPoints(cell->PointIds, cell->Points);

  // Explicit face representation
  if (cell->RequiresExplicitFaceRepresentation())
  {
    cell->SetFaces(this->GetFaces(cellId));
  }

  // Some cells require special initialization to build data structures
  // and such.
  if (cell->RequiresInitialization())
  {
    cell->Initialize();
  }
  this->SetCellOrderAndRationalWeights(cellId, cell);
}

//----------------------------------------------------------------------------
// Support GetCellBounds()
namespace
{ // anonymous
struct ComputeCellBoundsWorker
{
  struct Visitor
  {
    // vtkCellArray::Visit entry point:
    template <typename CellStateT, typename PointArrayT>
    void operator()(
      CellStateT& state, PointArrayT* ptArray, vtkIdType cellId, double bounds[6]) const
    {
      using IdType = typename CellStateT::ValueType;

      const auto ptIds = state.GetCellRange(cellId);
      if (ptIds.size() == 0)
      {
        vtkMath::UninitializeBounds(bounds);
        return;
      }

      const auto points = vtk::DataArrayTupleRange<3>(ptArray);

      // Initialize bounds to first point:
      {
        const auto pt = points[ptIds[0]];

        // Explicitly reusing a local will improve performance when virtual
        // calls are involved in the iterator read:
        const double x = static_cast<double>(pt[0]);
        const double y = static_cast<double>(pt[1]);
        const double z = static_cast<double>(pt[2]);

        bounds[0] = x;
        bounds[1] = x;
        bounds[2] = y;
        bounds[3] = y;
        bounds[4] = z;
        bounds[5] = z;
      }

      // Reduce bounds with the rest of the ids:
      for (const IdType ptId : ptIds.GetSubRange(1))
      {
        const auto pt = points[ptId];

        // Explicitly reusing a local will improve performance when virtual
        // calls are involved in the iterator read:
        const double x = static_cast<double>(pt[0]);
        const double y = static_cast<double>(pt[1]);
        const double z = static_cast<double>(pt[2]);

        bounds[0] = std::min(bounds[0], x);
        bounds[1] = std::max(bounds[1], x);
        bounds[2] = std::min(bounds[2], y);
        bounds[3] = std::max(bounds[3], y);
        bounds[4] = std::min(bounds[4], z);
        bounds[5] = std::max(bounds[5], z);
      }
    }
  };

  // vtkArrayDispatch entry point:
  template <typename PointArrayT>
  void operator()(
    PointArrayT* ptArray, vtkCellArray* conn, vtkIdType cellId, double bounds[6]) const
  {
    conn->Visit(Visitor{}, ptArray, cellId, bounds);
  }
};

} // anonymous

//----------------------------------------------------------------------------
// Faster implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkUnstructuredGrid::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  // Fast path for float/double:
  using vtkArrayDispatch::Reals;
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<Reals>;
  ComputeCellBoundsWorker worker;

  vtkDataArray* ptArray = this->Points->GetData();
  if (!Dispatcher::Execute(ptArray, worker, this->Connectivity, cellId, bounds))
  { // fallback for weird types:
    worker(ptArray, this->Connectivity, cellId, bounds);
  }
}

//----------------------------------------------------------------------------
// Return the number of points from the cell defined by the maximum number of
// points/
int vtkUnstructuredGrid::GetMaxCellSize()
{
  if (this->Connectivity)
  { // The internal implementation is threaded.
    return this->Connectivity->GetMaxCellSize();
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkUnstructuredGrid::GetNumberOfCells()
{
  vtkDebugMacro(<< "NUMBER OF CELLS = "
                << (this->Connectivity ? this->Connectivity->GetNumberOfCells() : 0));
  return (this->Connectivity ? this->Connectivity->GetNumberOfCells() : 0);
}

//----------------------------------------------------------------------------
// Insert/create cell in object by type and list of point ids defining
// cell topology. Using a special input format, this function also support
// polyhedron cells.
vtkIdType vtkUnstructuredGrid::InternalInsertNextCell(int type, vtkIdList* ptIds)
{
  if (type == VTK_POLYHEDRON)
  {
    // For polyhedron cell, input ptIds is of format:
    // (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
    vtkIdType* dataPtr = ptIds->GetPointer(0);
    return this->InsertNextCell(type, dataPtr[0], dataPtr + 1);
  }

  this->Connectivity->InsertNextCell(ptIds);

  // If faces have been created, we need to pad them (we are not creating
  // a polyhedral cell in this method)
  if (this->FaceLocations)
  {
    this->FaceLocations->InsertNextValue(-1);
  }

  // insert cell type
  return this->Types->InsertNextValue(static_cast<unsigned char>(type));
}

//----------------------------------------------------------------------------
// Insert/create cell in object by type and list of point ids defining
// cell topology. Using a special input format, this function also support
// polyhedron cells.
vtkIdType vtkUnstructuredGrid::InternalInsertNextCell(
  int type, vtkIdType npts, const vtkIdType ptIds[])
{
  if (type != VTK_POLYHEDRON)
  {
    // insert connectivity
    this->Connectivity->InsertNextCell(npts, ptIds);

    // If faces have been created, we need to pad them (we are not creating
    // a polyhedral cell in this method)
    if (this->FaceLocations)
    {
      this->FaceLocations->InsertNextValue(-1);
    }
  }
  else
  {
    // For polyhedron, npts is actually number of faces, ptIds is of format:
    // (numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
    vtkIdType realnpts;

    // We defer allocation for the faces because they are not commonly used and
    // we only want to allocate when necessary.
    if (!this->Faces)
    {
      this->Faces = vtkSmartPointer<vtkIdTypeArray>::New();
      this->Faces->Allocate(this->Types->GetSize());
      this->FaceLocations = vtkSmartPointer<vtkIdTypeArray>::New();
      this->FaceLocations->Allocate(this->Types->GetSize());
      // FaceLocations must be padded until the current position
      for (vtkIdType i = 0; i <= this->Types->GetMaxId(); i++)
      {
        this->FaceLocations->InsertNextValue(-1);
      }
    }

    // insert face location
    this->FaceLocations->InsertNextValue(this->Faces->GetMaxId() + 1);

    // insert cell connectivity and faces stream
    vtkUnstructuredGrid::DecomposeAPolyhedronCell(
      npts, ptIds, realnpts, this->Connectivity, this->Faces);
  }

  return this->Types->InsertNextValue(static_cast<unsigned char>(type));
}

//----------------------------------------------------------------------------
// Insert/create cell in object by type and list of point and face ids
// defining cell topology. This method is meant for face-explicit cells (e.g.
// polyhedron).
vtkIdType vtkUnstructuredGrid::InternalInsertNextCell(
  int type, vtkIdType npts, const vtkIdType pts[], vtkIdType nfaces, const vtkIdType faces[])
{
  if (type != VTK_POLYHEDRON)
  {
    return this->InsertNextCell(type, npts, pts);
  }
  // Insert connectivity (points that make up polyhedron)
  this->Connectivity->InsertNextCell(npts, pts);

  // Now insert faces; allocate storage if necessary.
  // We defer allocation for the faces because they are not commonly used and
  // we only want to allocate when necessary.
  if (!this->Faces)
  {
    this->Faces = vtkSmartPointer<vtkIdTypeArray>::New();
    this->Faces->Allocate(this->Types->GetSize());
    this->FaceLocations = vtkSmartPointer<vtkIdTypeArray>::New();
    this->FaceLocations->Allocate(this->Types->GetSize());
    // FaceLocations must be padded until the current position
    for (vtkIdType i = 0; i <= this->Types->GetMaxId(); i++)
    {
      this->FaceLocations->InsertNextValue(-1);
    }
  }

  // Okay the faces go in
  this->FaceLocations->InsertNextValue(this->Faces->GetMaxId() + 1);
  this->Faces->InsertNextValue(nfaces);

  for (int faceNum = 0; faceNum < nfaces; ++faceNum)
  {
    npts = faces[0];
    this->Faces->InsertNextValue(npts);
    for (vtkIdType i = 1; i <= npts; ++i)
    {
      this->Faces->InsertNextValue(faces[i]);
    }
    faces += npts + 1;
  } // for all faces

  return this->Types->InsertNextValue(static_cast<unsigned char>(type));
}

//----------------------------------------------------------------------------
int vtkUnstructuredGrid::InitializeFacesRepresentation(vtkIdType numPrevCells)
{
  if (this->Faces || this->FaceLocations)
  {
    vtkErrorMacro("Face information already exist for this unstuructured grid. "
                  "InitializeFacesRepresentation returned without execution.");
    return 0;
  }

  this->Faces = vtkSmartPointer<vtkIdTypeArray>::New();
  this->Faces->Allocate(this->Types->GetSize());

  this->FaceLocations = vtkSmartPointer<vtkIdTypeArray>::New();
  this->FaceLocations->Allocate(this->Types->GetSize());
  // FaceLocations must be padded until the current position
  for (vtkIdType i = 0; i < numPrevCells; i++)
  {
    this->FaceLocations->InsertNextValue(-1);
  }

  return 1;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkUnstructuredGrid::GetMeshMTime()
{
  return vtkMath::Max(this->Points ? this->Points->GetMTime() : 0,
    this->Connectivity ? this->Connectivity->GetMTime() : 0);
}

//----------------------------------------------------------------------------
// Return faces for a polyhedral cell (or face-explicit cell).
vtkIdType* vtkUnstructuredGrid::GetFaces(vtkIdType cellId)
{
  // Get the locations of the face
  vtkIdType loc;
  if (!this->Faces || cellId < 0 || cellId > this->FaceLocations->GetMaxId() ||
    (loc = this->FaceLocations->GetValue(cellId)) == -1)
  {
    return nullptr;
  }

  return this->Faces->GetPointer(loc);
}

//----------------------------------------------------------------------------
vtkIdTypeArray* vtkUnstructuredGrid::GetFaces()
{
  return this->Faces;
}

//----------------------------------------------------------------------------
vtkIdTypeArray* vtkUnstructuredGrid::GetFaceLocations()
{
  return this->FaceLocations;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(int type, vtkCellArray* cells)
{
  vtkNew<vtkUnsignedCharArray> types;
  types->SetNumberOfComponents(1);
  types->SetNumberOfValues(cells->GetNumberOfCells());
  types->FillValue(static_cast<unsigned char>(type));

  this->SetCells(types, cells);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(int* types, vtkCellArray* cells)
{
  const vtkIdType ncells = cells->GetNumberOfCells();

  // Convert the types into a vtkUnsignedCharArray:
  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfTuples(ncells);
  auto typeRange = vtk::DataArrayValueRange<1>(cellTypes);
  std::transform(types, types + ncells, typeRange.begin(),
    [](int t) -> unsigned char { return static_cast<unsigned char>(t); });

  this->SetCells(cellTypes, cells);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(vtkUnsignedCharArray* cellTypes, vtkCellArray* cells)
{
  // check if cells contain any polyhedron cell
  const vtkIdType ncells = cells->GetNumberOfCells();
  const auto typeRange = vtk::DataArrayValueRange<1>(cellTypes);
  const bool containPolyhedron =
    std::find(typeRange.cbegin(), typeRange.cend(), VTK_POLYHEDRON) != typeRange.cend();

  if (!containPolyhedron)
  {
    this->SetCells(cellTypes, cells, nullptr, nullptr);
    return;
  }

  // If a polyhedron cell exists, its input cellArray is of special format.
  // [nCell0Faces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
  // We need to convert it into new cell connectivities of standard format,
  // update cellLocations as well as create faces and facelocations.
  vtkNew<vtkCellArray> newCells;
  newCells->AllocateExact(ncells, cells->GetNumberOfConnectivityIds());

  vtkNew<vtkIdTypeArray> faces;
  faces->Allocate(ncells + cells->GetNumberOfConnectivityIds());

  vtkNew<vtkIdTypeArray> faceLocations;
  faceLocations->Allocate(ncells);

  auto cellIter = vtkSmartPointer<vtkCellArrayIterator>::Take(cells->NewIterator());

  for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellIter->GetCurrentCell(npts, pts);
    const vtkIdType cellId = cellIter->GetCurrentCellId();

    if (cellTypes->GetValue(cellId) != VTK_POLYHEDRON)
    {
      newCells->InsertNextCell(npts, pts);
      faceLocations->InsertNextValue(-1);
    }
    else
    {
      vtkIdType realnpts;
      vtkIdType nfaces;
      faceLocations->InsertNextValue(faces->GetMaxId() + 1);
      vtkUnstructuredGrid::DecomposeAPolyhedronCell(pts, realnpts, nfaces, newCells, faces);
    }
  }

  this->SetCells(cellTypes, newCells, faceLocations, faces);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::SetCells(vtkUnsignedCharArray* cellTypes, vtkCellArray* cells,
  vtkIdTypeArray* faceLocations, vtkIdTypeArray* faces)
{
  this->Connectivity = cells;
  this->Types = cellTypes;
  this->DistinctCellTypes = nullptr;
  this->DistinctCellTypesUpdateMTime = 0;
  this->Faces = faces;
  this->FaceLocations = faceLocations;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::BuildLinks()
{
  // Create appropriate locator. Currently it's either a vtkCellLocator (when
  // the dataset is editable) or vtkStaticCellLocator (when the dataset is
  // not editable).
  vtkIdType numPts = this->GetNumberOfPoints();
  if (!this->Editable)
  {
    this->Links = vtkSmartPointer<vtkStaticCellLinks>::New();
  }
  else
  {
    vtkNew<vtkCellLinks> links;
    links->Allocate(numPts);
    this->Links = std::move(links);
  }

  this->Links->BuildLinks(this);
}

//----------------------------------------------------------------------------
vtkAbstractCellLinks* vtkUnstructuredGrid::GetCellLinks()
{
  return this->Links;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetPointCells(vtkIdType ptId, vtkIdType& ncells, vtkIdType*& cells)
{
  if (!this->Editable)
  {
    vtkStaticCellLinks* links = static_cast<vtkStaticCellLinks*>(this->Links.Get());

    ncells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }
  else
  {
    vtkCellLinks* links = static_cast<vtkCellLinks*>(this->Links.Get());

    ncells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }
}

//----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkUnstructuredGrid::GetPointCells(vtkIdType ptId, unsigned short& ncells, vtkIdType*& cells)
{
  VTK_LEGACY_BODY(vtkUnstructuredGrid::GetPointCells, "VTK 9.0");
  vtkIdType nc;
  this->GetPointCells(ptId, nc, cells);
  ncells = static_cast<unsigned short>(nc);
}
#endif

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  this->Connectivity->GetCellAtId(cellId, ptIds);
}

namespace
{
class DistinctCellTypesWorker
{
public:
  DistinctCellTypesWorker(vtkUnstructuredGrid* grid)
    : Grid(grid)
  {
  }

  vtkUnstructuredGrid* Grid;
  std::set<unsigned char> DistinctCellTypes;

  // Thread-local storage
  vtkSMPThreadLocal<std::set<unsigned char> > LocalDistinctCellTypes;

  void Initialize() {}

  void operator()(vtkIdType begin, vtkIdType end)
  {
    if (!this->Grid)
    {
      return;
    }

    for (vtkIdType idx = begin; idx < end; ++idx)
    {
      unsigned char cellType = static_cast<unsigned char>(this->Grid->GetCellType(idx));
      this->LocalDistinctCellTypes.Local().insert(cellType);
    }
  }

  void Reduce()
  {
    this->DistinctCellTypes.clear();
    for (vtkSMPThreadLocal<std::set<unsigned char> >::iterator iter =
           this->LocalDistinctCellTypes.begin();
         iter != this->LocalDistinctCellTypes.end(); ++iter)
    {
      this->DistinctCellTypes.insert(iter->begin(), iter->end());
    }
  }
};
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetCellTypes(vtkCellTypes* types)
{
  if (this->Types == nullptr)
  {
    // No cell types
    return;
  }

  if (this->DistinctCellTypes == nullptr ||
    this->Types->GetMTime() > this->DistinctCellTypesUpdateMTime)
  {
    // Update the list of cell types
    DistinctCellTypesWorker cellTypesWorker(this);
    vtkSMPTools::For(0, this->GetNumberOfCells(), cellTypesWorker);

    if (this->DistinctCellTypes)
    {
      this->DistinctCellTypes->Reset();
    }
    else
    {
      this->DistinctCellTypes = vtkSmartPointer<vtkCellTypes>::New();
      this->DistinctCellTypes->Register(this);
      this->DistinctCellTypes->Delete();
    }
    this->DistinctCellTypes->Allocate(static_cast<int>(cellTypesWorker.DistinctCellTypes.size()));

    for (auto cellType : cellTypesWorker.DistinctCellTypes)
    {
      this->DistinctCellTypes->InsertNextType(cellType);
    }

    this->DistinctCellTypesUpdateMTime = this->Types->GetMTime();
  }

  types->DeepCopy(this->DistinctCellTypes);
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkUnstructuredGrid::GetCellTypesArray()
{
  return this->Types;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetFaceStream(vtkIdType cellId, vtkIdList* ptIds)
{
  if (this->GetCellType(cellId) != VTK_POLYHEDRON)
  {
    this->GetCellPoints(cellId, ptIds);
    return;
  }

  ptIds->Reset();

  if (!this->Faces || !this->FaceLocations)
  {
    return;
  }

  vtkIdType loc = this->FaceLocations->GetValue(cellId);
  vtkIdType* facePtr = this->Faces->GetPointer(loc);

  vtkIdType nfaces = *facePtr++;
  ptIds->InsertNextId(nfaces);
  for (vtkIdType i = 0; i < nfaces; i++)
  {
    vtkIdType npts = *facePtr++;
    ptIds->InsertNextId(npts);
    for (vtkIdType j = 0; j < npts; j++)
    {
      ptIds->InsertNextId(*facePtr++);
    }
  }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetFaceStream(
  vtkIdType cellId, vtkIdType& nfaces, vtkIdType const*& ptIds)
{
  if (this->GetCellType(cellId) != VTK_POLYHEDRON)
  {
    this->GetCellPoints(cellId, nfaces, ptIds);
    return;
  }

  if (!this->Faces || !this->FaceLocations)
  {
    return;
  }

  vtkIdType loc = this->FaceLocations->GetValue(cellId);
  const vtkIdType* facePtr = this->Faces->GetPointer(loc);

  nfaces = *facePtr;
  ptIds = facePtr + 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::GetPointCells(vtkIdType ptId, vtkIdList* cellIds)
{
  if (!this->Links)
  {
    this->BuildLinks();
  }
  cellIds->Reset();

  vtkIdType numCells, *cells;
  if (!this->Editable)
  {
    vtkStaticCellLinks* links = static_cast<vtkStaticCellLinks*>(this->Links.Get());
    numCells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }
  else
  {
    vtkCellLinks* links = static_cast<vtkCellLinks*>(this->Links.Get());
    numCells = links->GetNcells(ptId);
    cells = links->GetCells(ptId);
  }

  cellIds->SetNumberOfIds(numCells);
  for (auto i = 0; i < numCells; i++)
  {
    cellIds->SetId(i, cells[i]);
  }
}

//----------------------------------------------------------------------------
vtkCellIterator* vtkUnstructuredGrid::NewCellIterator()
{
  vtkUnstructuredGridCellIterator* iter(vtkUnstructuredGridCellIterator::New());
  iter->SetUnstructuredGrid(this);
  return iter;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::Reset()
{
  if (this->Connectivity)
  {
    this->Connectivity->Reset();
  }
  if (this->Links)
  {
    this->Links->Reset();
  }
  if (this->Types)
  {
    this->Types->Reset();
  }
  if (this->DistinctCellTypes)
  {
    this->DistinctCellTypes->Reset();
  }
  if (this->Faces)
  {
    this->Faces->Reset();
  }
  if (this->FaceLocations)
  {
    this->FaceLocations->Reset();
  }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::Squeeze()
{
  if (this->Connectivity)
  {
    this->Connectivity->Squeeze();
  }
  if (this->Links)
  {
    this->Links->Squeeze();
  }
  if (this->Types)
  {
    this->Types->Squeeze();
  }
  if (this->Faces)
  {
    this->Faces->Squeeze();
  }
  if (this->FaceLocations)
  {
    this->FaceLocations->Squeeze();
  }

  vtkPointSet::Squeeze();
}

//----------------------------------------------------------------------------
// Remove a reference to a cell in a particular point's link list. You may
// also consider using RemoveCellReference() to remove the references from
// all the cell's points to the cell. This operator does not reallocate
// memory; use the operator ResizeCellList() to do this if necessary. Note that
// dataset should be set to "Editable".
void vtkUnstructuredGrid::RemoveReferenceToCell(vtkIdType ptId, vtkIdType cellId)
{
  static_cast<vtkCellLinks*>(this->Links.Get())->RemoveCellReference(cellId, ptId);
}

//----------------------------------------------------------------------------
// Add a reference to a cell in a particular point's link list. (You may also
// consider using AddCellReference() to add the references from all the
// cell's points to the cell.) This operator does not realloc memory; use the
// operator ResizeCellList() to do this if necessary. Note that dataset
// should be set to "Editable".
void vtkUnstructuredGrid::AddReferenceToCell(vtkIdType ptId, vtkIdType cellId)
{
  static_cast<vtkCellLinks*>(this->Links.Get())->AddCellReference(cellId, ptId);
}

//----------------------------------------------------------------------------
// Resize the list of cells using a particular point. (This operator assumes
// that BuildLinks() has been called.) Note that dataset should be set to
// "Editable".
void vtkUnstructuredGrid::ResizeCellList(vtkIdType ptId, int size)
{
  static_cast<vtkCellLinks*>(this->Links.Get())->ResizeCellList(ptId, size);
}

//----------------------------------------------------------------------------
// Replace the points defining cell "cellId" with a new set of points. This
// operator is (typically) used when links from points to cells have not been
// built (i.e., BuildLinks() has not been executed). Use the operator
// ReplaceLinkedCell() to replace a cell when cell structure has been built.
void vtkUnstructuredGrid::InternalReplaceCell(vtkIdType cellId, int npts, const vtkIdType pts[])
{
  this->Connectivity->ReplaceCellAtId(cellId, npts, pts);
}

//----------------------------------------------------------------------------
// Add a new cell to the cell data structure (after cell links have been
// built). This method adds the cell and then updates the links from the points
// to the cells. (Memory is allocated as necessary.) Note that the dataset must
// be in "Editable" mode.
vtkIdType vtkUnstructuredGrid::InsertNextLinkedCell(int type, int npts, const vtkIdType pts[])
{
  vtkIdType i, id;

  id = this->InsertNextCell(type, npts, pts);

  vtkCellLinks* clinks = static_cast<vtkCellLinks*>(this->Links.Get());
  for (i = 0; i < npts; i++)
  {
    clinks->ResizeCellList(pts[i], 1);
    clinks->AddCellReference(id, pts[i]);
  }

  return id;
}

//----------------------------------------------------------------------------
unsigned long vtkUnstructuredGrid::GetActualMemorySize()
{
  unsigned long size = this->vtkPointSet::GetActualMemorySize();
  if (this->Connectivity)
  {
    size += this->Connectivity->GetActualMemorySize();
  }

  if (this->Links)
  {
    size += this->Links->GetActualMemorySize();
  }

  if (this->Types)
  {
    size += this->Types->GetActualMemorySize();
  }

  if (this->Faces)
  {
    size += this->Faces->GetActualMemorySize();
  }

  if (this->FaceLocations)
  {
    size += this->FaceLocations->GetActualMemorySize();
  }

  return size;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::ShallowCopy(vtkDataObject* dataObject)
{
  if (vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(dataObject))
  {
    // I do not know if this is correct but.
    // ^ I really hope this comment lives for another 20 years.

    this->Connectivity = grid->Connectivity;
    this->Links = grid->Links;
    this->Types = grid->Types;
    this->DistinctCellTypes = nullptr;
    this->DistinctCellTypesUpdateMTime = 0;
    this->Faces = grid->Faces;
    this->FaceLocations = grid->FaceLocations;
  }
  else if (vtkUnstructuredGridBase* ugb = vtkUnstructuredGridBase::SafeDownCast(dataObject))
  {
    // The source object has vtkUnstructuredGrid topology, but a different
    // cell implementation. Deep copy the cells, and shallow copy the rest:
    vtkSmartPointer<vtkCellIterator> cellIter =
      vtkSmartPointer<vtkCellIterator>::Take(ugb->NewCellIterator());
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      this->InsertNextCell(cellIter->GetCellType(), cellIter->GetNumberOfPoints(),
        cellIter->GetPointIds()->GetPointer(0), cellIter->GetNumberOfFaces(),
        cellIter->GetFaces()->GetPointer(1));
    }
  }

  this->Superclass::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::DeepCopy(vtkDataObject* dataObject)
{
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(dataObject);

  if (grid != nullptr)
  {
    if (grid->Connectivity)
    {
      this->Connectivity = vtkSmartPointer<vtkCellArray>::New();
      this->Connectivity->DeepCopy(grid->Connectivity);
    }
    else
    {
      this->Connectivity = nullptr;
    }

    if (grid->Types)
    {
      this->Types = vtkSmartPointer<vtkUnsignedCharArray>::New();
      this->Types->DeepCopy(grid->Types);
    }
    else
    {
      this->Types = nullptr;
    }

    if (grid->DistinctCellTypes)
    {
      this->DistinctCellTypes = vtkSmartPointer<vtkCellTypes>::New();
      this->DistinctCellTypes->DeepCopy(grid->DistinctCellTypes);
    }
    else
    {
      this->DistinctCellTypes = nullptr;
    }

    if (grid->Faces)
    {
      this->Faces = vtkSmartPointer<vtkIdTypeArray>::New();
      this->Faces->DeepCopy(grid->Faces);
    }
    else
    {
      this->Faces = nullptr;
    }

    if (grid->FaceLocations)
    {
      this->FaceLocations = vtkSmartPointer<vtkIdTypeArray>::New();
      this->FaceLocations->DeepCopy(grid->FaceLocations);
    }
    else
    {
      this->FaceLocations = nullptr;
    }

    // Skip the unstructured grid base implementation, as it uses a less
    // efficient method of copying cell data.
    this->vtkUnstructuredGridBase::Superclass::DeepCopy(grid);
  }
  else
  {
    // Use the vtkUnstructuredGridBase deep copy implementation.
    this->Superclass::DeepCopy(dataObject);
  }

  // Finally Build Links if we need to
  if (grid && grid->Links)
  {
    this->BuildLinks();
  }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number Of Pieces: " << this->GetNumberOfPieces() << endl;
  os << indent << "Piece: " << this->GetPiece() << endl;
  os << indent << "Ghost Level: " << this->GetGhostLevel() << endl;
}

//----------------------------------------------------------------------------
bool vtkUnstructuredGrid::AllocateExact(vtkIdType numCells, vtkIdType connectivitySize)
{
  if (numCells < 1)
  {
    numCells = 1024;
  }
  if (connectivitySize < 1)
  {
    connectivitySize = 1024;
  }

  this->DistinctCellTypesUpdateMTime = 0;
  this->DistinctCellTypes = vtkSmartPointer<vtkCellTypes>::New();
  this->Types = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->Connectivity = vtkSmartPointer<vtkCellArray>::New();

  bool result = this->Connectivity->AllocateExact(numCells, connectivitySize);
  if (result)
  {
    result = this->Types->Allocate(numCells) != 0;
  }
  if (result)
  {
    result = this->DistinctCellTypes->Allocate(VTK_NUMBER_OF_CELL_TYPES) != 0;
  }

  return result;
}

//----------------------------------------------------------------------------
// Return the cells that use the ptIds provided. This is a set (intersection)
// operation - it can have significant performance impacts on certain filters
// like vtkGeometryFilter. It would be nice to make this faster.
void vtkUnstructuredGrid::GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds)
{
  // Ensure links are built.
  if (!this->Links)
  {
    this->BuildLinks();
  }
  cellIds->Reset();

  // Ensure that a proper neighborhood request is made.
  vtkIdType numPts = ptIds->GetNumberOfIds();
  if (numPts <= 0)
  {
    return;
  }

  vtkIdType numCells, ptId, *pts = ptIds->GetPointer(0);
  int minNumCells = VTK_INT_MAX;
  vtkIdType* minCells = nullptr;
  vtkIdType minPtId = 0;

  // Find the point used by the fewest number of cells as a starting set.
  // Note the explicit cast to locator type. Several experiments were
  // undertaken using virtual methods, switch statements, etc. but there were
  // significant performance impacts. This includes using different
  // instantiations of vtkStaticCellLinksTemplate<> of various types (to
  // reduce memory footprint) - the lesson learned is that the code here is
  // very sensitive to compiler optimizations so if you make changes, make
  // sure to test the impacts on performance.
  if (!this->Editable)
  {
    vtkStaticCellLinks* links = static_cast<vtkStaticCellLinks*>(this->Links.Get());

    for (auto i = 0; i < numPts; ++i)
    {
      ptId = pts[i];
      numCells = links->GetNcells(ptId);
      if (numCells < minNumCells)
      {
        minNumCells = numCells;
        minPtId = ptId;
      }
    }
    minCells = links->GetCells(minPtId);
  }
  else
  {
    vtkCellLinks* links = static_cast<vtkCellLinks*>(this->Links.Get());

    for (auto i = 0; i < numPts; ++i)
    {
      ptId = pts[i];
      numCells = links->GetNcells(ptId);
      if (numCells < minNumCells)
      {
        minNumCells = numCells;
        minPtId = ptId;
      }
    }
    minCells = links->GetCells(minPtId);
  }

  // Now for each cell, see if it contains all the points
  // in the ptIds list. If so, add the cellId to the neighbor list.
  bool match;
  for (auto i = 0; i < minNumCells; ++i)
  {
    if (minCells[i] != cellId) // don't include current cell
    {
      const vtkIdType* cellPts;
      vtkIdType npts;
      this->GetCellPoints(minCells[i], npts, cellPts);
      match = true;
      for (auto j = 0; j < numPts && match; ++j) // for all pts in input cell
      {
        if (pts[j] != minPtId) // of course minPtId is contained by cell
        {
          match = false;
          for (auto k = 0; k < npts; ++k) // for all points in candidate cell
          {
            if (pts[j] == cellPts[k])
            {
              match = true; // a match was found
              break;
            }
          } // for all points in current cell
        }   // if not guaranteed match
      }     // for all points in input cell
      if (match)
      {
        cellIds->InsertNextId(minCells[i]);
      }
    } // if not the reference cell
  }   // for all candidate cells attached to point
}

//----------------------------------------------------------------------------
int vtkUnstructuredGrid::IsHomogeneous()
{
  unsigned char type;
  if (this->Types && this->Types->GetMaxId() >= 0)
  {
    type = Types->GetValue(0);
    vtkIdType numCells = this->GetNumberOfCells();
    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
      if (this->Types->GetValue(cellId) != type)
      {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
// Fill container with indices of cells which match given type.
void vtkUnstructuredGrid::GetIdsOfCellsOfType(int type, vtkIdTypeArray* array)
{
  for (int cellId = 0; cellId < this->GetNumberOfCells(); cellId++)
  {
    if (static_cast<int>(Types->GetValue(cellId)) == type)
    {
      array->InsertNextValue(cellId);
    }
  }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::RemoveGhostCells()
{
  vtkUnstructuredGrid* newGrid = vtkUnstructuredGrid::New();
  vtkUnsignedCharArray* temp;
  unsigned char* cellGhosts;

  vtkIdType cellId, newCellId;
  vtkIdList *cellPts, *pointMap;
  vtkIdList* newCellPts;
  vtkCell* cell;
  vtkPoints* newPoints;
  vtkIdType i, ptId, newId, numPts;
  vtkIdType numCellPts;
  double* x;
  vtkPointData* pd = this->GetPointData();
  vtkPointData* outPD = newGrid->GetPointData();
  vtkCellData* cd = this->GetCellData();
  vtkCellData* outCD = newGrid->GetCellData();

  // Get a pointer to the cell ghost array.
  temp = this->GetCellGhostArray();
  if (temp == nullptr)
  {
    vtkDebugMacro("Could not find cell ghost array.");
    newGrid->Delete();
    return;
  }
  if ((temp->GetNumberOfComponents() != 1) ||
    (temp->GetNumberOfTuples() < this->GetNumberOfCells()))
  {
    vtkErrorMacro("Poorly formed ghost array.");
    newGrid->Delete();
    return;
  }
  cellGhosts = temp->GetPointer(0);

  // Now threshold based on the cell ghost array.

  // ensure that all attributes are copied over, including global ids.
  outPD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  outCD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);

  outPD->CopyAllocate(pd);
  outCD->CopyAllocate(cd);

  numPts = this->GetNumberOfPoints();
  newGrid->Allocate(this->GetNumberOfCells());
  newPoints = vtkPoints::New();
  newPoints->SetDataType(this->GetPoints()->GetDataType());
  newPoints->Allocate(numPts);

  pointMap = vtkIdList::New(); // maps old point ids into new
  pointMap->SetNumberOfIds(numPts);
  for (i = 0; i < numPts; i++)
  {
    pointMap->SetId(i, -1);
  }

  newCellPts = vtkIdList::New();

  // Check that the scalars of each cell satisfy the threshold criterion
  for (cellId = 0; cellId < this->GetNumberOfCells(); cellId++)
  {
    cell = this->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    if ((cellGhosts[cellId] & vtkDataSetAttributes::DUPLICATECELL) == 0) // Keep the cell.
    {
      for (i = 0; i < numCellPts; i++)
      {
        ptId = cellPts->GetId(i);
        if ((newId = pointMap->GetId(ptId)) < 0)
        {
          x = this->GetPoint(ptId);
          newId = newPoints->InsertNextPoint(x);
          pointMap->SetId(ptId, newId);
          outPD->CopyData(pd, ptId, newId);
        }
        newCellPts->InsertId(i, newId);
      }
      newCellId = newGrid->InsertNextCell(cell->GetCellType(), newCellPts);
      outCD->CopyData(cd, cellId, newCellId);
      newCellPts->Reset();
    } // satisfied thresholding
  }   // for all cells

  // now clean up / update ourselves
  pointMap->Delete();
  newCellPts->Delete();

  newGrid->SetPoints(newPoints);
  newPoints->Delete();

  this->CopyStructure(newGrid);
  this->GetPointData()->ShallowCopy(newGrid->GetPointData());
  this->GetCellData()->ShallowCopy(newGrid->GetCellData());
  newGrid->Delete();
  newGrid = nullptr;

  this->Squeeze();
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(vtkCellArray* polyhedronCell,
  vtkIdType& numCellPts, vtkIdType& nCellfaces, vtkCellArray* cellArray, vtkIdTypeArray* faces)
{
  const vtkIdType* cellStream = nullptr;
  vtkIdType cellLength = 0;

  polyhedronCell->InitTraversal();
  polyhedronCell->GetNextCell(cellLength, cellStream);

  vtkUnstructuredGrid::DecomposeAPolyhedronCell(
    cellStream, numCellPts, nCellfaces, cellArray, faces);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(const vtkIdType* cellStream,
  vtkIdType& numCellPts, vtkIdType& nCellFaces, vtkCellArray* cellArray, vtkIdTypeArray* faces)
{
  nCellFaces = cellStream[0];
  if (nCellFaces <= 0)
  {
    return;
  }

  vtkUnstructuredGrid::DecomposeAPolyhedronCell(
    nCellFaces, cellStream + 1, numCellPts, cellArray, faces);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::DecomposeAPolyhedronCell(vtkIdType nCellFaces,
  const vtkIdType cellStream[], vtkIdType& numCellPts, vtkCellArray* cellArray,
  vtkIdTypeArray* faces)
{
  std::set<vtkIdType> cellPointSet;
  std::set<vtkIdType>::iterator it;

  // insert number of faces into the face array
  faces->InsertNextValue(nCellFaces);

  // for each face
  for (vtkIdType fid = 0; fid < nCellFaces; fid++)
  {
    // extract all points on the same face, store them into a set
    vtkIdType npts = *cellStream++;
    faces->InsertNextValue(npts);
    for (vtkIdType i = 0; i < npts; i++)
    {
      vtkIdType pid = *cellStream++;
      faces->InsertNextValue(pid);
      cellPointSet.insert(pid);
    }
  }

  // standard cell connectivity array that stores the number of points plus
  // a list of point ids.
  cellArray->InsertNextCell(static_cast<int>(cellPointSet.size()));
  for (it = cellPointSet.begin(); it != cellPointSet.end(); ++it)
  {
    cellArray->InsertCellPoint(*it);
  }

  // the real number of points in the polyhedron cell.
  numCellPts = static_cast<vtkIdType>(cellPointSet.size());
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::ConvertFaceStreamPointIds(vtkIdList* faceStream, vtkIdType* idMap)
{
  vtkIdType* idPtr = faceStream->GetPointer(0);
  vtkIdType nfaces = *idPtr++;
  for (vtkIdType i = 0; i < nfaces; i++)
  {
    vtkIdType npts = *idPtr++;
    for (vtkIdType j = 0; j < npts; j++)
    {
      *idPtr = idMap[*idPtr];
      idPtr++;
    }
  }
}

//----------------------------------------------------------------------------
void vtkUnstructuredGrid::ConvertFaceStreamPointIds(
  vtkIdType nfaces, vtkIdType* faceStream, vtkIdType* idMap)
{
  vtkIdType* idPtr = faceStream;
  for (vtkIdType i = 0; i < nfaces; i++)
  {
    vtkIdType npts = *idPtr++;
    for (vtkIdType j = 0; j < npts; j++)
    {
      *idPtr = idMap[*idPtr];
      idPtr++;
    }
  }
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGrid::GetData(vtkInformation* info)
{
  return info ? vtkUnstructuredGrid::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGrid::GetData(vtkInformationVector* v, int i)
{
  return vtkUnstructuredGrid::GetData(v->GetInformationObject(i));
}
