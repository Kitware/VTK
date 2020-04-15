/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellValidator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests vtkCellValidator

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
#include "vtkMathUtilities.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

#include <vtkActor.h>
#include <vtkDataSetMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <map>
#include <sstream>
#include <string>
#include <vector>

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
//----------------------------------------------------------------------------

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
  state = vtkCellValidator::Check(cellPtr, FLT_EPSILON);                                           \
  if (state != vtkCellValidator::State::Valid)                                                     \
  {                                                                                                \
    cellPtr->Print(std::cout);                                                                     \
    vtkCellValidator::PrintState(state, std::cout, vtkIndent(0));                                  \
    return EXIT_FAILURE;                                                                           \
  }

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
  CheckCell(cubicLine);
  CheckCell(triQuadraticHexahedron);
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

  return EXIT_SUCCESS;
}

vtkSmartPointer<vtkEmptyCell> MakeEmptyCell()
{
  vtkSmartPointer<vtkEmptyCell> anEmptyCell = vtkSmartPointer<vtkEmptyCell>::New();
  return anEmptyCell;
}

vtkSmartPointer<vtkVertex> MakeVertex()
{
  vtkSmartPointer<vtkVertex> aVertex = vtkSmartPointer<vtkVertex>::New();
  aVertex->GetPointIds()->SetId(0, 0);
  aVertex->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);

  return aVertex;
  ;
}

vtkSmartPointer<vtkPolyVertex> MakePolyVertex()
{
  vtkSmartPointer<vtkPolyVertex> aPolyVertex = vtkSmartPointer<vtkPolyVertex>::New();
  aPolyVertex->GetPointIds()->SetNumberOfIds(2);
  aPolyVertex->GetPointIds()->SetId(0, 0);
  aPolyVertex->GetPointIds()->SetId(1, 1);

  aPolyVertex->GetPoints()->SetNumberOfPoints(2);
  aPolyVertex->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);
  aPolyVertex->GetPoints()->SetPoint(1, 30.0, 20.0, 10.0);

  return aPolyVertex;
  ;
}

vtkSmartPointer<vtkLine> MakeLine()
{
  vtkSmartPointer<vtkLine> aLine = vtkSmartPointer<vtkLine>::New();
  aLine->GetPointIds()->SetId(0, 0);
  aLine->GetPointIds()->SetId(1, 1);
  aLine->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);
  aLine->GetPoints()->SetPoint(1, 30.0, 20.0, 10.0);
  return aLine;
  ;
}

vtkSmartPointer<vtkPolyLine> MakePolyLine()
{
  vtkSmartPointer<vtkPolyLine> aPolyLine = vtkSmartPointer<vtkPolyLine>::New();
  aPolyLine->GetPointIds()->SetNumberOfIds(3);
  aPolyLine->GetPointIds()->SetId(0, 0);
  aPolyLine->GetPointIds()->SetId(1, 1);
  aPolyLine->GetPointIds()->SetId(2, 2);

  aPolyLine->GetPoints()->SetNumberOfPoints(3);
  aPolyLine->GetPoints()->SetPoint(0, 10.0, 20.0, 30.0);
  aPolyLine->GetPoints()->SetPoint(1, 10.0, 30.0, 30.0);
  aPolyLine->GetPoints()->SetPoint(2, 10.0, 30.0, 40.0);

  return aPolyLine;
  ;
}

vtkSmartPointer<vtkTriangle> MakeTriangle()
{
  vtkSmartPointer<vtkTriangle> aTriangle = vtkSmartPointer<vtkTriangle>::New();
  aTriangle->GetPoints()->SetPoint(0, -10.0, -10.0, 0.0);
  aTriangle->GetPoints()->SetPoint(1, 10.0, -10.0, 0.0);
  aTriangle->GetPoints()->SetPoint(2, 10.0, 10.0, 0.0);
  aTriangle->GetPointIds()->SetId(0, 0);
  aTriangle->GetPointIds()->SetId(1, 1);
  aTriangle->GetPointIds()->SetId(2, 2);
  return aTriangle;
}

vtkSmartPointer<vtkTriangleStrip> MakeTriangleStrip()
{
  vtkSmartPointer<vtkTriangleStrip> aTriangleStrip = vtkSmartPointer<vtkTriangleStrip>::New();
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
  vtkSmartPointer<vtkPolygon> aPolygon = vtkSmartPointer<vtkPolygon>::New();
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
  vtkSmartPointer<vtkQuad> aQuad = vtkSmartPointer<vtkQuad>::New();
  aQuad->GetPoints()->SetPoint(0, -10.0, -10.0, 0.0);
  aQuad->GetPoints()->SetPoint(1, 10.0, -10.0, 0.0);
  aQuad->GetPoints()->SetPoint(2, 10.0, 10.0, 0.0);
  aQuad->GetPoints()->SetPoint(3, -10.0, 10.0, 0.0);
  aQuad->GetPointIds()->SetId(0, 0);
  aQuad->GetPointIds()->SetId(1, 1);
  aQuad->GetPointIds()->SetId(2, 2);
  aQuad->GetPointIds()->SetId(2, 3);
  return aQuad;
}

vtkSmartPointer<vtkPixel> MakePixel()
{
  vtkSmartPointer<vtkPixel> aPixel = vtkSmartPointer<vtkPixel>::New();
  aPixel->GetPointIds()->SetId(0, 0);
  aPixel->GetPointIds()->SetId(1, 1);
  aPixel->GetPointIds()->SetId(2, 2);
  aPixel->GetPointIds()->SetId(3, 3);

  aPixel->GetPoints()->SetPoint(0, 10.0, 10.0, 10.0);
  aPixel->GetPoints()->SetPoint(1, 12.0, 10.0, 10.0);
  aPixel->GetPoints()->SetPoint(2, 10.0, 12.0, 10.0);
  aPixel->GetPoints()->SetPoint(3, 12.0, 12.0, 10.0);
  return aPixel;
}

vtkSmartPointer<vtkVoxel> MakeVoxel()
{
  vtkSmartPointer<vtkVoxel> aVoxel = vtkSmartPointer<vtkVoxel>::New();
  aVoxel->GetPointIds()->SetId(0, 0);
  aVoxel->GetPointIds()->SetId(1, 1);
  aVoxel->GetPointIds()->SetId(2, 2);
  aVoxel->GetPointIds()->SetId(3, 3);
  aVoxel->GetPointIds()->SetId(4, 4);
  aVoxel->GetPointIds()->SetId(5, 5);
  aVoxel->GetPointIds()->SetId(6, 6);
  aVoxel->GetPointIds()->SetId(7, 7);

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
  vtkSmartPointer<vtkHexahedron> aHexahedron = vtkSmartPointer<vtkHexahedron>::New();
  aHexahedron->GetPointIds()->SetId(0, 0);
  aHexahedron->GetPointIds()->SetId(1, 1);
  aHexahedron->GetPointIds()->SetId(2, 2);
  aHexahedron->GetPointIds()->SetId(3, 3);
  aHexahedron->GetPointIds()->SetId(4, 4);
  aHexahedron->GetPointIds()->SetId(5, 5);
  aHexahedron->GetPointIds()->SetId(6, 6);
  aHexahedron->GetPointIds()->SetId(7, 7);

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
  vtkSmartPointer<vtkHexahedron> aHexahedron = vtkSmartPointer<vtkHexahedron>::New();
  aHexahedron->GetPointIds()->SetId(0, 0);
  aHexahedron->GetPointIds()->SetId(1, 1);
  aHexahedron->GetPointIds()->SetId(2, 2);
  aHexahedron->GetPointIds()->SetId(3, 3);
  aHexahedron->GetPointIds()->SetId(4, 4);
  aHexahedron->GetPointIds()->SetId(5, 5);
  aHexahedron->GetPointIds()->SetId(6, 6);
  aHexahedron->GetPointIds()->SetId(7, 7);

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
  vtkSmartPointer<vtkHexahedron> aHexahedron = vtkSmartPointer<vtkHexahedron>::New();
  aHexahedron->GetPointIds()->SetId(0, 0);
  aHexahedron->GetPointIds()->SetId(1, 1);
  aHexahedron->GetPointIds()->SetId(2, 3);
  aHexahedron->GetPointIds()->SetId(3, 2);
  aHexahedron->GetPointIds()->SetId(4, 4);
  aHexahedron->GetPointIds()->SetId(5, 5);
  aHexahedron->GetPointIds()->SetId(6, 6);
  aHexahedron->GetPointIds()->SetId(7, 7);

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
  vtkSmartPointer<vtkPyramid> aPyramid = vtkSmartPointer<vtkPyramid>::New();
  aPyramid->GetPointIds()->SetId(0, 0);
  aPyramid->GetPointIds()->SetId(1, 1);
  aPyramid->GetPointIds()->SetId(2, 2);
  aPyramid->GetPointIds()->SetId(3, 3);
  aPyramid->GetPointIds()->SetId(4, 4);

  aPyramid->GetPoints()->SetPoint(0, 0, 0, 0);
  aPyramid->GetPoints()->SetPoint(1, 1, 0, 0);
  aPyramid->GetPoints()->SetPoint(2, 1, 1, 0);
  aPyramid->GetPoints()->SetPoint(3, 0, 1, 0);
  aPyramid->GetPoints()->SetPoint(4, .5, .5, 1);

  return aPyramid;
}

vtkSmartPointer<vtkQuadraticPyramid> MakeQuadraticPyramid()
{
  vtkSmartPointer<vtkQuadraticPyramid> aPyramid = vtkSmartPointer<vtkQuadraticPyramid>::New();
  for (int i = 0; i < 13; ++i)
  {
    aPyramid->GetPointIds()->SetId(i, i);
  }

  aPyramid->GetPoints()->SetPoint(0, 0, 0, 0);
  aPyramid->GetPoints()->SetPoint(1, 1, 0, 0);
  aPyramid->GetPoints()->SetPoint(2, 1, 1, 0);
  aPyramid->GetPoints()->SetPoint(3, 0, 1, 0);
  aPyramid->GetPoints()->SetPoint(4, .5, .5, 1);

  aPyramid->GetPoints()->SetPoint(5, 0.5, 0.0, 0.0);
  aPyramid->GetPoints()->SetPoint(6, 1.0, 0.5, 0.0);
  aPyramid->GetPoints()->SetPoint(7, 0.5, 1.0, 0.0);
  aPyramid->GetPoints()->SetPoint(8, 0.0, 0.5, 0.0);

  aPyramid->GetPoints()->SetPoint(9, 0.5, 0.5, 0.5);
  aPyramid->GetPoints()->SetPoint(10, 0.75, 0.5, 0.5);
  aPyramid->GetPoints()->SetPoint(11, 0.75, 0.75, 0.5);
  aPyramid->GetPoints()->SetPoint(12, 0.5, 0.75, 0.5);

  return aPyramid;
}

vtkSmartPointer<vtkQuadraticEdge> MakeQuadraticEdge()
{
  vtkSmartPointer<vtkQuadraticEdge> anEdge = vtkSmartPointer<vtkQuadraticEdge>::New();
  for (int i = 0; i < 3; ++i)
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
  vtkSmartPointer<vtkQuadraticHexahedron> aHexahedron =
    vtkSmartPointer<vtkQuadraticHexahedron>::New();
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
  vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron> aHexahedron =
    vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron>::New();
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
  vtkSmartPointer<vtkTriQuadraticHexahedron> aHexahedron =
    vtkSmartPointer<vtkTriQuadraticHexahedron>::New();
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
  vtkSmartPointer<vtkQuadraticPolygon> aPolygon = vtkSmartPointer<vtkQuadraticPolygon>::New();

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
  vtkSmartPointer<vtkQuadraticLinearQuad> aLinearQuad =
    vtkSmartPointer<vtkQuadraticLinearQuad>::New();
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
  vtkSmartPointer<vtkQuadraticLinearWedge> aLinearWedge =
    vtkSmartPointer<vtkQuadraticLinearWedge>::New();
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
  vtkSmartPointer<vtkQuadraticQuad> aQuad = vtkSmartPointer<vtkQuadraticQuad>::New();
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
  vtkSmartPointer<vtkQuadraticTetra> aTetra = vtkSmartPointer<vtkQuadraticTetra>::New();
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
  vtkSmartPointer<vtkQuadraticTriangle> aTriangle = vtkSmartPointer<vtkQuadraticTriangle>::New();
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
  vtkSmartPointer<vtkBiQuadraticTriangle> aTriangle =
    vtkSmartPointer<vtkBiQuadraticTriangle>::New();
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
  vtkSmartPointer<vtkBiQuadraticQuad> aQuad = vtkSmartPointer<vtkBiQuadraticQuad>::New();
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
  vtkSmartPointer<vtkCubicLine> aLine = vtkSmartPointer<vtkCubicLine>::New();
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
  vtkSmartPointer<vtkQuadraticWedge> aWedge = vtkSmartPointer<vtkQuadraticWedge>::New();
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
  vtkSmartPointer<vtkBiQuadraticQuadraticWedge> aWedge =
    vtkSmartPointer<vtkBiQuadraticQuadraticWedge>::New();
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
  vtkSmartPointer<vtkTetra> aTetra = vtkSmartPointer<vtkTetra>::New();
  aTetra->GetPointIds()->SetId(0, 0);
  aTetra->GetPointIds()->SetId(1, 1);
  aTetra->GetPointIds()->SetId(2, 2);
  aTetra->GetPointIds()->SetId(3, 3);
  aTetra->GetPoints()->SetPoint(0, 10.0, 10.0, 10.0);
  aTetra->GetPoints()->SetPoint(1, 12.0, 10.0, 10.0);
  aTetra->GetPoints()->SetPoint(2, 11.0, 12.0, 10.0);
  aTetra->GetPoints()->SetPoint(3, 11.0, 11.0, 12.0);
  return aTetra;
}

vtkSmartPointer<vtkWedge> MakeWedge()
{
  vtkSmartPointer<vtkWedge> aWedge = vtkSmartPointer<vtkWedge>::New();
  aWedge->GetPointIds()->SetId(0, 0);
  aWedge->GetPointIds()->SetId(1, 1);
  aWedge->GetPointIds()->SetId(2, 2);
  aWedge->GetPointIds()->SetId(3, 3);
  aWedge->GetPointIds()->SetId(4, 4);
  aWedge->GetPointIds()->SetId(5, 5);

  aWedge->GetPoints()->SetPoint(0, 0, 1, 0);
  aWedge->GetPoints()->SetPoint(1, 0, 0, 0);
  aWedge->GetPoints()->SetPoint(2, 0, .5, .5);
  aWedge->GetPoints()->SetPoint(3, 1, 1, 0);
  aWedge->GetPoints()->SetPoint(4, 1, 0.0, 0.0);
  aWedge->GetPoints()->SetPoint(5, 1, .5, .5);

  return aWedge;
}

vtkSmartPointer<vtkPolyhedron> MakeCube()
{
  vtkSmartPointer<vtkPolyhedron> aCube = vtkSmartPointer<vtkPolyhedron>::New();

  // create polyhedron (cube)
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

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

  vtkIdType faces[31] = {
    6,             // number of faces
    4, 0, 3, 2, 1, //
    4, 0, 4, 7, 3, //
    4, 4, 5, 6, 7, //
    4, 5, 1, 2, 6, //
    4, 0, 1, 5, 4, //
    4, 2, 3, 7, 6  //
  };

  aCube->SetFaces(faces);
  aCube->Initialize();
  return aCube;
}

vtkSmartPointer<vtkPolyhedron> MakeDodecahedron()
{
  vtkSmartPointer<vtkPolyhedron> aDodecahedron = vtkSmartPointer<vtkPolyhedron>::New();

  // create polyhedron (dodecahedron)
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

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

  vtkIdType faces[73] = {
    12,                   // number of faces
    5, 0, 1, 2, 3, 4,     // number of ids on face, ids
    5, 0, 5, 10, 6, 1,    //
    5, 1, 6, 11, 7, 2,    //
    5, 2, 7, 12, 8, 3,    //
    5, 3, 8, 13, 9, 4,    //
    5, 4, 9, 14, 5, 0,    //
    5, 15, 10, 5, 14, 19, //
    5, 16, 11, 6, 10, 15, //
    5, 17, 12, 7, 11, 16, //
    5, 18, 13, 8, 12, 17, //
    5, 19, 14, 9, 13, 18, //
    5, 19, 18, 17, 16, 15 //
  };

  aDodecahedron->SetFaces(faces);
  aDodecahedron->Initialize();

  return aDodecahedron;
}

vtkSmartPointer<vtkPentagonalPrism> MakePentagonalPrism()
{
  vtkSmartPointer<vtkPentagonalPrism> aPentagonalPrism = vtkSmartPointer<vtkPentagonalPrism>::New();

  aPentagonalPrism->GetPointIds()->SetId(0, 0);
  aPentagonalPrism->GetPointIds()->SetId(1, 1);
  aPentagonalPrism->GetPointIds()->SetId(2, 2);
  aPentagonalPrism->GetPointIds()->SetId(3, 3);
  aPentagonalPrism->GetPointIds()->SetId(4, 4);
  aPentagonalPrism->GetPointIds()->SetId(5, 5);
  aPentagonalPrism->GetPointIds()->SetId(6, 6);
  aPentagonalPrism->GetPointIds()->SetId(7, 7);
  aPentagonalPrism->GetPointIds()->SetId(8, 8);
  aPentagonalPrism->GetPointIds()->SetId(9, 9);

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
  vtkSmartPointer<vtkHexagonalPrism> aHexagonalPrism = vtkSmartPointer<vtkHexagonalPrism>::New();
  aHexagonalPrism->GetPointIds()->SetId(0, 0);
  aHexagonalPrism->GetPointIds()->SetId(1, 1);
  aHexagonalPrism->GetPointIds()->SetId(2, 2);
  aHexagonalPrism->GetPointIds()->SetId(3, 3);
  aHexagonalPrism->GetPointIds()->SetId(4, 4);
  aHexagonalPrism->GetPointIds()->SetId(5, 5);
  aHexagonalPrism->GetPointIds()->SetId(6, 6);
  aHexagonalPrism->GetPointIds()->SetId(7, 7);
  aHexagonalPrism->GetPointIds()->SetId(8, 8);
  aHexagonalPrism->GetPointIds()->SetId(9, 9);
  aHexagonalPrism->GetPointIds()->SetId(10, 10);
  aHexagonalPrism->GetPointIds()->SetId(11, 11);

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

  vtkSmartPointer<vtkLagrangeCurve> curve = vtkSmartPointer<vtkLagrangeCurve>::New();

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

  vtkSmartPointer<vtkLagrangeTriangle> triangle = vtkSmartPointer<vtkLagrangeTriangle>::New();

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

  vtkSmartPointer<vtkLagrangeTriangle> triangle = vtkSmartPointer<vtkLagrangeTriangle>::New();

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

  vtkSmartPointer<vtkLagrangeQuadrilateral> quadrilateral =
    vtkSmartPointer<vtkLagrangeQuadrilateral>::New();

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

  vtkSmartPointer<vtkLagrangeHexahedron> hexahedron = vtkSmartPointer<vtkLagrangeHexahedron>::New();

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

  vtkSmartPointer<vtkLagrangeTetra> tetra = vtkSmartPointer<vtkLagrangeTetra>::New();

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

  vtkSmartPointer<vtkLagrangeWedge> wedge = vtkSmartPointer<vtkLagrangeWedge>::New();

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

  vtkSmartPointer<vtkBezierCurve> curve = vtkSmartPointer<vtkBezierCurve>::New();

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

  vtkSmartPointer<vtkBezierTriangle> triangle = vtkSmartPointer<vtkBezierTriangle>::New();

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

  vtkSmartPointer<vtkBezierQuadrilateral> quadrilateral =
    vtkSmartPointer<vtkBezierQuadrilateral>::New();

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

  vtkSmartPointer<vtkBezierHexahedron> hexahedron = vtkSmartPointer<vtkBezierHexahedron>::New();

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

  vtkSmartPointer<vtkBezierTetra> tetra = vtkSmartPointer<vtkBezierTetra>::New();

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

  vtkSmartPointer<vtkBezierWedge> wedge = vtkSmartPointer<vtkBezierWedge>::New();

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
