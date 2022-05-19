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

#include <limits>

#include "vtkNew.h"

#include "vtkBitArray.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometricLocator.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"

namespace htggeomlocator
{
typedef std::pair<std::vector<double>, bool> SearchPair;

struct TestResults
{
  bool OutsidePointSearch = false;
  bool OuterEdgeSearch = false;
  bool MaskedSearch = false;
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
  bool success = false;
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  vtkIdType globId = htgLoc->Search(pt, cursor);
  if (globId == -1)
  {
    return false;
  }
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

bool RunTests(vtkHyperTreeGridGeometricLocator* htgLoc, TestResults* thisResult)
{
  bool success = true;
  success = runOutsidePointSearch(htgLoc, thisResult) && success;
  success = runOuterEdgeSearch(htgLoc, thisResult) && success;
  for (auto it = thisResult->points.begin(); it != thisResult->points.end(); it++)
  {
    it->second = runPointSearch(htgLoc, it->first.data());
    success = success && it->second;
  }
  success = runMaskedPointSearch(htgLoc, thisResult) && success;
  success = runAllMaskedPointSearch(htgLoc, thisResult) && success;
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
