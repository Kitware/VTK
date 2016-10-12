/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestCells.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkCellType.h"

#include "vtkEmptyCell.h"
#include "vtkVertex.h"
#include "vtkPolyVertex.h"
#include "vtkLine.h"
#include "vtkPolyLine.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkPolygon.h"
#include "vtkQuad.h"
#include "vtkPixel.h"
#include "vtkVoxel.h"
#include "vtkHexahedron.h"
#include "vtkPyramid.h"
#include "vtkTetra.h"
#include "vtkPolyhedron.h"
#include "vtkPentagonalPrism.h"
#include "vtkHexagonalPrism.h"
#include "vtkWedge.h"
#include "vtkPolyhedron.h"

#include "vtkQuadraticEdge.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkQuadraticHexahedron.h"
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

#include "vtkMathUtilities.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include <sstream>
#include <vector>
#include <string>
#include <map>

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

template<typename T> int TestOneCell(const VTKCellType cellType, vtkSmartPointer<T> cell, int linear = 1);
//----------------------------------------------------------------------------
int UnitTestCells(int, char*[])
{
  std::map<std::string,int> results;

  results["EmptyCell"] = TestOneCell<vtkEmptyCell>(VTK_EMPTY_CELL, MakeEmptyCell());
  results["Vertex"] = TestOneCell<vtkVertex>(VTK_VERTEX, MakeVertex());
  results["PolyVertex"] = TestOneCell<vtkPolyVertex>(VTK_POLY_VERTEX, MakePolyVertex());
  results["Line"] = TestOneCell<vtkLine>(VTK_LINE, MakeLine());
  results["PolyLine"] = TestOneCell<vtkPolyLine>(VTK_POLY_LINE, MakePolyLine());
  results["Triangle"] = TestOneCell<vtkTriangle>(VTK_TRIANGLE, MakeTriangle());
  results["TriangleStrip"] = TestOneCell<vtkTriangleStrip>(VTK_TRIANGLE_STRIP, MakeTriangleStrip());
  results["Polygon"] = TestOneCell<vtkPolygon>(VTK_POLYGON, MakePolygon());
  results["Pixel"] = TestOneCell<vtkPixel>(VTK_PIXEL, MakePixel());
  results["Quad"] = TestOneCell<vtkQuad>(VTK_QUAD, MakeQuad());
  results["Tetra"] = TestOneCell<vtkTetra>(VTK_TETRA, MakeTetra());
  results["Voxel"] = TestOneCell<vtkVoxel>(VTK_VOXEL, MakeVoxel());
  results["Hexahedron"] = TestOneCell<vtkHexahedron>(VTK_HEXAHEDRON, MakeHexahedron());
  results["Wedge"] = TestOneCell<vtkWedge>(VTK_WEDGE, MakeWedge());
  results["Pyramid"] = TestOneCell<vtkPyramid>(VTK_PYRAMID, MakePyramid());
  results["PentagonalPrism"] = TestOneCell<vtkPentagonalPrism>(VTK_PENTAGONAL_PRISM, MakePentagonalPrism());
  results["HexagonalPrism"] = TestOneCell<vtkHexagonalPrism>(VTK_HEXAGONAL_PRISM, MakeHexagonalPrism());
  results["Polyhedron(Cube)"] = TestOneCell<vtkPolyhedron>(VTK_POLYHEDRON, MakeCube());
  results["Polyhedron(Dodecahedron)"] = TestOneCell<vtkPolyhedron>(VTK_POLYHEDRON, MakeDodecahedron());

  results["QuadraticEdge"] = TestOneCell<vtkQuadraticEdge>(VTK_QUADRATIC_EDGE, MakeQuadraticEdge(), 0);
  results["QuadraticHexahedron"] = TestOneCell<vtkQuadraticHexahedron>(VTK_QUADRATIC_HEXAHEDRON, MakeQuadraticHexahedron(), 0);
  results["QuadraticPolygon"] = TestOneCell<vtkQuadraticPolygon>(VTK_QUADRATIC_POLYGON, MakeQuadraticPolygon(), 0);
  results["QuadraticLinearQuad"] = TestOneCell<vtkQuadraticLinearQuad>(VTK_QUADRATIC_LINEAR_QUAD, MakeQuadraticLinearQuad(), 0);
  results["QuadraticLinearWedge"] = TestOneCell<vtkQuadraticLinearWedge>(VTK_QUADRATIC_LINEAR_WEDGE, MakeQuadraticLinearWedge(), 0);
  results["QuadraticPyramid"] = TestOneCell<vtkQuadraticPyramid>(VTK_QUADRATIC_PYRAMID, MakeQuadraticPyramid(), 0);
  results["QuadraticQuad"] = TestOneCell<vtkQuadraticQuad>(VTK_QUADRATIC_QUAD, MakeQuadraticQuad(), 0);
  results["QuadraticTetra"] = TestOneCell<vtkQuadraticTetra>(VTK_QUADRATIC_TETRA, MakeQuadraticTetra(), 0);
  results["QuadraticTrangle"] = TestOneCell<vtkQuadraticTriangle>(VTK_QUADRATIC_TRIANGLE, MakeQuadraticTriangle(), 0);
  results["QuadraticWedge"] = TestOneCell<vtkQuadraticWedge>(VTK_QUADRATIC_WEDGE, MakeQuadraticWedge(), 0);

  results["BiQuadraticQuad"] = TestOneCell<vtkBiQuadraticQuad>(VTK_BIQUADRATIC_QUAD, MakeBiQuadraticQuad(), 0);
  results["BiQuadraticQuadraticHexahedron"] = TestOneCell<vtkBiQuadraticQuadraticHexahedron>(VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, MakeBiQuadraticQuadraticHexahedron(), 0);
  results["BiQuadraticQuadraticWedge"] = TestOneCell<vtkBiQuadraticQuadraticWedge>(VTK_BIQUADRATIC_QUADRATIC_WEDGE, MakeBiQuadraticQuadraticWedge(), 0);
  results["BiQuadraticTrangle"] = TestOneCell<vtkBiQuadraticTriangle>(VTK_BIQUADRATIC_TRIANGLE, MakeBiQuadraticTriangle(), 0);
  results["CubicLine"] = TestOneCell<vtkCubicLine>(VTK_CUBIC_LINE, MakeCubicLine(), 0);

  results["TriQuadraticHexahedron"] = TestOneCell<vtkTriQuadraticHexahedron>(VTK_TRIQUADRATIC_HEXAHEDRON, MakeTriQuadraticHexahedron(), 0);

  int status = 0;
  std::cout << "----- Unit Test Summary -----" << std::endl;
  std::map <std::string, int>::iterator it;
  for (it = results.begin(); it != results.end(); ++it)
  {
    std:: cout << std::setw(25) << it->first << " "  << (it->second ? " FAILED" : " OK") << std::endl;
    if (it->second != 0)
    {
      ++status;
    }
  }
  if (status)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

vtkSmartPointer<vtkEmptyCell> MakeEmptyCell()
{
  vtkSmartPointer<vtkEmptyCell> anEmptyCell =
    vtkSmartPointer<vtkEmptyCell>::New();
  return anEmptyCell;
}

vtkSmartPointer<vtkVertex> MakeVertex()
{
  vtkSmartPointer<vtkVertex> aVertex =
    vtkSmartPointer<vtkVertex>::New();
  aVertex->GetPointIds()->SetId(0,0);
  aVertex->GetPoints()->SetPoint (0, 10.0, 20.0, 30.0);

  return aVertex;;
}

vtkSmartPointer<vtkPolyVertex> MakePolyVertex()
{
  vtkSmartPointer<vtkPolyVertex> aPolyVertex =
    vtkSmartPointer<vtkPolyVertex>::New();
  aPolyVertex->GetPointIds()->SetNumberOfIds(2);
  aPolyVertex->GetPointIds()->SetId(0,0);
  aPolyVertex->GetPointIds()->SetId(1,1);

  aPolyVertex->GetPoints()->SetNumberOfPoints(2);
  aPolyVertex->GetPoints()->SetPoint (0, 10.0, 20.0, 30.0);
  aPolyVertex->GetPoints()->SetPoint (1, 30.0, 20.0, 10.0);

  return aPolyVertex;;
}

vtkSmartPointer<vtkLine> MakeLine()
{
  vtkSmartPointer<vtkLine> aLine =
    vtkSmartPointer<vtkLine>::New();
  aLine->GetPointIds()->SetId(0,0);
  aLine->GetPointIds()->SetId(1,1);
  aLine->GetPoints()->SetPoint (0, 10.0, 20.0, 30.0);
  aLine->GetPoints()->SetPoint (1, 30.0, 20.0, 10.0);
  return aLine;;
}

vtkSmartPointer<vtkPolyLine> MakePolyLine()
{
  vtkSmartPointer<vtkPolyLine> aPolyLine =
    vtkSmartPointer<vtkPolyLine>::New();
  aPolyLine->GetPointIds()->SetNumberOfIds(3);
  aPolyLine->GetPointIds()->SetId(0,0);
  aPolyLine->GetPointIds()->SetId(1,1);
  aPolyLine->GetPointIds()->SetId(2,2);

  aPolyLine->GetPoints()->SetNumberOfPoints(3);
  aPolyLine->GetPoints()->SetPoint (0, 10.0, 20.0, 30.0);
  aPolyLine->GetPoints()->SetPoint (1, 10.0, 30.0, 30.0);
  aPolyLine->GetPoints()->SetPoint (2, 10.0, 30.0, 40.0);

  return aPolyLine;;
}

vtkSmartPointer<vtkTriangle> MakeTriangle()
{
  vtkSmartPointer<vtkTriangle> aTriangle =
    vtkSmartPointer<vtkTriangle>::New();
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
  vtkSmartPointer<vtkTriangleStrip> aTriangleStrip =
    vtkSmartPointer<vtkTriangleStrip>::New();
  aTriangleStrip->GetPointIds()->SetNumberOfIds(4);
  aTriangleStrip->GetPointIds()->SetId(0,0);
  aTriangleStrip->GetPointIds()->SetId(1,1);
  aTriangleStrip->GetPointIds()->SetId(2,2);
  aTriangleStrip->GetPointIds()->SetId(3,3);

  aTriangleStrip->GetPoints()->SetNumberOfPoints(4);
  aTriangleStrip->GetPoints()->SetPoint (0, 10.0, 10.0, 10.0);
  aTriangleStrip->GetPoints()->SetPoint (1, 12.0, 10.0, 10.0);
  aTriangleStrip->GetPoints()->SetPoint (2, 11.0, 12.0, 10.0);
  aTriangleStrip->GetPoints()->SetPoint (3, 13.0, 10.0, 10.0);

  return aTriangleStrip;
}

vtkSmartPointer<vtkPolygon> MakePolygon()
{
  vtkSmartPointer<vtkPolygon> aPolygon =
    vtkSmartPointer<vtkPolygon>::New();
  aPolygon->GetPointIds()->SetNumberOfIds(4);
  aPolygon->GetPointIds()->SetId(0,0);
  aPolygon->GetPointIds()->SetId(1,1);
  aPolygon->GetPointIds()->SetId(2,2);
  aPolygon->GetPointIds()->SetId(3,3);

  aPolygon->GetPoints()->SetNumberOfPoints(4);
  aPolygon->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(1, 10.0, 0.0, 0.0);
  aPolygon->GetPoints()->SetPoint(2, 10.0, 10.0, 0.0);
  aPolygon->GetPoints()->SetPoint(3, 0.0, 10.0, 0.0);

  return aPolygon;
}

vtkSmartPointer<vtkQuad> MakeQuad()
{
  vtkSmartPointer<vtkQuad> aQuad =
    vtkSmartPointer<vtkQuad>::New();
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
  vtkSmartPointer<vtkPixel> aPixel =
    vtkSmartPointer<vtkPixel>::New();
  aPixel->GetPointIds()->SetId(0,0);
  aPixel->GetPointIds()->SetId(1,1);
  aPixel->GetPointIds()->SetId(2,3);
  aPixel->GetPointIds()->SetId(3,2);

  aPixel->GetPoints()->SetPoint (0, 10.0, 10.0, 10.0);
  aPixel->GetPoints()->SetPoint (1, 12.0, 10.0, 10.0);
  aPixel->GetPoints()->SetPoint (3, 12.0, 12.0, 10.0);
  aPixel->GetPoints()->SetPoint (2, 10.0, 12.0, 10.0);
  return aPixel;
}

vtkSmartPointer<vtkVoxel> MakeVoxel()
{
  vtkSmartPointer<vtkVoxel> aVoxel =
    vtkSmartPointer<vtkVoxel>::New();
  aVoxel->GetPointIds()->SetId(0,0);
  aVoxel->GetPointIds()->SetId(1,1);
  aVoxel->GetPointIds()->SetId(2,3);
  aVoxel->GetPointIds()->SetId(3,2);
  aVoxel->GetPointIds()->SetId(4,4);
  aVoxel->GetPointIds()->SetId(5,5);
  aVoxel->GetPointIds()->SetId(6,7);
  aVoxel->GetPointIds()->SetId(7,6);

  aVoxel->GetPoints()->SetPoint(0, 10, 10, 10);
  aVoxel->GetPoints()->SetPoint(1, 12, 10, 10);
  aVoxel->GetPoints()->SetPoint(3, 12, 12, 10);
  aVoxel->GetPoints()->SetPoint(2, 10, 12, 10);
  aVoxel->GetPoints()->SetPoint(4, 10, 10, 12);
  aVoxel->GetPoints()->SetPoint(5, 12, 10, 12);
  aVoxel->GetPoints()->SetPoint(7, 12, 12, 12);
  aVoxel->GetPoints()->SetPoint(6, 10, 12, 12);
  return aVoxel;
}

vtkSmartPointer<vtkHexahedron> MakeHexahedron()
{
  vtkSmartPointer<vtkHexahedron> aHexahedron =
    vtkSmartPointer<vtkHexahedron>::New();
  aHexahedron->GetPointIds()->SetId(0,0);
  aHexahedron->GetPointIds()->SetId(1,1);
  aHexahedron->GetPointIds()->SetId(2,2);
  aHexahedron->GetPointIds()->SetId(3,3);
  aHexahedron->GetPointIds()->SetId(4,4);
  aHexahedron->GetPointIds()->SetId(5,5);
  aHexahedron->GetPointIds()->SetId(6,6);
  aHexahedron->GetPointIds()->SetId(7,7);

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

vtkSmartPointer<vtkPyramid> MakePyramid()
{
  vtkSmartPointer<vtkPyramid> aPyramid =
    vtkSmartPointer<vtkPyramid>::New();
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
  vtkSmartPointer<vtkQuadraticPyramid> aPyramid =
    vtkSmartPointer<vtkQuadraticPyramid>::New();
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

  aPyramid->GetPoints()->SetPoint(9,  0.5,  0.5,  0.5);
  aPyramid->GetPoints()->SetPoint(10, 0.75, 0.5,  0.5);
  aPyramid->GetPoints()->SetPoint(11, 0.75, 0.75, 0.5);
  aPyramid->GetPoints()->SetPoint(12, 0.5,  0.75, 0.5);

  return aPyramid;
}

vtkSmartPointer<vtkQuadraticEdge> MakeQuadraticEdge()
{
  vtkSmartPointer<vtkQuadraticEdge> anEdge =
    vtkSmartPointer<vtkQuadraticEdge>::New();
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
  double *pcoords = aHexahedron->GetParametricCoords();
  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
    aHexahedron->GetPoints()->SetPoint(i,
                                       *(pcoords + 3 * i) + vtkMath::Random(-.1,.1),
                                       *(pcoords + 3 * i + 1) + vtkMath::Random(-.1,.1),
                                       *(pcoords + 3 * i + 2) + vtkMath::Random(-.1,.1));
  }
  return aHexahedron;
}

vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron> MakeBiQuadraticQuadraticHexahedron()
{
  vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron> aHexahedron =
    vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron>::New();
  double *pcoords = aHexahedron->GetParametricCoords();
  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
    aHexahedron->GetPoints()->SetPoint(i,
                                       *(pcoords + 3 * i) + vtkMath::Random(-.1,.1),
                                       *(pcoords + 3 * i + 1) + vtkMath::Random(-.1,.1),
                                       *(pcoords + 3 * i + 2) + vtkMath::Random(-.1,.1));
  }
  return aHexahedron;
}

vtkSmartPointer<vtkTriQuadraticHexahedron> MakeTriQuadraticHexahedron()
{
  vtkSmartPointer<vtkTriQuadraticHexahedron> aHexahedron =
    vtkSmartPointer<vtkTriQuadraticHexahedron>::New();
  double *pcoords = aHexahedron->GetParametricCoords();
  for (int i = 0; i < aHexahedron->GetNumberOfPoints(); ++i)
  {
    aHexahedron->GetPointIds()->SetId(i, i);
    aHexahedron->GetPoints()->SetPoint(i,
                                       *(pcoords + 3 * i) + vtkMath::Random(-.1,.1),
                                       *(pcoords + 3 * i + 1) + vtkMath::Random(-.1,.1),
                                       *(pcoords + 3 * i + 2) + vtkMath::Random(-.1,.1));
  }
  return aHexahedron;
}

vtkSmartPointer<vtkQuadraticPolygon> MakeQuadraticPolygon()
{
  vtkSmartPointer<vtkQuadraticPolygon> aPolygon =
    vtkSmartPointer<vtkQuadraticPolygon>::New();

  aPolygon->GetPointIds()->SetNumberOfIds(8);
  aPolygon->GetPointIds()->SetId(0,0);
  aPolygon->GetPointIds()->SetId(1,1);
  aPolygon->GetPointIds()->SetId(2,2);
  aPolygon->GetPointIds()->SetId(3,3);
  aPolygon->GetPointIds()->SetId(4,4);
  aPolygon->GetPointIds()->SetId(5,5);
  aPolygon->GetPointIds()->SetId(6,6);
  aPolygon->GetPointIds()->SetId(7,7);

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
  double *pcoords = aLinearQuad->GetParametricCoords();
  for (int i = 0; i < aLinearQuad->GetNumberOfPoints(); ++i)
  {
    aLinearQuad->GetPointIds()->SetId(i, i);
    aLinearQuad->GetPoints()->SetPoint(i,
                                       *(pcoords + 3 * i),
                                       *(pcoords + 3 * i + 1),
                                       *(pcoords + 3 * i + 2));
  }
  return aLinearQuad;
}

vtkSmartPointer<vtkQuadraticLinearWedge> MakeQuadraticLinearWedge()
{
  vtkSmartPointer<vtkQuadraticLinearWedge> aLinearWedge =
    vtkSmartPointer<vtkQuadraticLinearWedge>::New();
  double *pcoords = aLinearWedge->GetParametricCoords();
  for (int i = 0; i < 12; ++i)
  {
    aLinearWedge->GetPointIds()->SetId(i, i);
    aLinearWedge->GetPoints()->SetPoint(i,
                                       *(pcoords + 3 * i),
                                       *(pcoords + 3 * i + 1),
                                       *(pcoords + 3 * i + 2));
  }
  return aLinearWedge;
}

vtkSmartPointer<vtkQuadraticQuad> MakeQuadraticQuad()
{
  vtkSmartPointer<vtkQuadraticQuad> aQuad =
    vtkSmartPointer<vtkQuadraticQuad>::New();
  double *pcoords = aQuad->GetParametricCoords();
  for (int i = 0; i < 8; ++i)
  {
    aQuad->GetPointIds()->SetId(i, i);
    aQuad->GetPoints()->SetPoint(i,
                                 *(pcoords + 3 * i) + vtkMath::Random(-.1, .1),
                                 *(pcoords + 3 * i + 1) + vtkMath::Random(-.1, .1),
                                 *(pcoords + 3 * i + 2));
  }
  return aQuad;
}

vtkSmartPointer<vtkQuadraticTetra> MakeQuadraticTetra()
{
  vtkSmartPointer<vtkQuadraticTetra> aTetra =
    vtkSmartPointer<vtkQuadraticTetra>::New();
  double *pcoords = aTetra->GetParametricCoords();
  for (int i = 0; i < 10; ++i)
  {
    aTetra->GetPointIds()->SetId(i, i);
    aTetra->GetPoints()->SetPoint(i,
                                  *(pcoords + 3 * i) + vtkMath::Random(-.1, .1),
                                 *(pcoords + 3 * i + 1) + vtkMath::Random(-.1, .1),
                                 *(pcoords + 3 * i + 2) + vtkMath::Random(-.1, .1));
  }
  return aTetra;
}

vtkSmartPointer<vtkQuadraticTriangle> MakeQuadraticTriangle()
{
  vtkSmartPointer<vtkQuadraticTriangle> aTriangle =
    vtkSmartPointer<vtkQuadraticTriangle>::New();
  double *pcoords = aTriangle->GetParametricCoords();
  for (int i = 0; i < aTriangle->GetNumberOfPoints(); ++i)
  {
    aTriangle->GetPointIds()->SetId(i, i);
    aTriangle->GetPoints()->SetPoint(i,
                                     *(pcoords + 3 * i),
                                     *(pcoords + 3 * i + 1),
                                     *(pcoords + 3 * i + 2));
  }
  return aTriangle;
}

vtkSmartPointer<vtkBiQuadraticTriangle> MakeBiQuadraticTriangle()
{
  vtkSmartPointer<vtkBiQuadraticTriangle> aTriangle =
    vtkSmartPointer<vtkBiQuadraticTriangle>::New();
  double *pcoords = aTriangle->GetParametricCoords();
  for (int i = 0; i < aTriangle->GetNumberOfPoints(); ++i)
  {
    aTriangle->GetPointIds()->SetId(i, i);
    aTriangle->GetPoints()->SetPoint(i,
                                     *(pcoords + 3 * i),
                                     *(pcoords + 3 * i + 1),
                                     *(pcoords + 3 * i + 2));
  }
  return aTriangle;
}

vtkSmartPointer<vtkBiQuadraticQuad> MakeBiQuadraticQuad()
{
  vtkSmartPointer<vtkBiQuadraticQuad> aQuad =
    vtkSmartPointer<vtkBiQuadraticQuad>::New();
  double *pcoords = aQuad->GetParametricCoords();
  for (int i = 0; i < aQuad->GetNumberOfPoints(); ++i)
  {
    aQuad->GetPointIds()->SetId(i, i);
    aQuad->GetPoints()->SetPoint(i,
                                 *(pcoords + 3 * i) + vtkMath::Random(-.1, .1),
                                 *(pcoords + 3 * i + 1) + vtkMath::Random(-.1, .1),
                                 *(pcoords + 3 * i + 2));
  }
  return aQuad;
}

vtkSmartPointer<vtkCubicLine> MakeCubicLine()
{
  vtkSmartPointer<vtkCubicLine> aLine =
    vtkSmartPointer<vtkCubicLine>::New();
  double *pcoords = aLine->GetParametricCoords();
  for (int i = 0; i < aLine->GetNumberOfPoints(); ++i)
  {
    aLine->GetPointIds()->SetId(i, i);
    aLine->GetPoints()->SetPoint(i,
                                 *(pcoords + 3 * i),
                                 *(pcoords + 3 * i + 1),
                                 *(pcoords + 3 * i + 2));
  }
  return aLine;
}

vtkSmartPointer<vtkQuadraticWedge> MakeQuadraticWedge()
{
  vtkSmartPointer<vtkQuadraticWedge> aWedge =
    vtkSmartPointer<vtkQuadraticWedge>::New();
  double *pcoords = aWedge->GetParametricCoords();
  for (int i = 0; i < aWedge->GetNumberOfPoints(); ++i)
  {
    aWedge->GetPointIds()->SetId(i, i);
    aWedge->GetPoints()->SetPoint(i,
                                  *(pcoords + 3 * i),
                                  *(pcoords + 3 * i + 1),
                                  *(pcoords + 3 * i + 2));
  }
  return aWedge;
}

vtkSmartPointer<vtkBiQuadraticQuadraticWedge> MakeBiQuadraticQuadraticWedge()
{
  vtkSmartPointer<vtkBiQuadraticQuadraticWedge> aWedge =
    vtkSmartPointer<vtkBiQuadraticQuadraticWedge>::New();
  double *pcoords = aWedge->GetParametricCoords();
  for (int i = 0; i < aWedge->GetNumberOfPoints(); ++i)
  {
    aWedge->GetPointIds()->SetId(i, i);
    aWedge->GetPoints()->SetPoint(i,
                                  *(pcoords + 3 * i),
                                  *(pcoords + 3 * i + 1),
                                  *(pcoords + 3 * i + 2));
  }
  return aWedge;
}

vtkSmartPointer<vtkTetra> MakeTetra()
{
  vtkSmartPointer<vtkTetra> aTetra =
    vtkSmartPointer<vtkTetra>::New();
  aTetra->GetPointIds()->SetId(0,0);
  aTetra->GetPointIds()->SetId(1,1);
  aTetra->GetPointIds()->SetId(2,2);
  aTetra->GetPointIds()->SetId(3,3);
  aTetra->GetPoints()->SetPoint(0, 10.0, 10.0, 10.0);
  aTetra->GetPoints()->SetPoint(1, 12.0, 10.0, 10.0);
  aTetra->GetPoints()->SetPoint(2, 11.0, 12.0, 10.0);
  aTetra->GetPoints()->SetPoint(3, 11.0, 11.0, 12.0);
  return aTetra;
}

vtkSmartPointer<vtkWedge> MakeWedge()
{
  vtkSmartPointer<vtkWedge> aWedge =
    vtkSmartPointer<vtkWedge>::New();
  aWedge->GetPointIds()->SetId(0,0);
  aWedge->GetPointIds()->SetId(1,1);
  aWedge->GetPointIds()->SetId(2,2);
  aWedge->GetPointIds()->SetId(3,3);
  aWedge->GetPointIds()->SetId(4,4);
  aWedge->GetPointIds()->SetId(5,5);

  aWedge->GetPoints()->SetPoint(0, 10, 10, 10);
  aWedge->GetPoints()->SetPoint(1, 12, 10, 10);
  aWedge->GetPoints()->SetPoint(2, 11, 12, 10);
  aWedge->GetPoints()->SetPoint(3, 10, 10, 12);
  aWedge->GetPoints()->SetPoint(4, 12, 10, 12);
  aWedge->GetPoints()->SetPoint(5, 11, 12, 12);
  return aWedge;
}

vtkSmartPointer<vtkPolyhedron>MakeCube()
{
  vtkSmartPointer<vtkPolyhedron> aCube =
    vtkSmartPointer<vtkPolyhedron>::New();

  // create polyhedron (cube)
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();

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
  aCube->GetPoints()->SetPoint(0, -1.0,-1.0,-1.0);
  aCube->GetPoints()->SetPoint(1,  1.0,-1.0,-1.0);
  aCube->GetPoints()->SetPoint(2,  1.0, 1.0,-1.0);
  aCube->GetPoints()->SetPoint(3, -1.0, 1.0,-1.0);
  aCube->GetPoints()->SetPoint(4, -1.0,-1.0, 1.0);
  aCube->GetPoints()->SetPoint(5,  1.0,-1.0, 1.0);
  aCube->GetPoints()->SetPoint(6,  1.0, 1.0, 1.0);
  aCube->GetPoints()->SetPoint(7, -1.0, 1.0, 1.0);

  vtkIdType faces[31] =
    {6,              // number of faces
     4, 0, 3, 2, 1,
     4, 0, 4, 7, 3,
     4, 4, 5, 6, 7,
     4, 5, 1, 2, 6,
     4, 0, 1, 5, 4,
     4, 2, 3, 7, 6};

  aCube->SetFaces(faces);
  aCube->Initialize();
  return aCube;
}

vtkSmartPointer<vtkPolyhedron>MakeDodecahedron()
{
  vtkSmartPointer<vtkPolyhedron> aDodecahedron =
    vtkSmartPointer<vtkPolyhedron>::New();

  // create polyhedron (dodecahedron)
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();

  for (int i = 0; i < 20; ++i)
  {
    aDodecahedron->GetPointIds()->InsertNextId(i);
  }

  aDodecahedron->GetPoints()->InsertNextPoint(1.21412,    0,          1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.375185,   1.1547,     1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.982247,  0.713644,   1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.982247,  -0.713644,  1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.375185,   -1.1547,    1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(1.96449,    0,          0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.607062,   1.86835,    0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.58931,   1.1547,     0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.58931,   -1.1547,    0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.607062,   -1.86835,   0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(1.58931,    1.1547,     -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.607062,  1.86835,    -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.96449,   0,          -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.607062,  -1.86835,   -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(1.58931,    -1.1547,    -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.982247,   0.713644,   -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.375185,  1.1547,     -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.21412,   0,          -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.375185,  -1.1547,    -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.982247,   -0.713644,  -1.58931);

  vtkIdType faces[73] =
    {12,                   // number of faces
     5, 0, 1, 2, 3, 4,     // number of ids on face, ids
     5, 0, 5, 10, 6, 1,
     5, 1, 6, 11, 7, 2,
     5, 2, 7, 12, 8, 3,
     5, 3, 8, 13, 9, 4,
     5, 4, 9, 14, 5, 0,
     5, 15, 10, 5, 14, 19,
     5, 16, 11, 6, 10, 15,
     5, 17, 12, 7, 11, 16,
     5, 18, 13, 8, 12, 17,
     5, 19, 14, 9, 13, 18,
     5, 19, 18, 17, 16, 15};

  aDodecahedron->SetFaces(faces);
  aDodecahedron->Initialize();

  return aDodecahedron;
}

vtkSmartPointer<vtkPentagonalPrism> MakePentagonalPrism()
{
  vtkSmartPointer<vtkPentagonalPrism> aPentagonalPrism =
    vtkSmartPointer<vtkPentagonalPrism>::New();

  aPentagonalPrism->GetPointIds()->SetId(0,0);
  aPentagonalPrism->GetPointIds()->SetId(1,1);
  aPentagonalPrism->GetPointIds()->SetId(2,2);
  aPentagonalPrism->GetPointIds()->SetId(3,3);
  aPentagonalPrism->GetPointIds()->SetId(4,4);
  aPentagonalPrism->GetPointIds()->SetId(5,5);
  aPentagonalPrism->GetPointIds()->SetId(6,6);
  aPentagonalPrism->GetPointIds()->SetId(7,7);
  aPentagonalPrism->GetPointIds()->SetId(8,8);
  aPentagonalPrism->GetPointIds()->SetId(9,9);

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
  vtkSmartPointer<vtkHexagonalPrism> aHexagonalPrism =
    vtkSmartPointer<vtkHexagonalPrism>::New();
  aHexagonalPrism->GetPointIds()->SetId(0,0);
  aHexagonalPrism->GetPointIds()->SetId(1,1);
  aHexagonalPrism->GetPointIds()->SetId(2,2);
  aHexagonalPrism->GetPointIds()->SetId(3,3);
  aHexagonalPrism->GetPointIds()->SetId(4,4);
  aHexagonalPrism->GetPointIds()->SetId(5,5);
  aHexagonalPrism->GetPointIds()->SetId(6,6);
  aHexagonalPrism->GetPointIds()->SetId(7,7);
  aHexagonalPrism->GetPointIds()->SetId(8,8);
  aHexagonalPrism->GetPointIds()->SetId(9,9);
  aHexagonalPrism->GetPointIds()->SetId(10,10);
  aHexagonalPrism->GetPointIds()->SetId(11,11);

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

template<typename T> int TestOneCell(const VTKCellType cellType,
                                  vtkSmartPointer<T> aCell,
                                  int linear)
{
  int status = 0;
  std::cout << "Testing " << aCell->GetClassName() << std::endl;

  std::cout << "  Testing Print of an unitialized cell...";
  std::ostringstream cellPrint;
  aCell->Print(cellPrint);
  std::cout << "PASSED" << std::endl;

  std::cout << "  Testing GetCellType...";
  if (cellType != aCell->GetCellType())
  {
    std::cout << "Expected " << cellType
              << " but got " << aCell->GetCellType()
              << " FAILED" << std::endl;
    ++status;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }

  std::cout << "  Testing GetCellDimension...";
  std::cout << aCell->GetCellDimension();
  std::cout << "...PASSED" << std::endl;

  std::cout << "  Testing IsLinear...";
  if (aCell->IsLinear() != 1 && linear)
  {
    ++status;
    std::cout << "...FAILED" << std::endl;
  }
  else
  {
    std::cout << "...PASSED" << std::endl;
  }

  std::cout << "  Testing IsPrimaryCell...";
  std::cout << aCell->IsPrimaryCell();
  std::cout << "...PASSED" << std::endl;

  std::cout << "  Testing IsExplicitCell...";
  std::cout << aCell->IsExplicitCell();
  std::cout << "...PASSED" << std::endl;

  std::cout << "  Testing RequiresInitialization...";
  std::cout << aCell->RequiresInitialization();
  std::cout << "...PASSED" << std::endl;

  std::cout << "  Testing RequiresExplicitFaceRepresentation...";
  std::cout << aCell->RequiresExplicitFaceRepresentation();
  std::cout << "...PASSED" << std::endl;

  if (aCell->RequiresInitialization())
  {
    aCell->Initialize();
  }
  std::cout << "  Testing GetNumberOfPoints...";
  std::cout << aCell->GetNumberOfPoints();
  std::cout << "...PASSED" << std::endl;

  std::cout << "  Testing GetNumberOfEdges...";
  std::cout << aCell->GetNumberOfEdges();
  std::cout << "...PASSED" << std::endl;

  std::cout << "  Testing GetNumberOfFaces...";
  std::cout << aCell->GetNumberOfFaces();
  std::cout << "...PASSED" << std::endl;

  if (std::string(aCell->GetClassName()) != "vtkEmptyCell" &&
      std::string(aCell->GetClassName()) != "vtkVertex" &&
      std::string(aCell->GetClassName()) != "vtkPolyhedron")
  {
    std::cout << "  Testing GetParametricCoords...";
    double *parametricCoords = aCell->GetParametricCoords();
    if (aCell->IsPrimaryCell() && parametricCoords == NULL)
    {
      ++status;
      std::cout << "...FAILED" << std::endl;
    }
    else if (parametricCoords)
    {
      std::vector<double> pweights(aCell->GetNumberOfPoints());
      // The pcoords should correspond to the cell points
      for (int p = 0; p < aCell->GetNumberOfPoints(); ++p)
      {
        double vertex[3];
        aCell->GetPoints()->GetPoint(p, vertex);
        int subId = 0;
        double x[3];
        aCell->EvaluateLocation(subId, parametricCoords + 3 * p, x, &(*pweights.begin()));
        if (!vtkMathUtilities::FuzzyCompare(
              x[0], vertex[0], 1.e-3) ||
            !vtkMathUtilities::FuzzyCompare(
              x[1], vertex[1], 1.e-3) ||
            !vtkMathUtilities::FuzzyCompare(
              x[2], vertex[2], 1.e-3))
        {
          std::cout << "EvaluateLocation failed...";
          std::cout << "pcoords[" << p << "]: "
                    << parametricCoords[3 * p] << " "
                    << parametricCoords[3 * p  + 1] << " "
                    << parametricCoords[3 * p + 2] << std::endl;
          std::cout << "x[" << p << "]: "
                    << x[0] << " " << x[1] << " " << x[2] << std::endl;
          std::cout << "...FAILED" << std::endl;
          ++status;
        }
      }
      std::cout << "...PASSED" << std::endl;
    }
  }
  std::cout << "  Testing GetBounds...";
  double bounds[6];
  aCell->GetBounds(bounds);
  std::cout << bounds[0] << "," << bounds[1] << " "
            << bounds[2] << "," << bounds[3] << " "
            << bounds[4] << "," << bounds[5];
  std::cout << "...PASSED" << std::endl;

  if (aCell->GetNumberOfPoints() > 0)
  {
    std::cout << "  Testing GetParametricCenter...";
    double pcenter[3], center[3];
    pcenter[0] = pcenter[1] = pcenter[2] = -12345.0;
    aCell->GetParametricCenter(pcenter);
    std::cout << pcenter[0] << ", " << pcenter[1] << ", " << pcenter[2];
    std::vector<double> cweights(aCell->GetNumberOfPoints());
    int pSubId = 0;
    aCell->EvaluateLocation(pSubId, pcenter, center, &(*cweights.begin()));
    std::cout << " -> " << center[0] << ", " << center[1] << ", " << center[2];
    if (center[0] < bounds[0] || center[0] > bounds[1] ||
        center[1] < bounds[2] || center[1] > bounds[3] ||
        center[2] < bounds[4] || center[2] > bounds[5])
    {
      std::cout << " The computed center is not within the bounds of the cell" << std::endl;
      std::cout << "bounds: "
                << bounds[0] << "," << bounds[1] << " "
                << bounds[2] << "," << bounds[3] << " "
                << bounds[4] << "," << bounds[5]
                << std::endl;
      std::cout << "parametric center "
                << pcenter[0] << ", " << pcenter[1] << ", " << pcenter[2] << " "
                << "center: "
                << center[0] << ", " << center[1] << ", " << center[2]
                << std::endl;
      std::cout << "...FAILED" << std::endl;
    }
    else
    {
      std::cout << "...PASSED" << std::endl;
    }
  }

  std::cout << "  Testing GetParametricDistance...";
  double pcenter[3];
  aCell->GetParametricCenter(pcenter);
  double pd = aCell->GetParametricDistance(pcenter);
  if (pd == 0.0)
  {
    std::cout << "...PASSED" << std::endl;
  }
  else
  {
    ++status;
    std::cout << "...FAILED" << std::endl;
  }

  std::cout << "  Testing CellBoundaries...";
  vtkSmartPointer<vtkIdList> cellIds =
    vtkSmartPointer<vtkIdList>::New();
  int cellStatus = aCell->CellBoundary(0, pcenter, cellIds);
  if (aCell->GetCellDimension() > 0 && cellStatus != 1)
  {
    ++status;
    std::cout << "FAILED" << std::endl;
  }
  else
  {
    for (int c = 0; c < cellIds->GetNumberOfIds(); ++c)
    {
      std::cout << " " << cellIds->GetId(c) << ", ";
    }
    std::cout << "PASSED" << std::endl;
  }

  if (aCell->GetNumberOfPoints() > 0 &&
      strcmp(aCell->GetClassName(), "vtkQuadraticEdge") != 0 )
  {
    std::cout << "  Testing Derivatives...";
    // Create scalars and set first scalar to 1.0
    std::vector<double> scalars(aCell->GetNumberOfPoints());
    scalars[0] = 1.0;
    for (int s = 1; s < aCell->GetNumberOfPoints(); ++s)
    {
      scalars[s] = 0.0;
    }
    std::vector<double> derivs(3, -12345.0);
    aCell->Derivatives(0, pcenter, &(*scalars.begin()), 1, &(*derivs.begin()));
    if (derivs[0] == -12345. && derivs[1] == -12345. && derivs[2] == -12345.)
    {
      std::cout << " not computed";
    }
    else
    {
      std::cout << " "
                << derivs[0] << " "
                << derivs[1] << " "
                << derivs[2] << " ";
    }
    std::cout << "...PASSED" << std::endl;
  }

  std::cout << "  Testing EvaluateLocation vertex matches pcoord...";
  int status5 = 0;
  double *locations = aCell->GetParametricCoords();
  if (locations)
  {
    std::vector<double> lweights(aCell->GetNumberOfPoints());
    for (int l = 0; l < aCell->GetNumberOfPoints(); ++l)
    {
      double point[3];
      double vertex[3];
      aCell->GetPoints()->GetPoint(l, vertex);
      int subId = 0;
      aCell->EvaluateLocation(subId, locations + 3 * l, point, &(*lweights.begin()));
      for (int v = 0; v < 3; ++v)
      {
        if (!vtkMathUtilities::FuzzyCompare(
              point[v], vertex[v],
              1.e-3))
        {
          std::cout << " " << point[0] << ", " << point[1] << ", " << point[2] << " != "
                    << vertex[0] << ", " << vertex[1] << ", " << vertex[2] << " " ;
          std::cout << "eps ratio is: " << (point[v] - vertex[v])
            / std::numeric_limits<double>::epsilon() << std::endl;

          ++status5;
          break;
        }
      }
    }
  }
  if (status5)
  {
    std::cout << "...FAILED" << std::endl;
    ++status;
  }
  else
  {
    std::cout << "...PASSED"<< std::endl;
  }

  std::cout << "  Testing EvaluatePosition pcoord matches vertex...";
  // Each vertex should corrrespond to a pcoord.
  int subId = 0;
  int status6 = 0;
  std::vector<double> weights(aCell->GetNumberOfPoints());
  double *vlocations = aCell->GetParametricCoords();
  if (vlocations)
  {
    for (int i = 0; i < aCell->GetNumberOfPoints(); ++i)
    {
      int status61 = 0;
      double closestPoint[3];
      double point[3];
      double pcoords[3];
      double dist2;
      aCell->GetPoints()->GetPoint(i, point);
      aCell->EvaluatePosition( point, closestPoint, subId, pcoords, dist2, &(*weights.begin()));
      for (int v = 0; v < 3; ++v)
      {
        if (!vtkMathUtilities::FuzzyCompare(
              *(vlocations + 3 * i + v) ,pcoords[v],
              1.e-3))
        {
          ++status61;
        }
      }
      if (status61)
      {
        std::cout << std::endl
                  << *(vlocations + 3 * i + 0) << ", "
                  << *(vlocations + 3 * i + 1) << ", "
                  << *(vlocations + 3 * i + 2)
                  << " != "
                  << pcoords[0] << ", "
                  << pcoords[1] << ", "
                  << pcoords[2] << " " ;
        ++status6;
      }
    }
  }
  if (status6)
  {
    ++status;
    std::cout << "...FAILED" << std::endl;
  }
  else
  {
    std::cout << "...PASSED" << std::endl;
  }

  std::cout << "  Testing EvaluatePosition in/out test...";

  int status2 = 0;
  std::vector<std::vector<double> > testPoints;
  std::vector<int> inOuts;
  std::vector<std::string> typePoint;

  // First test cell points
  for (int i = 0; i < aCell->GetNumberOfPoints(); ++i)
  {
    std::vector<double> point(3);
    aCell->GetPoints()->GetPoint(i, &(*point.begin()));
    testPoints.push_back(point);
    inOuts.push_back(1);
    typePoint.push_back("cell point");
  }
  // Then test center of cell
  if (aCell->GetNumberOfPoints() > 0)
  {
    std::vector<double> tCenter(3);
    aCell->EvaluateLocation(subId, pcenter, &(*tCenter.begin()), &(*weights.begin()));
    testPoints.push_back(tCenter);
    inOuts.push_back(1);
    typePoint.push_back("cell center");
    // Test a point above the cell
    if (aCell->GetCellDimension() == 2)
    {
      std::vector<double> above(3);
      above[0] = tCenter[0]; above[1] = tCenter[1];
      above[2] = tCenter[2] + aCell->GetLength2();
      testPoints.push_back(above);
      inOuts.push_back(0);
      typePoint.push_back("point above cell");
    }
  }

  // Test points at the center of each edge
  for (int e = 0; e < aCell->GetNumberOfEdges(); ++e)
  {
    std::vector<double> eCenter(3);
    vtkCell *c = aCell->GetEdge(e);
    c->GetParametricCenter(pcenter);
    c->EvaluateLocation(subId, pcenter, &(*eCenter.begin()), &(*weights.begin()));
    testPoints.push_back(eCenter);
    typePoint.push_back("edge center");
    inOuts.push_back(1);
  }

  // Test points at the center of each face
  for (int f = 0; f < aCell->GetNumberOfFaces(); ++f)
  {
    std::vector<double> fCenter(3);
    vtkCell *c = aCell->GetFace(f);
    c->GetParametricCenter(pcenter);
    c->EvaluateLocation(subId, pcenter, &(*fCenter.begin()), &(*weights.begin()));
    testPoints.push_back(fCenter);
    inOuts.push_back(1);
    typePoint.push_back("face center");
  }

  // Test a point outside the cell
  if (aCell->GetNumberOfPoints() > 0)
  {
    std::vector<double> outside(3, -12345.0);
    testPoints.push_back(outside);
    inOuts.push_back(0);
    typePoint.push_back("outside point");
  }
  for (size_t p = 0; p < testPoints.size(); ++p)
  {
    double closestPoint[3], pcoords[3], dist2;
    int inOut = aCell->EvaluatePosition( &(*testPoints[p].begin()),
                                         closestPoint, subId,
                                         pcoords, dist2,
                                         &(*weights.begin()));
    if ((inOut == 0 || inOut == -1) && inOuts[p] == 0)
    {
      continue;
    }
    else if (inOut == 1 && dist2 == 0.0 && inOuts[p] == 1)
    {
      continue;
    }
    else if (inOut == 1 && dist2 != 0.0 && inOuts[p] == 0)
    {
      continue;
    }
    // inOut failed
    std::cout << typePoint[p] << " failed inOut: " << inOut << " "
              << "point: " <<testPoints[p][0] << ", " << testPoints[p][1] << ", " << testPoints[p][2] << "-> "
              << "pcoords: " << pcoords[0] << ", " << pcoords[1] << ", " << pcoords[2] << ": "
              << "closestPoint: " << closestPoint[0] << ", " << closestPoint[1] << ", " << closestPoint[2] << " "
              << "dist2: " << dist2;
    std::cout << " weights: ";
    for (int w = 0; w < aCell->GetNumberOfPoints(); ++w)
    {
      std::cout << weights[w] << " ";
    }
    std::cout << std::endl;
    status2 += 1;
  }
  if (status2)
  {
    ++status;
    std::cout << "FAILED" << std::endl;
  }
  else
  {
    std::cout << "PASSED" << std::endl;
  }

  if (aCell->GetNumberOfPoints() > 0 &&
      aCell->GetCellDimension() > 0)
  {
    std::cout << "  Testing IntersectWithLine...";
    double tol = 1.e-5;
    double t;
    double startPoint[3];
    double endPoint[3];
    double intersection[3], pintersection[3];
    aCell->GetParametricCenter(pcenter);
    aCell->EvaluateLocation(subId, pcenter, startPoint, &(*weights.begin()));
    endPoint[0] = startPoint[0];
    endPoint[1] = startPoint[1];
    endPoint[2] = startPoint[2] + aCell->GetLength2();
    startPoint[2] = startPoint[2] - aCell->GetLength2();
    int status3 = 0;
    int result =
      aCell->IntersectWithLine(
        startPoint, endPoint,
        tol,
        t,
        intersection,
        pintersection,
        subId);
    if (result == 0)
    {
      ++status3;
    }
    else
    {
      std::cout << " t: " << t << " ";
    }
    startPoint[2] = endPoint[2] + aCell->GetLength2();
    result =
      aCell->IntersectWithLine(
        startPoint, endPoint,
        tol,
        t,
        intersection,
        pintersection,
        subId);
    if (result == 1)
    {
      ++status3;
    }

    if (status3 != 0)
    {
      ++status;
      std::cout << "...FAILED" << std::endl;
    }
    else
    {
      std::cout << "...PASSED" << std::endl;
    }
  }

  // Triangulate
  std::cout << "  Testing Triangulate...";
  int index = 0;
  vtkSmartPointer<vtkIdList> ptIds =
    vtkSmartPointer<vtkIdList>::New();
  ptIds->SetNumberOfIds(100);
  vtkSmartPointer<vtkPoints> triPoints =
    vtkSmartPointer<vtkPoints>::New();
  aCell->Triangulate(index, ptIds, triPoints);
  int pts = ptIds->GetNumberOfIds();
  if (aCell->GetCellDimension() == 0)
  {
    std::cout << "Generated " << pts << " Points";
  }
  else if (aCell->GetCellDimension() == 1)
  {
    std::cout << "Generated " << pts / 2 << " Lines";
  }
  else if (aCell->GetCellDimension() == 2)
  {
    std::cout << "Generated " << pts / 3 << " Triangles";
  }
  else if (aCell->GetCellDimension() == 3)
  {
    std::cout << "Generated " << pts / 4 << " Tetra";
  }
  std::cout << "...PASSED" << std::endl;

  if (status)
  {
    std::cout << aCell->GetClassName() << " FAILED" << std::endl;
  }
  else
  {
    std::cout << aCell->GetClassName() << " PASSED" << std::endl;
  }
  return status;
}
