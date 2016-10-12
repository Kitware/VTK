/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPentagonalPrism.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkCellType.h"

#include "vtkPentagonalPrism.h"
#include "vtkHexagonalPrism.h"

#include "vtkMathUtilities.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include <sstream>
#include <vector>
#include <string>
#include <map>

vtkSmartPointer<vtkPentagonalPrism> MakePentagonalPrism();
vtkSmartPointer<vtkHexagonalPrism> MakeHexagonalPrism();

template<typename T> int TestCell(const VTKCellType cellType, vtkSmartPointer<T> cell);

//----------------------------------------------------------------------------
int TestPentagonalPrism(int, char*[])
{
  std::map<std::string,int> results;

  results["PentagonalPrism"] = TestCell<vtkPentagonalPrism>(VTK_PENTAGONAL_PRISM, MakePentagonalPrism());
  results["HexagonalPrism"] = TestCell<vtkHexagonalPrism>(VTK_HEXAGONAL_PRISM, MakeHexagonalPrism());

  int status = 0;
  std::cout << "----- Unit Test Summary -----" << std::endl;
  std::map <std::string, int>::iterator it;
  for (it = results.begin(); it != results.end(); ++it)
  {
    std:: cout << std::setw(25)
               << it->first << " "  << (it->second ? " FAILED" : " OK")
               << std::endl;
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

template<typename T> int TestCell(const VTKCellType cellType,
                                  vtkSmartPointer<T> aCell)
{
  int status = 0;;
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
  if (aCell->IsLinear() != 1)
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

  std::cout << "  Testing GetParametricCoords...";
  double *parametricCoords = aCell->GetParametricCoords();
  if (aCell->IsPrimaryCell() && parametricCoords == NULL)
  {
    ++status;
    std::cout << "...FAILED" << std::endl;
  }
  else
  {
    double *pweights = new double[aCell->GetNumberOfPoints()];
    // The pcoords should correspond to the cell points
    for (int p = 0; p < aCell->GetNumberOfPoints(); ++p)
    {
      double vertex[3];
      aCell->GetPoints()->GetPoint(p, vertex);
      int subId = 0;
      double x[3];
      aCell->EvaluateLocation(subId, parametricCoords + 3 * p, x, pweights);
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
    delete [] pweights;
    std::cout << "...PASSED" << std::endl;
  }

  std::cout << "  Testing GetBounds...";
  double bounds[6];
  aCell->GetBounds(bounds);
  std::cout << bounds[0] << "," << bounds[1] << " "
            << bounds[2] << "," << bounds[3] << " "
            << bounds[4] << "," << bounds[5];
  std::cout << "...PASSED" << std::endl;

  std::cout << "  Testing GetParametricCenter...";
  double pcenter[3], center[3];
  pcenter[0] = pcenter[1] = pcenter[2] = -12345.0;
  aCell->GetParametricCenter(pcenter);
  std::cout << pcenter[0] << ", " << pcenter[1] << ", " << pcenter[2];
  double *cweights = new double[aCell->GetNumberOfPoints()];
  int pSubId = 0;
  aCell->EvaluateLocation(pSubId, pcenter, center, cweights);
  if (center[0] < bounds[0] || center[0] > bounds[1] ||
      center[1] < bounds[2] || center[1] > bounds[3] ||
      center[2] < bounds[4] || center[2] > bounds[5])
  {
    std::cout << "The computed center is not within the bounds of the cell" << std::endl;
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
  delete []cweights;

  std::cout << "  Testing GetParametricDistance...";
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

  std::cout << "  Testing Derivatives...";
  // Create scalars and set first scalar to 1.0
  double *scalars = new double[aCell->GetNumberOfPoints()];
  *scalars = 1.0;
  for (int s = 1; s < aCell->GetNumberOfPoints(); ++s)
  {
    *(scalars + s) = 0.0;
  }
  double *derivs = new double[3];
  *derivs = -12345.;
  aCell->Derivatives(0, pcenter, scalars, 1, derivs);
  std::cout << " "
            << derivs[0] << " "
            << derivs[1] << " "
            << derivs[2] << " ";
  delete [] derivs;
  delete [] scalars;
  std::cout << "...PASSED" << std::endl;

  std::cout << "  Testing EvaluateLocation vertex matches pcoord...";
  int status5 = 0;
  double *locations = aCell->GetParametricCoords();
  if (locations)
  {
    double *lweights = new double[aCell->GetNumberOfPoints()];
    for (int l = 0; l < aCell->GetNumberOfPoints(); ++l)
    {
      double point[3];
      double vertex[3];
      aCell->GetPoints()->GetPoint(l, vertex);
      int subId = 0;
      aCell->EvaluateLocation(subId, locations + 3 * l, point, lweights);
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
    delete []lweights;
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
  double *weights = new double[aCell->GetNumberOfPoints()];
  double *vlocations = aCell->GetParametricCoords();
  for (int i = 0; i < aCell->GetNumberOfPoints(); ++i)
  {
    int status61 = 0;
    double closestPoint[3];
    double point[3];
    double pcoords[3];
    double dist2;
    aCell->GetPoints()->GetPoint(i, point);
    aCell->EvaluatePosition( point, closestPoint, subId, pcoords, dist2, weights);
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
  std::vector<double *> testPoints;
  std::vector<int> inOuts;
  std::vector<std::string> typePoint;

  // First test cell points
  for (int i = 0; i < aCell->GetNumberOfPoints(); ++i)
  {
    double *point = new double[3];
    aCell->GetPoints()->GetPoint(i, point);
    testPoints.push_back(point);
    inOuts.push_back(1);
    typePoint.push_back("cell point");
  }
  // Then test center of cell
  if (aCell->GetNumberOfPoints() > 0)
  {
    double *tCenter = new double[3];
    aCell->EvaluateLocation(subId, pcenter, tCenter, weights);
    testPoints.push_back(tCenter);
    inOuts.push_back(1);
    typePoint.push_back("cell center");
    // Test a point above the cell
    if (aCell->GetCellDimension() == 2)
    {
      double *above = new double[3];
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
    double *eCenter = new double[3];
    vtkCell *c = aCell->GetEdge(e);
    c->GetParametricCenter(pcenter);
    c->EvaluateLocation(subId, pcenter, eCenter, weights);
    testPoints.push_back(eCenter);
    typePoint.push_back("edge center");
    inOuts.push_back(1);
  }

  // Test points at the center of each face
  for (int f = 0; f < aCell->GetNumberOfFaces(); ++f)
  {
    double *fCenter = new double[3];
    vtkCell *c = aCell->GetFace(f);
    c->GetParametricCenter(pcenter);
    c->EvaluateLocation(subId, pcenter, fCenter, weights);
    testPoints.push_back(fCenter);
    inOuts.push_back(1);
    typePoint.push_back("face center");
  }

  // Test a point outside the cell
  if (aCell->GetNumberOfPoints() > 0)
  {
    double *outside = new double[3];
    outside[0] = outside[1] = outside[2] = -12345.0;
    testPoints.push_back(outside);
    inOuts.push_back(0);
    typePoint.push_back("outside point");
  }
  for (size_t p = 0; p < testPoints.size(); ++p)
  {
    double closestPoint[3], pcoords[3], dist2;
    int inOut = aCell->EvaluatePosition( testPoints[p], closestPoint, subId, pcoords, dist2, weights);
    if ((inOut == 0 || inOut == -1) && inOuts[p] == 0)
    {
      delete []testPoints[p];
      continue;
    }
    else if (inOut == 1 && dist2 == 0.0 && inOuts[p] == 1)
    {
      delete []testPoints[p];
      continue;
    }
    else if (inOut == 1 && dist2 != 0.0 && inOuts[p] == 0)
    {
      delete []testPoints[p];
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
    delete []testPoints[p];
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
      aCell->GetCellDimension() > 1)
  {
    std::cout << "  Testing IntersectWithLine...";
    double tol = 1.e-5;
    double t;
    double startPoint[3];
    double endPoint[3];
    double intersection[3], pintersection[3];
    aCell->GetParametricCenter(pcenter);
    aCell->EvaluateLocation(subId, pcenter, startPoint, weights);
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
  delete []weights;
  return status;
}
