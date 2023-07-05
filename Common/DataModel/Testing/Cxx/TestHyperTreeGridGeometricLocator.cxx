// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

#include "vtkBitArray.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometricLocator.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPoints.h"

namespace testhtggeomlocator
{

double epsilon = 1e-6;
using SearchPair = std::pair<std::vector<double>, bool>;

struct TestResults
{
  bool OutsidePointSearch = false;
  bool OuterEdgeSearch = false;
  bool MaskedSearch = false;
  bool IntersectDiagonal = false;
  bool IntersectMaskedDiagonal = false;
  bool AllIntersectsDiagonal = false;
  std::vector<SearchPair> points;
};

bool runOutsidePointSearch(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* theseResults)
{
  double inf = std::numeric_limits<double>::infinity();
  double pt[3] = { inf, inf, inf };
  vtkIdType globId = htgLoc->Search(pt);
  bool success = (globId < 0);
  theseResults->OutsidePointSearch = success;
  if (!success)
  {
    std::cout << "Outside Point Search failed, found global ID " << globId << "\n";
  }
  return success;
}

bool runOuterEdgeSearch(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* theseResults)
{
  auto getLastComponent = [](vtkDataArray* thisArray) {
    return thisArray->GetComponent(thisArray->GetNumberOfTuples() - 1, 0);
  };
  double pt[3]{ getLastComponent(htgLoc->GetHTG()->GetXCoordinates()),
    getLastComponent(htgLoc->GetHTG()->GetYCoordinates()),
    getLastComponent(htgLoc->GetHTG()->GetZCoordinates()) };
  // do edge point
  vtkIdType globId = htgLoc->Search(pt);
  bool success = (globId < 0);
  theseResults->OuterEdgeSearch = success;
  if (!success)
  {
    std::cout << "Outer Edge Search failed, found global ID " << globId << "\n";
  }
  return success;
}

bool runMaskedPointSearch(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* theseResults)
{
  double pt[3] = { 0, 0, 0 };
  pt[0] = htgLoc->GetHTG()->GetXCoordinates()->GetComponent(0, 0) + epsilon;
  if (htgLoc->GetHTG()->GetDimension() > 1)
  {
    pt[1] = htgLoc->GetHTG()->GetYCoordinates()->GetComponent(0, 0) + epsilon;
  }
  if (htgLoc->GetHTG()->GetDimension() > 2)
  {
    pt[2] = htgLoc->GetHTG()->GetZCoordinates()->GetComponent(0, 0) + epsilon;
  }
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursorFirst;
  vtkIdType globId = htgLoc->Search(pt, cursorFirst);
  bool success = (globId >= 0);
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

bool runAllMaskedPointSearch(
  vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* vtkNotUsed(theseResults))
{
  double pt[3] = { htgLoc->GetHTG()->GetXCoordinates()->GetComponent(0, 0) + epsilon,
    htgLoc->GetHTG()->GetYCoordinates()->GetComponent(0, 0) + epsilon,
    htgLoc->GetHTG()->GetZCoordinates()->GetComponent(0, 0) };
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursorFirst;
  bool success = (htgLoc->Search(pt, cursorFirst) > 0);
  cursorFirst->ToParent();
  vtkIdType globIdFirst = cursorFirst->GetGlobalNodeIndex();
  success = (globIdFirst > 0);
  vtkIdType globIdSecond = 0;
  if (success)
  {
    auto setChildrenMask = [&htgLoc, &cursorFirst](bool state) {
      for (unsigned int d = 0; d < htgLoc->GetHTG()->GetNumberOfChildren(); d++)
      {
        cursorFirst->ToChild(d);
        cursorFirst->SetMask(state);
        cursorFirst->ToParent();
      }
    };
    setChildrenMask(true);
    vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursorSecond;
    globIdSecond = htgLoc->Search(pt, cursorSecond);
    success = (globIdFirst == globIdSecond);
    setChildrenMask(false);
  }
  if (!success)
  {
    std::cout << "All masked point search failed, parent of first found was " << globIdFirst
              << " while the result of the second search was " << globIdSecond << std::endl;
  }
  return success;
}

bool runPointSearch(vtkHyperTreeGridGeometricLocator* htgLoc, const double pt[3])
{
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  vtkIdType globId = htgLoc->Search(pt, cursor);
  bool success = (globId >= 0);
  if (success)
  {
    const double* origin = cursor->GetOrigin();
    const double* size = cursor->GetSize();
    for (unsigned int d = 0; d < 3; d++)
    {
      double buff = pt[d] - origin[d];
      success = (size[d] == 0.0) || ((buff < size[d]) && (buff >= 0.0));
      if (!success)
      {
        break;
      }
    }
  }
  if (!success)
  {
    std::cout << "Point search failed for point: \n";
    for (unsigned int d = 0; d < 3; d++)
    {
      std::cout << pt[d] << ",";
    }
    std::cout << std::endl;
  }

  return success;
}

bool runFindCell(vtkHyperTreeGridGeometricLocator* htgLoc, const double pt[3])
{
  vtkNew<vtkGenericCell> cell;
  int subId = 0;
  std::vector<double> pcoords(3, 0.0);
  std::vector<double> weights(
    static_cast<size_t>(std::pow(2, htgLoc->GetHTG()->GetDimension())), 0.0);
  vtkIdType globId = htgLoc->FindCell(pt, 0.0, cell, subId, pcoords.data(), weights.data());
  bool success = (globId >= 0);
  success &= (1 - std::accumulate(weights.begin(), weights.end(), 0.0) < epsilon);
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
  bool success = (htgLoc->IntersectWithLine(origin.data(), diagPt.data(), epsilon, t,
                    intercept.data(), pcoords.data(), subId, cellId, cell) != 0);
  success &= (cellId >= 0);
  success &= (t < epsilon);
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
  bool success = (htgLoc->IntersectWithLine(copyPt, ref, epsilon, t, intercept.data(),
                    pcoords.data(), subId, cellId, cell) != 0);
  success &= (cellId >= 0);
  success &= (htgLoc->Search(pt) == cellId);
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
  bool success = (htgLoc->IntersectWithLine(origin.data(), diagPt.data(), epsilon, t,
                    intercept.data(), pcoords.data(), subId, cellId, cell) != 0);
  cursor->SetMask(false);
  success &= (cellId >= 0);
  success &= (cellId != maskedId);
  theseResults->IntersectMaskedDiagonal = success;
  if (!success)
  {
    std::cout << "Intersect diagonal with masked point failed" << std::endl;
  }
  return success;
}

bool runAllIntersectsDiagonal(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* theseResults)
{
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
  bool success =
    (htgLoc->IntersectWithLine(origin.data(), diagPt.data(), epsilon, points, cellIds, cell) != 0);
  success &= (points->GetNumberOfPoints() > 0);
  success &= (cellIds->GetNumberOfIds() > 0);
  success &= (points->GetNumberOfPoints() == cellIds->GetNumberOfIds());
  if (success)
  {
    std::vector<double> pcoords(3, 0.0);
    std::vector<double> weights(std::pow(2, dim), 0.0);
    int subId = 0;
    std::vector<double> buffer(3, 0.0);
    vtkMath::Subtract(diagPt.data(), origin.data(), buffer.data());
    double dtot = vtkMath::Norm(buffer.data());
    for (unsigned int iP = 0; iP < points->GetNumberOfPoints(); iP++)
    {
      vtkMath::Subtract(points->GetPoint(iP), origin.data(), buffer.data());
      double d = vtkMath::Norm(buffer.data());
      vtkMath::Subtract(points->GetPoint(iP), diagPt.data(), buffer.data());
      d += vtkMath::Norm(buffer.data());
      success &= std::abs(dtot - d) < epsilon;
      if (!success)
      {
        break;
      }

      htgLoc->FindCell(points->GetPoint(iP), 0.0, cell, subId, pcoords.data(), weights.data());
      double t = 0.0;
      std::vector<double> x(3, 0.0);
      success &= (cell->IntersectWithLine(origin.data(), diagPt.data(), epsilon, t, x.data(),
                    pcoords.data(), subId) != 0);
      if (!success)
      {
        cell->PrintSelf(std::cout, vtkIndent());
        std::cout << points->GetPoint(iP)[0] << ", " << points->GetPoint(iP)[1] << ", "
                  << points->GetPoint(iP)[2] << std::endl;
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
  bool success = runOutsidePointSearch(htgLoc, thisResult);
  success = runOuterEdgeSearch(htgLoc, thisResult) && success;
  for (auto it = thisResult->points.begin(); it != thisResult->points.end(); it++)
  {
    it->second = runPointSearch(htgLoc, it->first.data());
    success &= it->second;
    it->second = runFindCell(htgLoc, it->first.data());
    success &= it->second;
    it->second = runIntersectWithPoints(htgLoc, it->first.data());
    success &= it->second;
  }
  success = runMaskedPointSearch(htgLoc, thisResult) && success;
  success = runAllMaskedPointSearch(htgLoc, thisResult) && success;
  success = runIntersectDiagonal(htgLoc, thisResult) && success;
  success = runIntersectWithMaskDiagonal(htgLoc, thisResult) && success;
  success = runAllIntersectsDiagonal(htgLoc, thisResult) && success;
  return success;
}

bool TestLocatorTolerance()
{
  vtkNew<vtkHyperTreeGridPreConfiguredSource> htgSource;
  htgSource->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::CUSTOM);
  htgSource->SetCustomArchitecture(vtkHyperTreeGridPreConfiguredSource::UNBALANCED);
  htgSource->SetCustomDim(2);
  htgSource->SetCustomFactor(2);
  htgSource->SetCustomDepth(3);
  htgSource->SetCustomExtent(0.0, 1.0, 0.0, 1.0, 1.0, 1.0);
  htgSource->SetCustomSubdivisions(3, 3, 0);
  htgSource->Update();
  vtkHyperTreeGrid* htg = htgSource->GetHyperTreeGridOutput();

  vtkNew<vtkHyperTreeGridGeometricLocator> locator;
  locator->SetHTG(htg);

  bool success = true;

  // Testing vtkHyperTreeGridGeometricLocator::Search
  auto TestSearchPoint = [&locator](std::array<double, 3> point, vtkIdType expected) {
    bool pointSuccess = true;
    vtkIdType cellId = locator->Search(point.data());
    if (cellId != expected)
    {
      std::cerr << "ERROR: point {" << point[0] << "," << point[1] << "," << point[2]
                << "} gave the wrong cell, expected " << expected << " but got " << cellId
                << std::endl;
      pointSuccess = false;
    }
    return pointSuccess;
  };
  constexpr double tol = 0.001;
  locator->SetTolerance(tol);
  success = TestSearchPoint({ 0.5, 0.5, 0.0 }, 15) && success;
  success = TestSearchPoint({ 0.0, 0.0, 0.0 }, 9) && success;
  success = TestSearchPoint({ 0.05, 0.05, 0.0005 }, 9) && success;
  success = TestSearchPoint({ 1.0, 0.0, 0.0 }, 13) && success;
  success = TestSearchPoint({ 0.0, 1.0, 0.0 }, 14) && success;
  success = TestSearchPoint({ 1.0, 1.0, 0.0 }, 15) && success;
  success = TestSearchPoint({ 1.0 + 0.5 * tol, 1.0, 0.0 }, 15) && success;
  success = TestSearchPoint({ 1.0 + 2.0 * tol, 1.0, 0.0 }, -1) && success;
  locator->SetTolerance(0.0);
  success = TestSearchPoint({ 0.0, 0.0, 0.0 }, 9) && success;
  success = TestSearchPoint({ 1.0, 1.0, 0.0 }, -1) && success;
  success = TestSearchPoint({ 1.0 + 0.5 * tol, 1.0, 0.0 }, -1) && success;

  // Testing vtkHyperTreeGridGeometricLocator::FindCell
  std::array<double, 3> point = { 1.0 + 0.5 * tol, 1.0, 0.0 };
  vtkNew<vtkGenericCell> cell;
  int subId;
  std::array<double, 3> pcoords;
  std::array<double, 4> weights;
  vtkIdType cellId =
    locator->FindCell(point.data(), tol, cell, subId, pcoords.data(), weights.data());
  if (cellId != 15)
  {
    std::cerr << "ERROR: vtkHyperTreeGridGeometricLocator::FindCell gave the wrong cell, expected "
                 "15 but got "
              << cellId << std::endl;
    success = false;
  }
  std::array<double, 6> bounds;
  cell->GetBounds(bounds.data());
  if (bounds != std::array<double, 6>{ 0.5, 1.0, 0.5, 1.0, 0.0, 0.0 })
  {
    std::cerr << "ERROR: vtkHyperTreeGridGeometricLocator::FindCell gave wrong cell, bounds are "
                 "not coherent"
              << std::endl;
    success = false;
  }

  if (locator->FindCell(point.data(), 0.0, cell, subId, pcoords.data(), weights.data()) >= 0)
  {
    std::cerr
      << "ERROR: vtkHyperTreeGridGeometricLocator::FindCell found a cell when it shouldn't have."
      << std::endl;
    success = false;
  }

  return success;
}

};

int TestHyperTreeGridGeometricLocator(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  using namespace testhtggeomlocator;
  vtkNew<vtkHyperTreeGridGeometricLocator> myLocator;

  // Setup HTG loop
  unsigned int nHTGs = 6;
  std::vector<vtkHyperTreeGridPreConfiguredSource::HTGType> myHTGTypes(nHTGs);
  myHTGTypes[0] = vtkHyperTreeGridPreConfiguredSource::UNBALANCED_3DEPTH_2BRANCH_2X3;
  myHTGTypes[1] = vtkHyperTreeGridPreConfiguredSource::BALANCED_3DEPTH_2BRANCH_2X3;
  myHTGTypes[2] = vtkHyperTreeGridPreConfiguredSource::UNBALANCED_2DEPTH_3BRANCH_3X3;
  myHTGTypes[3] = vtkHyperTreeGridPreConfiguredSource::BALANCED_4DEPTH_3BRANCH_2X2;
  myHTGTypes[4] = vtkHyperTreeGridPreConfiguredSource::UNBALANCED_3DEPTH_2BRANCH_3X2X3;
  myHTGTypes[5] = vtkHyperTreeGridPreConfiguredSource::BALANCED_2DEPTH_3BRANCH_3X3X2;

  std::vector<TestResults> myTestResults(nHTGs);

  std::vector<SearchPair> commonPoints;
  commonPoints.emplace_back(std::vector<double>{ 0.5, 0.5, 0.0 }, false);
  commonPoints.emplace_back(std::vector<double>(3, 0.0), false);
  commonPoints.emplace_back(std::vector<double>{ -1.0 + epsilon, -1.0 + epsilon, 0.0 }, false);
  commonPoints.emplace_back(std::vector<double>{ 1.0 - epsilon, 1.0 - epsilon, 0.0 }, false);
  commonPoints.emplace_back(std::vector<double>{ -0.2, 0.6, 0.0 }, false);

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
  bool success = true;
  for (iHTG = 0; iHTG < nHTGs; iHTG++)
  {
    std::cout << "iHTG: " << iHTG << "\n" << std::endl;
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

  success = success && TestLocatorTolerance();

  if (!success)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
