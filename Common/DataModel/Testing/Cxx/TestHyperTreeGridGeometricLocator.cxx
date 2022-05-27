/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridGeometricLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

#include "vtk_eigen.h"
#include VTK_EIGEN(Core)

#include "vtkNew.h"

#include "vtkBitArray.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometricLocator.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"
#include "vtkIdList.h"
#include "vtkPoints.h"

namespace htggeomlocator
{
typedef std::pair<std::vector<double>, bool> SearchPair;

struct TestResults
{
  bool OutsidePointSearch = false;
  bool OuterEdgeSearch = false;
  bool MaskedSearch = false;
  bool IntersectDiagonal = false;
  bool IntersectMaskedDiagonal = false;
  bool AllIntersectsDiagonal = false;
  std::vector<SearchPair> points;
}; // TestResult

bool runOutsidePointSearch(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* theseResults)
{
  bool success = false;
  double inf = std::numeric_limits<double>::infinity();
  double pt[3] = { inf, inf, inf };
  vtkIdType globId = htgLoc->Search(pt);
  success = (globId < 0);
  theseResults->OutsidePointSearch = success;
  if (!success)
  {
    std::cout << "Outside Point Search failed, found global ID " << globId << "\n";
  }
  return success;
} // runOutsidePointSearch

bool runOuterEdgeSearch(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* theseResults)
{
  bool success = false;
  double pt[3]{ htgLoc->GetHTG()->GetXCoordinates()->GetComponent(
                  htgLoc->GetHTG()->GetXCoordinates()->GetNumberOfTuples() - 1, 0),
    htgLoc->GetHTG()->GetYCoordinates()->GetComponent(
      htgLoc->GetHTG()->GetYCoordinates()->GetNumberOfTuples() - 1, 0),
    htgLoc->GetHTG()->GetZCoordinates()->GetComponent(
      htgLoc->GetHTG()->GetZCoordinates()->GetNumberOfTuples() - 1, 0) };
  // do edge point
  vtkIdType globId = htgLoc->Search(pt);
  success = (globId < 0);
  theseResults->OuterEdgeSearch = success;
  if (!success)
  {
    std::cout << "Outer Edge Search failed, found global ID " << globId << "\n";
  }
  return success;
} // runOuterEdgeSearch

bool runMaskedPointSearch(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* theseResults)
{
  bool success = true;
  double eps = 1e-3;
  double pt[3] = { 0, 0, 0 };
  pt[0] = htgLoc->GetHTG()->GetXCoordinates()->GetComponent(0, 0) + eps;
  if (htgLoc->GetHTG()->GetDimension() > 1)
  {
    pt[1] = htgLoc->GetHTG()->GetYCoordinates()->GetComponent(0, 0) + eps;
  }
  if (htgLoc->GetHTG()->GetDimension() > 2)
  {
    pt[2] = htgLoc->GetHTG()->GetZCoordinates()->GetComponent(0, 0) + eps;
  }
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursorFirst;
  vtkIdType globId = htgLoc->Search(pt, cursorFirst);
  success = (globId >= 0);
  cursorFirst->SetMask(true);
  if (success)
  {
    globId = htgLoc->Search(pt);
    success = (globId < 0);
  }
  cursorFirst->SetMask(false);
  theseResults->MaskedSearch = success;
  if (!success)
  {
    std::cout << "Masked point search failed" << std::endl;
  }
  return success;
}

bool runAllMaskedPointSearch(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* theseResults)
{
  bool success = true;
  double eps = 1e-6;
  double pt[3] = { htgLoc->GetHTG()->GetXCoordinates()->GetComponent(0, 0) + eps,
    htgLoc->GetHTG()->GetYCoordinates()->GetComponent(0, 0) + eps,
    htgLoc->GetHTG()->GetZCoordinates()->GetComponent(0, 0) + eps };
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursorFirst;
  success = (htgLoc->Search(pt, cursorFirst) > 0);
  cursorFirst->ToParent();
  vtkIdType globIdFirst = cursorFirst->GetGlobalNodeIndex();
  success = (globIdFirst > 0);
  vtkIdType globIdSecond = 0;
  if (success)
  {
    auto setChildrenMask = [htgLoc](vtkHyperTreeGridNonOrientedGeometryCursor* curse, bool state) {
      for (unsigned int d = 0; d < htgLoc->GetHTG()->GetNumberOfChildren(); d++)
      {
        curse->ToChild(d);
        curse->SetMask(state);
        curse->ToParent();
      }
    };
    setChildrenMask(cursorFirst, true);
    vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursorSecond;
    globIdSecond = htgLoc->Search(pt, cursorSecond);
    success = (globIdFirst == globIdSecond);
    setChildrenMask(cursorFirst, false);
  }
  if (!success)
  {
    std::cout << "All masked point search failed, parent of first found was " << globIdFirst
              << " while the result ofthe second search was " << globIdSecond << std::endl;
  }
  return success;
}

bool runPointSearch(vtkHyperTreeGridGeometricLocator* htgLoc, const double pt[3])
{
  bool success = true;
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  vtkIdType globId = htgLoc->Search(pt, cursor);
  if (globId == -1)
  {
    success = false;
  }
  if (success)
  {
    const double* origin = cursor->GetOrigin();
    const double* size = cursor->GetSize();
    for (unsigned int d = 0; d < htgLoc->GetHTG()->GetDimension(); d++)
    {
      double buff = pt[d] - origin[d];
      success = (buff < size[d]) && (buff >= 0.0);
      if (!success)
      {
        break;
      }
    }
  }
  if (!success)
  {
    std::cout << "Point search failed for point: \n";
    for (unsigned int d = 0; d < htgLoc->GetHTG()->GetDimension(); d++)
    {
      std::cout << pt[d] << ",";
    }
    std::cout << std::endl;
  }
  return success;
} // runPointSearch

bool runFindCell(vtkHyperTreeGridGeometricLocator* htgLoc, const double pt[3])
{
  bool success = false;
  vtkNew<vtkGenericCell> cell;
  int subId = 0;
  std::vector<double> pcoords(3, 0.0);
  std::vector<double> weights(
    static_cast<size_t>(std::pow(2, htgLoc->GetHTG()->GetDimension())), 0.0);
  vtkIdType globId = htgLoc->FindCell(pt, 0.0, cell, subId, pcoords.data(), weights.data());
  success = (globId >= 0);
  double eps = 1e-3;
  success = success && (1 - std::accumulate(weights.begin(), weights.end(), 0.0) < eps);
  if (!success)
  {
    std::cout << "FindCell failed for point: \n";
    for (unsigned int d = 0; d < htgLoc->GetHTG()->GetDimension(); d++)
    {
      std::cout << pt[d] << ",";
    }
    std::cout << std::endl;
  }
  return success;
}

bool runIntersectDiagonal(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* theseResults)
{
  bool success = false;
  std::vector<double> origin(3, 0.0);
  std::vector<double> diagPt(3, 0.0);
  unsigned int dim = htgLoc->GetHTG()->GetDimension();
  auto getDiagonalComponents = [](vtkDataArray* compArray, double& first, double& last) {
    first = compArray->GetComponent(0, 0);
    last = compArray->GetComponent(compArray->GetNumberOfTuples() - 1, 0);
  };
  getDiagonalComponents(htgLoc->GetHTG()->GetXCoordinates(), origin[0], diagPt[0]);
  if (dim > 1)
  {
    getDiagonalComponents(htgLoc->GetHTG()->GetYCoordinates(), origin[1], diagPt[1]);
  }
  if (dim > 2)
  {
    getDiagonalComponents(htgLoc->GetHTG()->GetZCoordinates(), origin[2], diagPt[2]);
  }
  double t = -1;
  std::vector<double> intercept(3, 0.0);
  std::vector<double> pcoords(3, 0.0);
  int subId = 0;
  vtkIdType cellId = -1;
  vtkNew<vtkGenericCell> cell;
  int resInter = htgLoc->IntersectWithLine(
    origin.data(), diagPt.data(), 0.0, t, intercept.data(), pcoords.data(), subId, cellId, cell);
  if (resInter != 0)
  {
    success = true;
  }
  if (success)
  {
    success = (cellId >= 0);
  }
  if (success)
  {
    success = (t < 1e-6);
  }
  if (success)
  {
    for (unsigned int d = 0; d < dim; d++)
    {
      success = (intercept[d] == origin[d]);
      if (!success)
      {
        break;
      }
    }
  }
  theseResults->IntersectDiagonal = success;
  if (!success)
  {
    std::cout << "Failed diagonal intersection" << std::endl;
  }
  return success;
}

bool runIntersectWithPoints(vtkHyperTreeGridGeometricLocator* htgLoc, const double pt[3])
{
  bool success = false;
  double ref[3] = { -1, 1, 0.0 };
  unsigned int dim = htgLoc->GetHTG()->GetDimension();
  if (dim == 3)
  {
    ref[2] = -1.0;
  }
  vtkNew<vtkGenericCell> cell;
  int subId = 0;
  std::vector<double> intercept(3, 0.0);
  std::vector<double> pcoords(3, 0.0);
  vtkIdType cellId = -1;
  double t = -1;
  double copyPt[3] = { 0.0, 0.0, 0.0 };
  std::copy(pt, pt + dim, copyPt);
  int resInter = htgLoc->IntersectWithLine(
    copyPt, ref, 0.0, t, intercept.data(), pcoords.data(), subId, cellId, cell);
  if (resInter != 0)
  {
    success = true;
  }
  if (success)
  {
    success = (cellId >= 0);
  }
  if (success)
  {
    vtkIdType searchId = htgLoc->Search(pt);
    success = (cellId == searchId);
  }
  if (!success)
  {
    std::cout << "Intersect with points failed for point: \n";
    for (unsigned int d = 0; d < htgLoc->GetHTG()->GetDimension(); d++)
    {
      std::cout << pt[d] << ",";
    }
    std::cout << std::endl;
  }
  return success;
}

bool runIntersectWithMaskDiagonal(
  vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* theseResults)
{
  bool success = false;
  std::vector<double> origin(3, 0.0);
  std::vector<double> diagPt(3, 0.0);
  unsigned int dim = htgLoc->GetHTG()->GetDimension();
  auto getDiagonalComponents = [](vtkDataArray* compArray, double& first, double& last) {
    first = compArray->GetComponent(0, 0);
    last = compArray->GetComponent(compArray->GetNumberOfTuples() - 1, 0);
  };
  getDiagonalComponents(htgLoc->GetHTG()->GetXCoordinates(), origin[0], diagPt[0]);
  if (dim > 1)
  {
    getDiagonalComponents(htgLoc->GetHTG()->GetYCoordinates(), origin[1], diagPt[1]);
  }
  if (dim > 2)
  {
    getDiagonalComponents(htgLoc->GetHTG()->GetZCoordinates(), origin[2], diagPt[2]);
  }
  vtkNew<vtkGenericCell> cell;
  int subId = 0;
  std::vector<double> intercept(3, 0.0);
  std::vector<double> pcoords(3, 0.0);
  vtkIdType cellId = -1;
  double t = -1;
  // first mask the cell where the first point is
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  vtkIdType maskedId = htgLoc->Search(origin.data(), cursor);
  cursor->SetMask(true);
  int resInter = htgLoc->IntersectWithLine(
    origin.data(), diagPt.data(), 0.0, t, intercept.data(), pcoords.data(), subId, cellId, cell);
  cursor->SetMask(false);
  if (resInter != 0)
  {
    success = true;
  }
  if (success)
  {
    success = (cellId >= 0);
  }
  if (success)
  {
    success = (cellId != maskedId);
  }
  theseResults->IntersectMaskedDiagonal = success;
  if (!success)
  {
    std::cout << "Intersect diagonal with masked point failed" << std::endl;
  }
  return success;
}

bool runAllIntersectsDiagonal(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* theseResults)
{
  bool success = false;
  std::vector<double> origin(3, 0.0);
  std::vector<double> diagPt(3, 0.0);
  unsigned int dim = htgLoc->GetHTG()->GetDimension();
  auto getDiagonalComponents = [](vtkDataArray* compArray, double& first, double& last) {
    first = compArray->GetComponent(0, 0);
    last = compArray->GetComponent(compArray->GetNumberOfTuples() - 1, 0);
  };
  getDiagonalComponents(htgLoc->GetHTG()->GetXCoordinates(), origin[0], diagPt[0]);
  if (dim > 1)
  {
    getDiagonalComponents(htgLoc->GetHTG()->GetYCoordinates(), origin[1], diagPt[1]);
  }
  if (dim > 2)
  {
    getDiagonalComponents(htgLoc->GetHTG()->GetZCoordinates(), origin[2], diagPt[2]);
  }
  vtkNew<vtkPoints> points;
  points->Initialize();
  vtkNew<vtkIdList> cellIds;
  cellIds->Initialize();
  vtkNew<vtkGenericCell> cell;
  int resInter =
    htgLoc->IntersectWithLine(origin.data(), diagPt.data(), 0.0, points, cellIds, cell);
  if (resInter != 0)
  {
    success = true;
  }
  if (success)
  {
    success = (points->GetNumberOfPoints() > 0);
  }
  if (success)
  {
    success = (cellIds->GetNumberOfIds() > 0);
  }
  if (success)
  {
    success = (points->GetNumberOfPoints() == cellIds->GetNumberOfIds());
  }
  if (success)
  {
    Eigen::Map<const Eigen::Vector3d> vOri(origin.data());
    Eigen::Map<const Eigen::Vector3d> vDiag(diagPt.data());
    vtkNew<vtkGenericCell> cell;
    std::vector<double> pcoords(3, 0.0);
    std::vector<double> weights(std::pow(2, dim), 0.0);
    int subId = 0;
    double dtot = (vDiag - vOri).norm();
    for (unsigned int iP = 0; iP < points->GetNumberOfPoints(); iP++)
    {
      Eigen::Map<const Eigen::Vector3d> pt(points->GetPoint(iP));
      double d = (pt - vOri).norm();
      d += (pt - vDiag).norm();
      success = success && std::abs(dtot - d) < 1e-6;
      if (!success)
      {
        break;
      }

      htgLoc->FindCell(pt.data(), 0.0, cell, subId, pcoords.data(), weights.data());
      double t = 0.0;
      std::vector<double> x(3, 0.0);
      success = success &&
        (cell->IntersectWithLine(
           origin.data(), diagPt.data(), 0.0, t, x.data(), pcoords.data(), subId) != 0);
      if (!success)
      {
        break;
      }
    }
  }
  theseResults->AllIntersectsDiagonal = success;
  if (!success)
  {
    std::cout << "Failed diagonal all intersections with line" << std::endl;
  }
  return success;
}

bool RunTests(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* thisResult)
{
  bool success = true;
  success = runOutsidePointSearch(htgLoc, thisResult) && success;
  success = runOuterEdgeSearch(htgLoc, thisResult) && success;
  for (auto it = thisResult->points.begin(); it != thisResult->points.end(); it++)
  {
    it->second = runPointSearch(htgLoc, it->first.data());
    success = success && it->second;
    it->second = runFindCell(htgLoc, it->first.data());
    success = success && it->second;
    it->second = runIntersectWithPoints(htgLoc, it->first.data());
    success = success && it->second;
  }
  success = runMaskedPointSearch(htgLoc, thisResult) && success;
  success = runAllMaskedPointSearch(htgLoc, thisResult) && success;
  success = runIntersectDiagonal(htgLoc, thisResult) && success;
  success = runIntersectWithMaskDiagonal(htgLoc, thisResult) && success;
  success = runAllIntersectsDiagonal(htgLoc, thisResult) && success;
  return success;
} // RunTests

}; // htggeomlocator

int TestHyperTreeGridGeometricLocator(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  using namespace htggeomlocator;
  bool success = true;
  vtkNew<vtkHyperTreeGridGeometricLocator> myLocator;

  // Setup HTG loop
  int nHTGs = 6;
  std::vector<vtkHyperTreeGridPreConfiguredSource::HTGType> myHTGTypes(nHTGs);
  myHTGTypes[0] = vtkHyperTreeGridPreConfiguredSource::UNBALANCED_3DEPTH_2BRANCH_2X3;
  myHTGTypes[1] = vtkHyperTreeGridPreConfiguredSource::BALANCED_3DEPTH_2BRANCH_2X3;
  myHTGTypes[2] = vtkHyperTreeGridPreConfiguredSource::UNBALANCED_2DEPTH_3BRANCH_3X3;
  myHTGTypes[3] = vtkHyperTreeGridPreConfiguredSource::BALANCED_4DEPTH_3BRANCH_2X2;
  myHTGTypes[4] = vtkHyperTreeGridPreConfiguredSource::UNBALANCED_3DEPTH_2BRANCH_3X2X3;
  myHTGTypes[5] = vtkHyperTreeGridPreConfiguredSource::BALANCED_2DEPTH_3BRANCH_3X3X2;

  std::vector<TestResults> myTestResults(nHTGs);

  std::vector<SearchPair> commonPoints;
  commonPoints.push_back(std::make_pair(std::vector<double>(3, 0.5), false));
  commonPoints.push_back(std::make_pair(std::vector<double>(3, 0.0), false));
  commonPoints.push_back(std::make_pair(std::vector<double>(3, -1.0), false));
  commonPoints.push_back(std::make_pair(std::vector<double>(3, 1.0 - 1e-10), false));
  {
    std::vector<double> randPt = { -0.2, 0.6, -0.7 };
    commonPoints.push_back(std::make_pair(randPt, false));
  }

  unsigned int iHTG = 0;
  for (auto it = myTestResults.begin(); it != myTestResults.end(); it++)
  {
    // common
    it->points.resize(commonPoints.size());
    std::copy(commonPoints.begin(), commonPoints.end(), it->points.begin());
    // particular
    iHTG++;
  }

  vtkNew<vtkHyperTreeGridPreConfiguredSource> myGenerator;

  // Loop over HTGs
  for (int iHTG = 0; iHTG < nHTGs; iHTG++)
  {
    std::cout << "iHTG: " << iHTG << "\n\n";
    // Generate HTG
    myGenerator->SetHTGMode(myHTGTypes[iHTG]);
    myGenerator->Update();
    vtkHyperTreeGrid* thisHTG = vtkHyperTreeGrid::SafeDownCast(myGenerator->GetOutput());

    vtkNew<vtkBitArray> thisMask;
    thisMask->SetNumberOfComponents(1);
    thisMask->SetNumberOfTuples(thisHTG->GetNumberOfCells());
    thisMask->Fill(false);
    thisHTG->SetMask(thisMask);

    myLocator->SetHTG(thisHTG);

    // Run tests on HTG
    success = RunTests(myLocator, &(myTestResults[iHTG])) && success;
    std::cout << "\n" << std::endl;
  }

  if (!success)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
} // TestHyperTreeGridGeometricLocator
