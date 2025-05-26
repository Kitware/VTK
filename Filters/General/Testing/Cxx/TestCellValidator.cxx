// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This tests vtkCellValidator specialized `Check` methods.

#include <vtkCellValidator.h>

#include "vtkEmptyCell.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkLine.h"
#include "vtkPentagonalPrism.h"
#include "vtkPixel.h"
#include "vtkPolyLine.h"
#include "vtkPolyVertex.h"
#include "vtkPolygon.h"
#include "vtkPolyhedron.h"
#include "vtkPyramid.h"
#include "vtkQuad.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

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

#include "vtkBiQuadraticQuad.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkTriQuadraticPyramid.h"

#include "vtkCubicLine.h"

#include "vtkLagrangeCurve.h"
#include "vtkLagrangeHexahedron.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLagrangeTetra.h"
#include "vtkLagrangeTriangle.h"
#include "vtkLagrangeWedge.h"

#include "vtkBezierCurve.h"
#include "vtkBezierHexahedron.h"
#include "vtkBezierQuadrilateral.h"
#include "vtkBezierTetra.h"
#include "vtkBezierTriangle.h"
#include "vtkBezierWedge.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

#include <vtkActor.h>
#include <vtkDataSetMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

static vtkSmartPointer<vtkEmptyCell> MakeEmptyCell();
static vtkSmartPointer<vtkVertex> MakeVertex();
static vtkSmartPointer<vtkPolyVertex> MakePolyVertex();
static vtkSmartPointer<vtkLine> MakeLine();
static vtkSmartPointer<vtkPolyLine> MakePolyLine();
static vtkSmartPointer<vtkTriangle> MakeTriangle();
static vtkSmartPointer<vtkTriangleStrip> MakeTriangleStrip();
static vtkSmartPointer<vtkPolygon> MakePolygon();
static vtkSmartPointer<vtkQuad> MakeQuad();
static vtkSmartPointer<vtkPixel> MakePixel();
static vtkSmartPointer<vtkVoxel> MakeVoxel();
static vtkSmartPointer<vtkHexahedron> MakeHexahedron();
static vtkSmartPointer<vtkHexahedron> MakeHexahedronConvexityNonTrivial();
static vtkSmartPointer<vtkHexahedron> MakeBrokenHexahedron();
static vtkSmartPointer<vtkPyramid> MakePyramid();
static vtkSmartPointer<vtkTetra> MakeTetra();
static vtkSmartPointer<vtkWedge> MakeWedge();
static vtkSmartPointer<vtkPentagonalPrism> MakePentagonalPrism();
static vtkSmartPointer<vtkHexagonalPrism> MakeHexagonalPrism();
static vtkSmartPointer<vtkPolyhedron> MakeCube();
static vtkSmartPointer<vtkPolyhedron> MakeDodecahedron();
static vtkSmartPointer<vtkPolyhedron> MakePolyhedralWedge(bool goodOrBad);

static vtkSmartPointer<vtkQuadraticEdge> MakeQuadraticEdge();
static vtkSmartPointer<vtkQuadraticHexahedron> MakeQuadraticHexahedron();
static vtkSmartPointer<vtkQuadraticPolygon> MakeQuadraticPolygon();
static vtkSmartPointer<vtkQuadraticLinearQuad> MakeQuadraticLinearQuad();
static vtkSmartPointer<vtkQuadraticLinearWedge> MakeQuadraticLinearWedge();
static vtkSmartPointer<vtkQuadraticPyramid> MakeQuadraticPyramid();
static vtkSmartPointer<vtkQuadraticQuad> MakeQuadraticQuad();
static vtkSmartPointer<vtkQuadraticTetra> MakeQuadraticTetra();
static vtkSmartPointer<vtkQuadraticTriangle> MakeQuadraticTriangle();
static vtkSmartPointer<vtkQuadraticWedge> MakeQuadraticWedge();

static vtkSmartPointer<vtkBiQuadraticQuad> MakeBiQuadraticQuad();
static vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron> MakeBiQuadraticQuadraticHexahedron();
static vtkSmartPointer<vtkBiQuadraticQuadraticWedge> MakeBiQuadraticQuadraticWedge();
static vtkSmartPointer<vtkBiQuadraticTriangle> MakeBiQuadraticTriangle();
static vtkSmartPointer<vtkTriQuadraticHexahedron> MakeTriQuadraticHexahedron();
static vtkSmartPointer<vtkTriQuadraticPyramid> MakeTriQuadraticPyramid();
static vtkSmartPointer<vtkCubicLine> MakeCubicLine();

static vtkSmartPointer<vtkLagrangeCurve> MakeLagrangeCurve();
static vtkSmartPointer<vtkLagrangeTriangle> MakeLagrangeTriangle();
static vtkSmartPointer<vtkLagrangeTriangle> MakeBrokenLagrangeTriangle();
static vtkSmartPointer<vtkLagrangeQuadrilateral> MakeLagrangeQuadrilateral();
static vtkSmartPointer<vtkLagrangeTetra> MakeLagrangeTetra();
static vtkSmartPointer<vtkLagrangeHexahedron> MakeLagrangeHexahedron();
static vtkSmartPointer<vtkLagrangeWedge> MakeLagrangeWedge();

static vtkSmartPointer<vtkBezierCurve> MakeBezierCurve();
static vtkSmartPointer<vtkBezierTriangle> MakeBezierTriangle();
static vtkSmartPointer<vtkBezierQuadrilateral> MakeBezierQuadrilateral();
static vtkSmartPointer<vtkBezierTetra> MakeBezierTetra();
static vtkSmartPointer<vtkBezierHexahedron> MakeBezierHexahedron();
static vtkSmartPointer<vtkBezierWedge> MakeBezierWedge();
//------------------------------------------------------------------------------

int TestCellValidator(int, char*[])
{
  vtkSmartPointer<vtkEmptyCell> emptyCell = MakeEmptyCell();
  vtkSmartPointer<vtkVertex> vertex = MakeVertex();
  vtkSmartPointer<vtkPolyVertex> polyVertex = MakePolyVertex();
  vtkSmartPointer<vtkLine> line = MakeLine();
  vtkSmartPointer<vtkPolyLine> polyLine = MakePolyLine();
  vtkSmartPointer<vtkTriangle> triangle = MakeTriangle();
  vtkSmartPointer<vtkTriangleStrip> triangleStrip = MakeTriangleStrip();
  vtkSmartPointer<vtkPolygon> polygon = MakePolygon();
  vtkSmartPointer<vtkQuad> quad = MakeQuad();
  vtkSmartPointer<vtkPixel> pixel = MakePixel();
  vtkSmartPointer<vtkVoxel> voxel = MakeVoxel();
  vtkSmartPointer<vtkHexahedron> hexahedron = MakeHexahedron();
  vtkSmartPointer<vtkHexahedron> hexahedronConvexityNonTrivial =
    MakeHexahedronConvexityNonTrivial();
  vtkSmartPointer<vtkPyramid> pyramid = MakePyramid();
  vtkSmartPointer<vtkTetra> tetra = MakeTetra();
  vtkSmartPointer<vtkWedge> wedge = MakeWedge();
  vtkSmartPointer<vtkPentagonalPrism> pentagonalPrism = MakePentagonalPrism();
  vtkSmartPointer<vtkHexagonalPrism> hexagonalPrism = MakeHexagonalPrism();
  vtkSmartPointer<vtkPolyhedron> poly1 = MakeCube();
  vtkSmartPointer<vtkPolyhedron> poly2 = MakeDodecahedron();
  vtkSmartPointer<vtkPolyhedron> poly3 = MakePolyhedralWedge(true);  // good
  vtkSmartPointer<vtkPolyhedron> poly4 = MakePolyhedralWedge(false); // broken

  vtkSmartPointer<vtkQuadraticEdge> quadraticEdge = MakeQuadraticEdge();
  vtkSmartPointer<vtkQuadraticHexahedron> quadraticHexahedron = MakeQuadraticHexahedron();
  vtkSmartPointer<vtkQuadraticPolygon> quadraticPolygon = MakeQuadraticPolygon();
  vtkSmartPointer<vtkQuadraticLinearQuad> quadraticLinearQuad = MakeQuadraticLinearQuad();
  vtkSmartPointer<vtkQuadraticLinearWedge> quadraticLinearWedge = MakeQuadraticLinearWedge();
  vtkSmartPointer<vtkQuadraticPyramid> quadraticPyramid = MakeQuadraticPyramid();
  vtkSmartPointer<vtkQuadraticQuad> quadraticQuad = MakeQuadraticQuad();
  vtkSmartPointer<vtkQuadraticTetra> quadraticTetra = MakeQuadraticTetra();
  vtkSmartPointer<vtkQuadraticTriangle> quadraticTriangle = MakeQuadraticTriangle();
  vtkSmartPointer<vtkQuadraticWedge> quadraticWedge = MakeQuadraticWedge();

  vtkSmartPointer<vtkBiQuadraticQuad> biQuadraticQuad = MakeBiQuadraticQuad();
  vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron> biQuadraticQuadraticHexahedron =
    MakeBiQuadraticQuadraticHexahedron();
  vtkSmartPointer<vtkBiQuadraticQuadraticWedge> biQuadraticQuadraticWedge =
    MakeBiQuadraticQuadraticWedge();
  vtkSmartPointer<vtkBiQuadraticTriangle> biQuadraticTriangle = MakeBiQuadraticTriangle();
  vtkSmartPointer<vtkTriQuadraticHexahedron> triQuadraticHexahedron = MakeTriQuadraticHexahedron();
  vtkSmartPointer<vtkTriQuadraticPyramid> triQuadraticPyramid = MakeTriQuadraticPyramid();
  vtkSmartPointer<vtkCubicLine> cubicLine = MakeCubicLine();

  vtkSmartPointer<vtkLagrangeCurve> lagrangeCurve = MakeLagrangeCurve();
  vtkSmartPointer<vtkLagrangeTriangle> lagrangeTriangle = MakeLagrangeTriangle();
  vtkSmartPointer<vtkLagrangeTriangle> brokenLagrangeTriangle = MakeBrokenLagrangeTriangle();
  vtkSmartPointer<vtkLagrangeQuadrilateral> lagrangeQuadrilateral = MakeLagrangeQuadrilateral();
  vtkSmartPointer<vtkLagrangeTetra> lagrangeTetra = MakeLagrangeTetra();
  vtkSmartPointer<vtkLagrangeHexahedron> lagrangeHexahedron = MakeLagrangeHexahedron();
  vtkSmartPointer<vtkLagrangeWedge> lagrangeWedge = MakeLagrangeWedge();

  vtkSmartPointer<vtkBezierCurve> bezierCurve = MakeBezierCurve();
  vtkSmartPointer<vtkBezierTriangle> bezierTriangle = MakeBezierTriangle();
  vtkSmartPointer<vtkBezierQuadrilateral> bezierQuadrilateral = MakeBezierQuadrilateral();
  vtkSmartPointer<vtkBezierTetra> bezierTetra = MakeBezierTetra();
  vtkSmartPointer<vtkBezierHexahedron> bezierHexahedron = MakeBezierHexahedron();
  vtkSmartPointer<vtkBezierWedge> bezierWedge = MakeBezierWedge();

  vtkCellValidator::State state;

#define CheckCell(cellPtr)                                                                         \
  do                                                                                               \
  {                                                                                                \
    state = vtkCellValidator::Check(cellPtr, FLT_EPSILON);                                         \
    if (state != vtkCellValidator::State::Valid)                                                   \
    {                                                                                              \
      cellPtr->Print(std::cout);                                                                   \
      vtkCellValidator::PrintState(state, std::cout, vtkIndent(0));                                \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

  CheckCell(emptyCell);
  CheckCell(vertex);
  CheckCell(polyVertex);
  CheckCell(line);
  CheckCell(polyLine);
  CheckCell(triangle);
  CheckCell(triangleStrip);
  CheckCell(polygon);
  CheckCell(pixel);
  CheckCell(quad);
  CheckCell(tetra);
  CheckCell(voxel);
  CheckCell(hexahedron);
  CheckCell(hexahedronConvexityNonTrivial);
  CheckCell(wedge);
  CheckCell(pyramid);
  CheckCell(pentagonalPrism);
  CheckCell(hexagonalPrism);
  CheckCell(poly1);
  CheckCell(poly2);
  CheckCell(poly3);
  CheckCell(quadraticEdge);
  CheckCell(quadraticHexahedron);
  CheckCell(quadraticPolygon);
  CheckCell(quadraticLinearQuad);
  CheckCell(quadraticLinearWedge);
  CheckCell(quadraticPyramid);
  CheckCell(quadraticQuad);
  CheckCell(quadraticTetra);
  CheckCell(quadraticTriangle);
  CheckCell(quadraticWedge);
  CheckCell(quadraticWedge);
  CheckCell(biQuadraticQuad);
  CheckCell(biQuadraticQuadraticHexahedron);
  CheckCell(biQuadraticQuadraticWedge);
  CheckCell(biQuadraticTriangle);
  CheckCell(triQuadraticHexahedron);
  CheckCell(triQuadraticPyramid);
  CheckCell(cubicLine);
  CheckCell(lagrangeCurve);
  CheckCell(lagrangeTriangle);
  CheckCell(lagrangeQuadrilateral);
  CheckCell(lagrangeTetra);
  CheckCell(lagrangeHexahedron);
  CheckCell(lagrangeWedge);
  CheckCell(bezierCurve);
  CheckCell(bezierTriangle);
  CheckCell(bezierQuadrilateral);
  CheckCell(bezierTetra);
  CheckCell(bezierHexahedron);
  CheckCell(bezierWedge);
#undef CheckCell

  state = vtkCellValidator::Check(MakeBrokenHexahedron(), FLT_EPSILON);
  if ((state & vtkCellValidator::State::IntersectingEdges) !=
    vtkCellValidator::State::IntersectingEdges)
  {
    vtkCellValidator::PrintState(state, std::cout, vtkIndent(0));
    return EXIT_FAILURE;
  }

  state = vtkCellValidator::Check(MakeBrokenLagrangeTriangle(), FLT_EPSILON);
  if ((state & vtkCellValidator::State::IntersectingEdges) !=
    vtkCellValidator::State::IntersectingEdges)
  {
    vtkCellValidator::PrintState(state, std::cout, vtkIndent(0));
    return EXIT_FAILURE;
  }

  state = vtkCellValidator::Check(poly4, FLT_EPSILON);
  if ((state & vtkCellValidator::State::Nonconvex) != vtkCellValidator::State::Nonconvex)
  {
    vtkCellValidator::PrintState(state, std::cout, vtkIndent(0));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

vtkSmartPointer<vtkEmptyCell> MakeEmptyCell()
{
  auto anEmptyCell = vtkSmartPointer<vtkEmptyCell>::New();

  return anEmptyCell;
}

vtkSmartPointer<vtkVertex> MakeVertex()
{
  auto aVertex = vtkSmartPointer<vtkVertex>::New();

  aVertex->GetPointIds()->SetId(0, 0);
  aVertex->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);

  return aVertex;
}

vtkSmartPointer<vtkPolyVertex> MakePolyVertex()
{
  auto aPolyVertex = vtkSmartPointer<vtkPolyVertex>::New();

  aPolyVertex->GetPointIds()->SetNumberOfIds(2);
  aPolyVertex->GetPointIds()->SetId(0, 0);
  aPolyVertex->GetPointIds()->SetId(1, 1);

  aPolyVertex->GetPoints()->SetNumberOfPoints(2);
  aPolyVertex->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);
  aPolyVertex->GetPoints()->SetPoint(1, 30.0, 20.0, 10.0);

  return aPolyVertex;
}

vtkSmartPointer<vtkLine> MakeLine()
{
  auto aLine = vtkSmartPointer<vtkLine>::New();

  aLine->GetPointIds()->SetId(0, 0);
  aLine->GetPointIds()->SetId(1, 1);
  aLine->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);
  aLine->GetPoints()->SetPoint(1, 30.0, 20.0, 10.0);

  return aLine;
}

vtkSmartPointer<vtkPolyLine> MakePolyLine()
{
  auto aPolyLine = vtkSmartPointer<vtkPolyLine>::New();

  aPolyLine->GetPointIds()->SetNumberOfIds(3);
  aPolyLine->GetPointIds()->SetId(0, 0);
  aPolyLine->GetPointIds()->SetId(1, 1);
  aPolyLine->GetPointIds()->SetId(2, 2);

  aPolyLine->GetPoints()->SetNumberOfPoints(3);
  aPolyLine->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);
  aPolyLine->GetPoints()->SetPoint(1, 10.0, 30.0, 30.0);
  aPolyLine->GetPoints()->SetPoint(2, 10.0, 30.0, 40.0);

  return aPolyLine;
}

vtkSmartPointer<vtkTriangle> MakeTriangle()
{
  auto aTriangle = vtkSmartPointer<vtkTriangle>::New();

  for (int i = 0; i < aTriangle->GetNumberOfPoints(); ++i)
  {
    aTriangle->GetPointIds()->SetId(i, i);
  }

  aTriangle->GetPoints()->SetPoint(0, -10.0, -10.0, 0.0);
  aTriangle->GetPoints()->SetPoint(1, 10.0, -10.0, 0.0);
  aTriangle->GetPoints()->SetPoint(2, 10.0, 10.0, 0.0);

  return aTriangle;
}

vtkSmartPointer<vtkTriangleStrip> MakeTriangleStrip()
{
  auto aTriangleStrip = vtkSmartPointer<vtkTriangleStrip>::New();

  aTriangleStrip->GetPointIds()->SetNumberOfIds(4);
  aTriangleStrip->GetPointIds()->SetId(0, 0);
  aTriangleStrip->GetPointIds()->SetId(1, 1);
  aTriangleStrip->GetPointIds()->SetId(2, 2);
  aTriangleStrip->GetPointIds()->SetId(3, 3);

  aTriangleStrip->GetPoints()->SetNumberOfPoints(4);
  aTriangleStrip->GetPoints()->SetPoint(0, 10.0, 10.0, 10.0);
  aTriangleStrip->GetPoints()->SetPoint(1, 12.0, 10.0, 10.0);
  aTriangleStrip->GetPoints()->SetPoint(2, 11.0, 12.0, 10.0);
  aTriangleStrip->GetPoints()->SetPoint(3, 13.0, 10.0, 10.0);

  return aTriangleStrip;
}

vtkSmartPointer<vtkPolygon> MakePolygon()
{
  auto aPolygon = vtkSmartPointer<vtkPolygon>::New();

  aPolygon->GetPointIds()->SetNumberOfIds(4);
  aPolygon->GetPointIds()->SetId(0, 0);
  aPolygon->GetPointIds()->SetId(1, 1);
  aPolygon->GetPointIds()->SetId(2, 2);
  aPolygon->GetPointIds()->SetId(3, 3);

  aPolygon->GetPoints()->SetNumberOfPoints(4);
  aPolygon->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(1, 10.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(2, 10.0, 10.0, 0.0);
  aPolygon->GetPoints()->SetPoint(3, 0.0, 10.0, 0.0);

  return aPolygon;
}

vtkSmartPointer<vtkQuad> MakeQuad()
{
  auto aQuad = vtkSmartPointer<vtkQuad>::New();

  for (int i = 0; i < aQuad->GetNumberOfPoints(); ++i)
  {
    aQuad->GetPointIds()->SetId(i, i);
  }

  aQuad->GetPoints()->SetPoint(0, -10.0, -10.0, 0.0);
  aQuad->GetPoints()->SetPoint(1, 10.0, -10.0, 0.0);
  aQuad->GetPoints()->SetPoint(2, 10.0, 10.0, 0.0);
  aQuad->GetPoints()->SetPoint(3, -10.0, 10.0, 0.0);

  return aQuad;
}

vtkSmartPointer<vtkPixel> MakePixel()
{
  auto aPixel = vtkSmartPointer<vtkPixel>::New();

  for (int i = 0; i < aPixel->GetNumberOfPoints(); ++i)
  {
    aPixel->GetPointIds()->SetId(i, i);
  }

  aPixel->GetPoints()->SetPoint(0, 10.0, 10.0, 10.0);
  aPixel->GetPoints()->SetPoint(1, 12.0, 10.0, 10.0);
  aPixel->GetPoints()->SetPoint(2, 10.0, 12.0, 10.0);
  aPixel->GetPoints()->SetPoint(3, 12.0, 12.0, 10.0);

  return aPixel;
}

vtkSmartPointer<vtkVoxel> MakeVoxel()
{
  auto aVoxel = vtkSmartPointer<vtkVoxel>::New();

  for (int i = 0; i < aVoxel->GetNumberOfPoints(); ++i)
  {
    aVoxel->GetPointIds()->SetId(i, i);
  }

  aVoxel->GetPoints()->SetPoint(0, 10, 10, 10);
  aVoxel->GetPoints()->SetPoint(1, 12, 10, 10);
  aVoxel->GetPoints()->SetPoint(2, 10, 12, 10);
  aVoxel->GetPoints()->SetPoint(3, 12, 12, 10);
  aVoxel->GetPoints()->SetPoint(4, 10, 10, 12);
  aVoxel->GetPoints()->SetPoint(5, 12, 10, 12);
  aVoxel->GetPoints()->SetPoint(6, 10, 12, 12);
  aVoxel->GetPoints()->SetPoint(7, 12, 12, 12);

  return aVoxel;
}

vtkSmartPointer<vtkHexahedron> MakeHexahedron()
{
  auto aHexahedron = vtkSmartPointer<vtkHexahedron>::New();

  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
  }

  aHexahedron->GetPoints()->SetPoint(0, 10, 10, 10);
  aHexahedron->GetPoints()->SetPoint(1, 12, 10, 10);
  aHexahedron->GetPoints()->SetPoint(2, 12, 12, 10);
  aHexahedron->GetPoints()->SetPoint(3, 10, 12, 10);
  aHexahedron->GetPoints()->SetPoint(4, 10, 10, 12);
  aHexahedron->GetPoints()->SetPoint(5, 12, 10, 12);
  aHexahedron->GetPoints()->SetPoint(6, 12, 12, 12);
  aHexahedron->GetPoints()->SetPoint(7, 10, 12, 12);

  return aHexahedron;
}

vtkSmartPointer<vtkHexahedron> MakeHexahedronConvexityNonTrivial()
{
  // Example that was failing before, if now fixed and tested
  // https://gitlab.kitware.com/vtk/vtk/-/issues/17673
  auto aHexahedron = vtkSmartPointer<vtkHexahedron>::New();

  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
  }

  aHexahedron->GetPoints()->SetPoint(0, -2.9417226413, -0.92284313965, 4.5809917214);
  aHexahedron->GetPoints()->SetPoint(1, -3.0207607208, -0.84291999288, 4.357055109);
  aHexahedron->GetPoints()->SetPoint(2, -3.1077984177, -0.31259201362, 4.8124331347);
  aHexahedron->GetPoints()->SetPoint(3, -2.9320660211, -0.86238701507, 4.7197960612);
  aHexahedron->GetPoints()->SetPoint(4, -2.8375199741, -0.57697632408, 3.8069219868);
  aHexahedron->GetPoints()->SetPoint(5, -3.1669520923, -0.64026224489, 3.8129245089);
  aHexahedron->GetPoints()->SetPoint(6, -3.1935454463, -0.017891697066, 4.8277744194);
  aHexahedron->GetPoints()->SetPoint(7, -2.8265109805, -0.51675730395, 3.9006508868);

  return aHexahedron;
}

vtkSmartPointer<vtkHexahedron> MakeBrokenHexahedron()
{
  auto aHexahedron = vtkSmartPointer<vtkHexahedron>::New();

  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
  }

  aHexahedron->GetPoints()->SetPoint(1, 10, 10, 10);
  aHexahedron->GetPoints()->SetPoint(0, 12, 10, 10);
  aHexahedron->GetPoints()->SetPoint(2, 12, 12, 10);
  aHexahedron->GetPoints()->SetPoint(3, 10, 12, 10);
  aHexahedron->GetPoints()->SetPoint(4, 10, 10, 12);
  aHexahedron->GetPoints()->SetPoint(5, 12, 10, 12);
  aHexahedron->GetPoints()->SetPoint(6, 12, 12, 12);
  aHexahedron->GetPoints()->SetPoint(7, 10, 12, 12);

  return aHexahedron;
}

vtkSmartPointer<vtkPyramid> MakePyramid()
{
  auto aPyramid = vtkSmartPointer<vtkPyramid>::New();

  for (int i = 0; i < aPyramid->GetNumberOfPoints(); ++i)
  {
    aPyramid->GetPointIds()->SetId(i, i);
  }

  aPyramid->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  aPyramid->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  aPyramid->GetPoints()->SetPoint(2, 1.0, 1.0, 0.0);
  aPyramid->GetPoints()->SetPoint(3, 0.0, 1.0, 0.0);
  aPyramid->GetPoints()->SetPoint(4, 0.5, 0.5, 1.0);

  return aPyramid;
}

vtkSmartPointer<vtkQuadraticPyramid> MakeQuadraticPyramid()
{
  auto aPyramid = vtkSmartPointer<vtkQuadraticPyramid>::New();

  for (int i = 0; i < aPyramid->GetNumberOfPoints(); ++i)
  {
    aPyramid->GetPointIds()->SetId(i, i);
  }

  aPyramid->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  aPyramid->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  aPyramid->GetPoints()->SetPoint(2, 1.0, 1.0, 0.0);
  aPyramid->GetPoints()->SetPoint(3, 0.0, 1.0, 0.0);
  aPyramid->GetPoints()->SetPoint(4, 0.5, 0.5, 1.0);

  aPyramid->GetPoints()->SetPoint(5, 0.5, 0.0, 0.0);
  aPyramid->GetPoints()->SetPoint(6, 1.0, 0.5, 0.0);
  aPyramid->GetPoints()->SetPoint(7, 0.5, 1.0, 0.0);
  aPyramid->GetPoints()->SetPoint(8, 0.0, 0.5, 0.0);
  aPyramid->GetPoints()->SetPoint(9, 0.25, 0.25, 0.5);
  aPyramid->GetPoints()->SetPoint(10, 0.75, 0.25, 0.5);
  aPyramid->GetPoints()->SetPoint(11, 0.75, 0.75, 0.5);
  aPyramid->GetPoints()->SetPoint(12, 0.25, 0.75, 0.5);

  return aPyramid;
}

vtkSmartPointer<vtkTriQuadraticPyramid> MakeTriQuadraticPyramid()
{
  auto aPyramid = vtkSmartPointer<vtkTriQuadraticPyramid>::New();

  for (int i = 0; i < aPyramid->GetNumberOfPoints(); ++i)
  {
    aPyramid->GetPointIds()->SetId(i, i);
  }

  aPyramid->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  aPyramid->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  aPyramid->GetPoints()->SetPoint(2, 1.0, 1.0, 0.0);
  aPyramid->GetPoints()->SetPoint(3, 0.0, 1.0, 0.0);
  aPyramid->GetPoints()->SetPoint(4, 0.5, 0.5, 1.0);

  aPyramid->GetPoints()->SetPoint(5, 0.5, 0.0, 0.0);
  aPyramid->GetPoints()->SetPoint(6, 1.0, 0.5, 0.0);
  aPyramid->GetPoints()->SetPoint(7, 0.5, 1.0, 0.0);
  aPyramid->GetPoints()->SetPoint(8, 0.0, 0.5, 0.0);
  aPyramid->GetPoints()->SetPoint(9, 0.25, 0.25, 0.5);
  aPyramid->GetPoints()->SetPoint(10, 0.75, 0.25, 0.5);
  aPyramid->GetPoints()->SetPoint(11, 0.75, 0.75, 0.5);
  aPyramid->GetPoints()->SetPoint(12, 0.25, 0.75, 0.5);

  aPyramid->GetPoints()->SetPoint(13, 0.5, 0.5, 0);
  aPyramid->GetPoints()->SetPoint(14, 0.5, 1.0 / 6.0, 1.0 / 3.0);
  aPyramid->GetPoints()->SetPoint(15, 5.0 / 6.0, 0.5, 1.0 / 3.0);
  aPyramid->GetPoints()->SetPoint(16, 0.5, 5.0 / 6.0, 1.0 / 3.0);
  aPyramid->GetPoints()->SetPoint(17, 1.0 / 6.0, 0.5, 1.0 / 3.0);

  aPyramid->GetPoints()->SetPoint(18, 0.5, 0.5, 0.2);

  return aPyramid;
}

vtkSmartPointer<vtkQuadraticEdge> MakeQuadraticEdge()
{
  auto anEdge = vtkSmartPointer<vtkQuadraticEdge>::New();

  for (int i = 0; i < anEdge->GetNumberOfPoints(); ++i)
  {
    anEdge->GetPointIds()->SetId(i, i);
  }

  anEdge->GetPoints()->SetPoint(0, 0, 0, 0);
  anEdge->GetPoints()->SetPoint(1, 1, 0, 0);
  anEdge->GetPoints()->SetPoint(2, .5, 0, 0);

  return anEdge;
}

vtkSmartPointer<vtkQuadraticHexahedron> MakeQuadraticHexahedron()
{
  auto aHexahedron = vtkSmartPointer<vtkQuadraticHexahedron>::New();

  double* pcoords = aHexahedron->GetParametricCoords();
  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
    aHexahedron->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + vtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + vtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 2) + vtkMath::Random(-.1, .1));
  }

  return aHexahedron;
}

vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron> MakeBiQuadraticQuadraticHexahedron()
{
  auto aHexahedron = vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron>::New();

  double* pcoords = aHexahedron->GetParametricCoords();
  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
    aHexahedron->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + vtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + vtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 2) + vtkMath::Random(-.1, .1));
  }

  return aHexahedron;
}

vtkSmartPointer<vtkTriQuadraticHexahedron> MakeTriQuadraticHexahedron()
{
  auto aHexahedron = vtkSmartPointer<vtkTriQuadraticHexahedron>::New();

  double* pcoords = aHexahedron->GetParametricCoords();
  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
    aHexahedron->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + vtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + vtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 2) + vtkMath::Random(-.1, .1));
  }

  return aHexahedron;
}

vtkSmartPointer<vtkQuadraticPolygon> MakeQuadraticPolygon()
{
  auto aPolygon = vtkSmartPointer<vtkQuadraticPolygon>::New();

  aPolygon->GetPointIds()->SetNumberOfIds(8);
  aPolygon->GetPointIds()->SetId(0, 0);
  aPolygon->GetPointIds()->SetId(1, 1);
  aPolygon->GetPointIds()->SetId(2, 2);
  aPolygon->GetPointIds()->SetId(3, 3);
  aPolygon->GetPointIds()->SetId(4, 4);
  aPolygon->GetPointIds()->SetId(5, 5);
  aPolygon->GetPointIds()->SetId(6, 6);
  aPolygon->GetPointIds()->SetId(7, 7);

  aPolygon->GetPoints()->SetNumberOfPoints(8);
  aPolygon->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(1, 2.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(2, 2.0, 2.0, 0.0);
  aPolygon->GetPoints()->SetPoint(3, 0.0, 2.0, 0.0);
  aPolygon->GetPoints()->SetPoint(4, 1.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(5, 2.0, 1.0, 0.0);
  aPolygon->GetPoints()->SetPoint(6, 1.0, 2.0, 0.0);
  aPolygon->GetPoints()->SetPoint(7, 0.0, 1.0, 0.0);
  aPolygon->GetPoints()->SetPoint(5, 3.0, 1.0, 0.0);

  return aPolygon;
}

vtkSmartPointer<vtkQuadraticLinearQuad> MakeQuadraticLinearQuad()
{
  auto aLinearQuad = vtkSmartPointer<vtkQuadraticLinearQuad>::New();

  double* pcoords = aLinearQuad->GetParametricCoords();
  for (int i = 0; i < aLinearQuad->GetNumberOfPoints(); ++i)
  {
    aLinearQuad->GetPointIds()->SetId(i, i);
    aLinearQuad->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }

  return aLinearQuad;
}

vtkSmartPointer<vtkQuadraticLinearWedge> MakeQuadraticLinearWedge()
{
  auto aLinearWedge = vtkSmartPointer<vtkQuadraticLinearWedge>::New();

  double* pcoords = aLinearWedge->GetParametricCoords();
  for (int i = 0; i < 12; ++i)
  {
    aLinearWedge->GetPointIds()->SetId(i, i);
    aLinearWedge->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }

  return aLinearWedge;
}

vtkSmartPointer<vtkQuadraticQuad> MakeQuadraticQuad()
{
  auto aQuad = vtkSmartPointer<vtkQuadraticQuad>::New();

  double* pcoords = aQuad->GetParametricCoords();
  for (int i = 0; i < 8; ++i)
  {
    aQuad->GetPointIds()->SetId(i, i);
    aQuad->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + vtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + vtkMath::Random(-.1, .1), *(pcoords + 3 * i + 2));
  }

  return aQuad;
}

vtkSmartPointer<vtkQuadraticTetra> MakeQuadraticTetra()
{
  auto aTetra = vtkSmartPointer<vtkQuadraticTetra>::New();

  double* pcoords = aTetra->GetParametricCoords();
  for (int i = 0; i < 10; ++i)
  {
    aTetra->GetPointIds()->SetId(i, i);
    aTetra->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + vtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + vtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 2) + vtkMath::Random(-.1, .1));
  }

  return aTetra;
}

vtkSmartPointer<vtkQuadraticTriangle> MakeQuadraticTriangle()
{
  auto aTriangle = vtkSmartPointer<vtkQuadraticTriangle>::New();

  double* pcoords = aTriangle->GetParametricCoords();
  for (int i = 0; i < aTriangle->GetNumberOfPoints(); ++i)
  {
    aTriangle->GetPointIds()->SetId(i, i);
    aTriangle->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }

  return aTriangle;
}

vtkSmartPointer<vtkBiQuadraticTriangle> MakeBiQuadraticTriangle()
{
  auto aTriangle = vtkSmartPointer<vtkBiQuadraticTriangle>::New();

  double* pcoords = aTriangle->GetParametricCoords();
  for (int i = 0; i < aTriangle->GetNumberOfPoints(); ++i)
  {
    aTriangle->GetPointIds()->SetId(i, i);
    aTriangle->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }

  return aTriangle;
}

vtkSmartPointer<vtkBiQuadraticQuad> MakeBiQuadraticQuad()
{
  auto aQuad = vtkSmartPointer<vtkBiQuadraticQuad>::New();

  double* pcoords = aQuad->GetParametricCoords();
  for (int i = 0; i < aQuad->GetNumberOfPoints(); ++i)
  {
    aQuad->GetPointIds()->SetId(i, i);
    aQuad->GetPoints()->SetPoint(i, *(pcoords + 3 * i) + vtkMath::Random(-.1, .1),
      *(pcoords + 3 * i + 1) + vtkMath::Random(-.1, .1), *(pcoords + 3 * i + 2));
  }

  return aQuad;
}

vtkSmartPointer<vtkCubicLine> MakeCubicLine()
{
  auto aLine = vtkSmartPointer<vtkCubicLine>::New();

  double* pcoords = aLine->GetParametricCoords();
  for (int i = 0; i < aLine->GetNumberOfPoints(); ++i)
  {
    aLine->GetPointIds()->SetId(i, i);
    aLine->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }

  return aLine;
}

vtkSmartPointer<vtkQuadraticWedge> MakeQuadraticWedge()
{
  auto aWedge = vtkSmartPointer<vtkQuadraticWedge>::New();

  double* pcoords = aWedge->GetParametricCoords();
  for (int i = 0; i < aWedge->GetNumberOfPoints(); ++i)
  {
    aWedge->GetPointIds()->SetId(i, i);
    aWedge->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }

  return aWedge;
}

vtkSmartPointer<vtkBiQuadraticQuadraticWedge> MakeBiQuadraticQuadraticWedge()
{
  auto aWedge = vtkSmartPointer<vtkBiQuadraticQuadraticWedge>::New();

  double* pcoords = aWedge->GetParametricCoords();
  for (int i = 0; i < aWedge->GetNumberOfPoints(); ++i)
  {
    aWedge->GetPointIds()->SetId(i, i);
    aWedge->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }

  return aWedge;
}

vtkSmartPointer<vtkTetra> MakeTetra()
{
  auto aTetra = vtkSmartPointer<vtkTetra>::New();

  for (int i = 0; i < aTetra->GetNumberOfPoints(); ++i)
  {
    aTetra->GetPointIds()->SetId(i, i);
  }

  aTetra->GetPoints()->SetPoint(0, 10.0, 10.0, 10.0);
  aTetra->GetPoints()->SetPoint(1, 12.0, 10.0, 10.0);
  aTetra->GetPoints()->SetPoint(2, 11.0, 12.0, 10.0);
  aTetra->GetPoints()->SetPoint(3, 11.0, 11.0, 12.0);

  return aTetra;
}

vtkSmartPointer<vtkWedge> MakeWedge()
{
  auto aWedge = vtkSmartPointer<vtkWedge>::New();

  for (int i = 0; i < aWedge->GetNumberOfPoints(); ++i)
  {
    aWedge->GetPointIds()->SetId(i, i);
  }

  aWedge->GetPoints()->SetPoint(0, 0.0, 1.0, 0.0);
  aWedge->GetPoints()->SetPoint(1, 0.0, 0.0, 0.0);
  aWedge->GetPoints()->SetPoint(2, 0.0, 0.5, 0.5);
  aWedge->GetPoints()->SetPoint(3, 1.0, 1.0, 0.0);
  aWedge->GetPoints()->SetPoint(4, 1.0, 0.0, 0.0);
  aWedge->GetPoints()->SetPoint(5, 1.0, 0.5, 0.5);

  return aWedge;
}

vtkSmartPointer<vtkPolyhedron> MakeCube()
{
  auto aCube = vtkSmartPointer<vtkPolyhedron>::New();

  // create polyhedron (cube)
  auto points = vtkSmartPointer<vtkPoints>::New();

  aCube->GetPointIds()->SetNumberOfIds(8);
  aCube->GetPointIds()->SetId(0, 0);
  aCube->GetPointIds()->SetId(1, 1);
  aCube->GetPointIds()->SetId(2, 2);
  aCube->GetPointIds()->SetId(3, 3);
  aCube->GetPointIds()->SetId(4, 4);
  aCube->GetPointIds()->SetId(5, 5);
  aCube->GetPointIds()->SetId(6, 6);
  aCube->GetPointIds()->SetId(7, 7);

  aCube->GetPoints()->SetNumberOfPoints(8);
  aCube->GetPoints()->SetPoint(0, -1.0, -1.0, -1.0);
  aCube->GetPoints()->SetPoint(1, 1.0, -1.0, -1.0);
  aCube->GetPoints()->SetPoint(2, 1.0, 1.0, -1.0);
  aCube->GetPoints()->SetPoint(3, -1.0, 1.0, -1.0);
  aCube->GetPoints()->SetPoint(4, -1.0, -1.0, 1.0);
  aCube->GetPoints()->SetPoint(5, 1.0, -1.0, 1.0);
  aCube->GetPoints()->SetPoint(6, 1.0, 1.0, 1.0);
  aCube->GetPoints()->SetPoint(7, -1.0, 1.0, 1.0);

  vtkIdType face_offsets[7] = { 0, 4, 8, 12, 16, 20, 24 };
  vtkIdType face_conns[24] = {
    0, 3, 2, 1, //
    0, 4, 7, 3, //
    4, 5, 6, 7, //
    5, 1, 2, 6, //
    0, 1, 5, 4, //
    2, 3, 7, 6  //
  };
  vtkNew<vtkCellArray> faces;
  vtkNew<vtkIdTypeArray> offsets_arr;
  vtkNew<vtkIdTypeArray> conns_arr;
  offsets_arr->SetArray(face_offsets, 7, 1);
  conns_arr->SetArray(face_conns, 24, 1);
  faces->SetData(offsets_arr, conns_arr);
  aCube->SetCellFaces(faces);
  aCube->Initialize();

  return aCube;
}

vtkSmartPointer<vtkPolyhedron> MakeDodecahedron()
{
  auto aDodecahedron = vtkSmartPointer<vtkPolyhedron>::New();

  // create polyhedron (dodecahedron)
  auto points = vtkSmartPointer<vtkPoints>::New();

  for (int i = 0; i < 20; ++i)
  {
    aDodecahedron->GetPointIds()->InsertNextId(i);
  }

  aDodecahedron->GetPoints()->InsertNextPoint(1.21412, 0, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.375185, 1.1547, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.982247, 0.713644, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.982247, -0.713644, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.375185, -1.1547, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(1.96449, 0, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.607062, 1.86835, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.58931, 1.1547, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.58931, -1.1547, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.607062, -1.86835, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(1.58931, 1.1547, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.607062, 1.86835, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.96449, 0, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.607062, -1.86835, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(1.58931, -1.1547, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.982247, 0.713644, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.375185, 1.1547, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.21412, 0, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.375185, -1.1547, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.982247, -0.713644, -1.58931);

  vtkIdType face_offsets[13] = { 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60 };
  vtkIdType face_conns[60] = {
    0, 1, 2, 3, 4,     // ids
    0, 5, 10, 6, 1,    //
    1, 6, 11, 7, 2,    //
    2, 7, 12, 8, 3,    //
    3, 8, 13, 9, 4,    //
    4, 9, 14, 5, 0,    //
    15, 10, 5, 14, 19, //
    16, 11, 6, 10, 15, //
    17, 12, 7, 11, 16, //
    18, 13, 8, 12, 17, //
    19, 14, 9, 13, 18, //
    19, 18, 17, 16, 15 //
  };
  vtkNew<vtkCellArray> faces;
  vtkNew<vtkIdTypeArray> offsets_arr;
  vtkNew<vtkIdTypeArray> conns_arr;
  offsets_arr->SetArray(face_offsets, 13, 1);
  conns_arr->SetArray(face_conns, 60, 1);
  faces->SetData(offsets_arr, conns_arr);
  aDodecahedron->SetCellFaces(faces);
  aDodecahedron->Initialize();

  return aDodecahedron;
}

vtkSmartPointer<vtkPolyhedron> MakePolyhedralWedge(bool goodOrBad)
{
  auto aWedge = vtkSmartPointer<vtkPolyhedron>::New();

  // create polyhedron (wedge)
  auto points = vtkSmartPointer<vtkPoints>::New();

  for (int i = 0; i < 6; ++i)
  {
    aWedge->GetPointIds()->InsertNextId(i);
  }

  // clang-format off
  // A wedge with one point slightly (or greatly) out of plane:
  double yc = goodOrBad ? -0.05 : -0.3;
  aWedge->GetPoints()->InsertNextPoint(0.0, 0.0, 0.0);
  aWedge->GetPoints()->InsertNextPoint(1.0, 0.0, 0.0);
  aWedge->GetPoints()->InsertNextPoint(0.0, 1.0, 0.0);
  aWedge->GetPoints()->InsertNextPoint(0.0,  yc, 0.5);
  aWedge->GetPoints()->InsertNextPoint(1.0, 0.0, 0.5);
  aWedge->GetPoints()->InsertNextPoint(0.0, 1.0, 0.5);

  vtkIdType face_offsets[6] = { 0, 4, 8, 11, 14, 18 };
  vtkIdType face_conns[18] = {
     4, 1, 2, 5,
     3, 0, 1, 4,
     2, 1, 0,
     3, 4, 5,
     5, 2, 0, 3
  };
  // clang-format on

  vtkNew<vtkCellArray> faces;
  vtkNew<vtkIdTypeArray> offsets_arr;
  vtkNew<vtkIdTypeArray> conns_arr;
  offsets_arr->SetArray(face_offsets, 6, 1);
  conns_arr->SetArray(face_conns, 18, 1);
  faces->SetData(offsets_arr, conns_arr);
  aWedge->SetCellFaces(faces);
  aWedge->Initialize();

  return aWedge;
}

vtkSmartPointer<vtkPentagonalPrism> MakePentagonalPrism()
{
  auto aPentagonalPrism = vtkSmartPointer<vtkPentagonalPrism>::New();

  for (int i = 0; i < aPentagonalPrism->GetNumberOfPoints(); ++i)
  {
    aPentagonalPrism->GetPointIds()->SetId(i, i);
  }

  aPentagonalPrism->GetPoints()->SetPoint(0, 11, 10, 10);
  aPentagonalPrism->GetPoints()->SetPoint(1, 13, 10, 10);
  aPentagonalPrism->GetPoints()->SetPoint(2, 14, 12, 10);
  aPentagonalPrism->GetPoints()->SetPoint(3, 12, 14, 10);
  aPentagonalPrism->GetPoints()->SetPoint(4, 10, 12, 10);
  aPentagonalPrism->GetPoints()->SetPoint(5, 11, 10, 14);
  aPentagonalPrism->GetPoints()->SetPoint(6, 13, 10, 14);
  aPentagonalPrism->GetPoints()->SetPoint(7, 14, 12, 14);
  aPentagonalPrism->GetPoints()->SetPoint(8, 12, 14, 14);
  aPentagonalPrism->GetPoints()->SetPoint(9, 10, 12, 14);

  return aPentagonalPrism;
}

vtkSmartPointer<vtkHexagonalPrism> MakeHexagonalPrism()
{
  auto aHexagonalPrism = vtkSmartPointer<vtkHexagonalPrism>::New();

  for (int i = 0; i < aHexagonalPrism->GetNumberOfPoints(); ++i)
  {
    aHexagonalPrism->GetPointIds()->SetId(i, i);
  }

  aHexagonalPrism->GetPoints()->SetPoint(0, 11, 10, 10);
  aHexagonalPrism->GetPoints()->SetPoint(1, 13, 10, 10);
  aHexagonalPrism->GetPoints()->SetPoint(2, 14, 12, 10);
  aHexagonalPrism->GetPoints()->SetPoint(3, 13, 14, 10);
  aHexagonalPrism->GetPoints()->SetPoint(4, 11, 14, 10);
  aHexagonalPrism->GetPoints()->SetPoint(5, 10, 12, 10);
  aHexagonalPrism->GetPoints()->SetPoint(6, 11, 10, 14);
  aHexagonalPrism->GetPoints()->SetPoint(7, 13, 10, 14);
  aHexagonalPrism->GetPoints()->SetPoint(8, 14, 12, 14);
  aHexagonalPrism->GetPoints()->SetPoint(9, 13, 14, 14);
  aHexagonalPrism->GetPoints()->SetPoint(10, 11, 14, 14);
  aHexagonalPrism->GetPoints()->SetPoint(11, 10, 12, 14);

  return aHexagonalPrism;
}

vtkSmartPointer<vtkLagrangeCurve> MakeLagrangeCurve()
{
  int nPoints = 5;

  auto curve = vtkSmartPointer<vtkLagrangeCurve>::New();

  curve->GetPointIds()->SetNumberOfIds(nPoints);
  curve->GetPoints()->SetNumberOfPoints(nPoints);
  curve->Initialize();
  double* points = curve->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    curve->GetPointIds()->SetId(i, i);
    curve->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return curve;
}

vtkSmartPointer<vtkLagrangeTriangle> MakeLagrangeTriangle()
{
  int nPoints = 15;

  auto triangle = vtkSmartPointer<vtkLagrangeTriangle>::New();

  triangle->GetPointIds()->SetNumberOfIds(nPoints);
  triangle->GetPoints()->SetNumberOfPoints(nPoints);
  triangle->Initialize();
  double* points = triangle->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    triangle->GetPointIds()->SetId(i, i);
    triangle->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return triangle;
}

vtkSmartPointer<vtkLagrangeTriangle> MakeBrokenLagrangeTriangle()
{
  int nPoints = 6;

  auto triangle = vtkSmartPointer<vtkLagrangeTriangle>::New();

  triangle->GetPointIds()->SetNumberOfIds(nPoints);
  triangle->GetPoints()->SetNumberOfPoints(nPoints);
  triangle->Initialize();
  double* points = triangle->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    triangle->GetPointIds()->SetId(i, (i == 2 ? 1 : i == 1 ? 2 : i));
    triangle->GetPoints()->SetPoint(i, &points[3 * (i == 2 ? 1 : i == 1 ? 2 : i)]);
  }

  return triangle;
}

vtkSmartPointer<vtkLagrangeQuadrilateral> MakeLagrangeQuadrilateral()
{
  int nPoints = 25;

  auto quadrilateral = vtkSmartPointer<vtkLagrangeQuadrilateral>::New();

  quadrilateral->GetPointIds()->SetNumberOfIds(nPoints);
  quadrilateral->GetPoints()->SetNumberOfPoints(nPoints);
  quadrilateral->SetUniformOrderFromNumPoints(nPoints);
  quadrilateral->Initialize();
  double* points = quadrilateral->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    quadrilateral->GetPointIds()->SetId(i, i);
    quadrilateral->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return quadrilateral;
}

vtkSmartPointer<vtkLagrangeHexahedron> MakeLagrangeHexahedron()
{
  int nPoints = 125;

  auto hexahedron = vtkSmartPointer<vtkLagrangeHexahedron>::New();

  hexahedron->GetPointIds()->SetNumberOfIds(nPoints);
  hexahedron->GetPoints()->SetNumberOfPoints(nPoints);
  hexahedron->SetUniformOrderFromNumPoints(nPoints);
  hexahedron->Initialize();
  double* points = hexahedron->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    hexahedron->GetPointIds()->SetId(i, i);
    hexahedron->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return hexahedron;
}

vtkSmartPointer<vtkLagrangeTetra> MakeLagrangeTetra()
{
  int nPoints = 10;

  auto tetra = vtkSmartPointer<vtkLagrangeTetra>::New();

  tetra->GetPointIds()->SetNumberOfIds(nPoints);
  tetra->GetPoints()->SetNumberOfPoints(nPoints);
  tetra->Initialize();
  double* points = tetra->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    tetra->GetPointIds()->SetId(i, i);
    tetra->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return tetra;
}

vtkSmartPointer<vtkLagrangeWedge> MakeLagrangeWedge()
{
  int nPoints = 75;

  auto wedge = vtkSmartPointer<vtkLagrangeWedge>::New();

  wedge->GetPointIds()->SetNumberOfIds(nPoints);
  wedge->GetPoints()->SetNumberOfPoints(nPoints);
  wedge->SetUniformOrderFromNumPoints(nPoints);
  wedge->Initialize();
  double* points = wedge->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    wedge->GetPointIds()->SetId(i, i);
    wedge->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return wedge;
}

vtkSmartPointer<vtkBezierCurve> MakeBezierCurve()
{
  int nPoints = 5;

  auto curve = vtkSmartPointer<vtkBezierCurve>::New();

  curve->GetPointIds()->SetNumberOfIds(nPoints);
  curve->GetPoints()->SetNumberOfPoints(nPoints);
  curve->Initialize();
  double* points = curve->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    curve->GetPointIds()->SetId(i, i);
    curve->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return curve;
}

vtkSmartPointer<vtkBezierTriangle> MakeBezierTriangle()
{
  int nPoints = 15;

  auto triangle = vtkSmartPointer<vtkBezierTriangle>::New();

  triangle->GetPointIds()->SetNumberOfIds(nPoints);
  triangle->GetPoints()->SetNumberOfPoints(nPoints);
  triangle->Initialize();
  double* points = triangle->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    triangle->GetPointIds()->SetId(i, i);
    triangle->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return triangle;
}

vtkSmartPointer<vtkBezierQuadrilateral> MakeBezierQuadrilateral()
{
  int nPoints = 25;

  auto quadrilateral = vtkSmartPointer<vtkBezierQuadrilateral>::New();

  quadrilateral->GetPointIds()->SetNumberOfIds(nPoints);
  quadrilateral->GetPoints()->SetNumberOfPoints(nPoints);
  quadrilateral->SetUniformOrderFromNumPoints(nPoints);
  quadrilateral->Initialize();
  double* points = quadrilateral->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    quadrilateral->GetPointIds()->SetId(i, i);
    quadrilateral->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return quadrilateral;
}

vtkSmartPointer<vtkBezierHexahedron> MakeBezierHexahedron()
{
  int nPoints = 125;

  auto hexahedron = vtkSmartPointer<vtkBezierHexahedron>::New();

  hexahedron->GetPointIds()->SetNumberOfIds(nPoints);
  hexahedron->GetPoints()->SetNumberOfPoints(nPoints);
  hexahedron->SetUniformOrderFromNumPoints(nPoints);
  hexahedron->Initialize();
  double* points = hexahedron->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    hexahedron->GetPointIds()->SetId(i, i);
    hexahedron->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return hexahedron;
}

vtkSmartPointer<vtkBezierTetra> MakeBezierTetra()
{
  int nPoints = 10;

  auto tetra = vtkSmartPointer<vtkBezierTetra>::New();

  tetra->GetPointIds()->SetNumberOfIds(nPoints);
  tetra->GetPoints()->SetNumberOfPoints(nPoints);
  tetra->Initialize();
  double* points = tetra->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    tetra->GetPointIds()->SetId(i, i);
    tetra->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return tetra;
}

vtkSmartPointer<vtkBezierWedge> MakeBezierWedge()
{
  int nPoints = 75;

  auto wedge = vtkSmartPointer<vtkBezierWedge>::New();

  wedge->GetPointIds()->SetNumberOfIds(nPoints);
  wedge->GetPoints()->SetNumberOfPoints(nPoints);
  wedge->SetUniformOrderFromNumPoints(nPoints);
  wedge->Initialize();
  double* points = wedge->GetParametricCoords();
  for (vtkIdType i = 0; i < nPoints; i++)
  {
    wedge->GetPointIds()->SetId(i, i);
    wedge->GetPoints()->SetPoint(i, &points[3 * i]);
  }

  return wedge;
}
