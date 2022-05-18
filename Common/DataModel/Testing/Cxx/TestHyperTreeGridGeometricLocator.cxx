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

#include "vtkDoubleArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometricLocator.h"
#include "vtkTestHTGGenerator.h"

namespace htggeomlocator
{
typedef std::pair<vtkIdType, bool> SearchIdPair;
typedef std::tuple<double*, vtkIdType, bool> SearchIdTriplet;

struct TestResults
{
  bool OutsidePointSearch = false;
  SearchIdPair HalfPointSearch = { 0, 0 };
  SearchIdPair ZeroPointSearch = { 0, 0 };
  SearchIdPair MOnePointSearch = { 0, 0 };
  SearchIdPair POnePointSearch = { 0, 0 };
  SearchIdPair EpsilonPointSearch = { 0, 0 };
  std::vector<SearchIdTriplet> SpecialPoints;
}; // TestResult

bool runOutsidePointSearch(vtkHyperTreeGridGeometricLocator* HTGLoc, TestResults* theseResults)
{
  bool success = 0;
  double inf = std::numeric_limits<double>::infinity();
  double pt[3] = { inf, inf, inf };
  vtkIdType globId = HTGLoc->Search(pt);
  success = (globId < 0);
  theseResults->OutsidePointSearch = success;
  if (!success)
  {
    std::cout << "Outside Point Search failed, found global ID " << globId << "\n";
  }
  return success;
} // runOutsidePointSearch

bool runHalfPointSearch(vtkHyperTreeGridGeometricLocator* HTGLoc, TestResults* theseResults)
{
  bool success = 0;
  double half = 0.5;
  double pt[3] = { half, half, half };
  vtkIdType globId = HTGLoc->Search(pt);
  success = (theseResults->HalfPointSearch.first == globId);
  theseResults->HalfPointSearch.second = success;
  if (!success)
  {
    std::cout << "Half Point Search failed, found global ID " << globId
              << " when should have found " << theseResults->HalfPointSearch.first << "\n";
  }
  return success;
} // runHalfPointSearch

bool runZeroPointSearch(vtkHyperTreeGridGeometricLocator* HTGLoc, TestResults* theseResults)
{
  bool success = 0;
  double pt[3] = { 0.0, 0.0, 0.0 };
  vtkIdType globId = HTGLoc->Search(pt);
  success = (theseResults->ZeroPointSearch.first == globId);
  theseResults->ZeroPointSearch.second = success;
  if (!success)
  {
    std::cout << "Zero Point Search failed, found global ID " << globId
              << " when should have found " << theseResults->ZeroPointSearch.first << "\n";
  }
  return success;
}

bool runMOnePointSearch(vtkHyperTreeGridGeometricLocator* HTGLoc, TestResults* theseResults)
{
  bool success = 0;
  double pt[3] = { -1.0, -1.0, -1.0 };
  vtkIdType globId = HTGLoc->Search(pt);
  success = (theseResults->MOnePointSearch.first == globId);
  theseResults->MOnePointSearch.second = success;
  if (!success)
  {
    std::cout << "MOne Point Search failed, found global ID " << globId
              << " when should have found " << theseResults->MOnePointSearch.first << "\n";
  }
  return success;
}

bool runPOnePointSearch(vtkHyperTreeGridGeometricLocator* HTGLoc, TestResults* theseResults)
{
  bool success = 0;
  double pt[3] = { -1.0, -1.0, -1.0 };
  vtkIdType globId = HTGLoc->Search(pt);
  success = (theseResults->POnePointSearch.first == globId);
  theseResults->POnePointSearch.second = success;
  if (!success)
  {
    std::cout << "POne Point Search failed, found global ID " << globId
              << " when should have found " << theseResults->POnePointSearch.first << "\n";
  }
  return success;
}

bool runEpsilonPointSearch(vtkHyperTreeGridGeometricLocator* HTGLoc, TestResults* theseResults)
{
  bool success = 0;
  double epsilon;
  double x = HTGLoc->GetHTG()->GetXCoordinates()->GetComponent(0, 0);
  double y = HTGLoc->GetHTG()->GetYCoordinates()->GetComponent(0, 0);
  double z = HTGLoc->GetHTG()->GetZCoordinates()->GetComponent(0, 0);
  double pt[3] = { x + epsilon, y + epsilon, z + epsilon };
  vtkIdType globId = HTGLoc->Search(pt);
  success = (theseResults->EpsilonPointSearch.first == globId);
  theseResults->EpsilonPointSearch.second = success;
  if (!success)
  {
    std::cout << "Epsilon Point Search failed, found global ID " << globId
              << " when should have found " << theseResults->EpsilonPointSearch.first << "\n";
  }
  return success;
}

bool runSpecialPointsSearch(vtkHyperTreeGridGeometricLocator* HTGLoc, TestResults* theseResults)
{
  bool success = true;
  bool locSuccess = false;
  int iPt = 0;
  for (auto it = theseResults->SpecialPoints.begin(); it != theseResults->SpecialPoints.end(); it++)
  {
    vtkIdType globId = HTGLoc->Search(std::get<0>(*it));
    locSuccess = (std::get<1>(*it) == globId);
    std::get<2>(*it) = locSuccess;
    if (!locSuccess)
    {
      std::cout << "Special Points Search failed on " << iPt << "th point, found global ID "
                << globId << " when should have found " << std::get<1>(*it) << "\n";
    }
    success = success && locSuccess;
    iPt++;
  }
  return success;
}

bool RunTests(vtkHyperTreeGridGeometricLocator* HTGLoc, TestResults* thisResult)
{
  bool success = true;
  success = runOutsidePointSearch(HTGLoc, thisResult) && success;
  success = runHalfPointSearch(HTGLoc, thisResult) && success;
  success = runZeroPointSearch(HTGLoc, thisResult) && success;
  success = runMOnePointSearch(HTGLoc, thisResult) && success;
  success = runPOnePointSearch(HTGLoc, thisResult) && success;
  success = runEpsilonPointSearch(HTGLoc, thisResult) && success;
  success = runSpecialPointsSearch(HTGLoc, thisResult) && success;
  return success;
} // TestResult

}; // htggeomlocator

int TestHyperTreeGridGeometricLocator(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  using namespace htggeomlocator;
  bool success = true;
  vtkNew<vtkHyperTreeGridGeometricLocator> myLocator;

  // Setup HTG loop
  int nHTGs = 6;
  std::vector<vtkTestHTGGenerator::HTGType> myHTGTypes(nHTGs);
  myHTGTypes[0] = vtkTestHTGGenerator::UNBALANCED_3DEPTH_2BRANCH_2X3;
  myHTGTypes[1] = vtkTestHTGGenerator::BALANCED_3DEPTH_2BRANCH_2X3;
  myHTGTypes[2] = vtkTestHTGGenerator::UNBALANCED_2DEPTH_3BRANCH_3X3;
  myHTGTypes[3] = vtkTestHTGGenerator::BALANCED_4DEPTH_3BRANCH_2X2;
  myHTGTypes[4] = vtkTestHTGGenerator::UNBALANCED_3DEPTH_2BRANCH_3X2X3;
  myHTGTypes[5] = vtkTestHTGGenerator::BALANCED_2DEPTH_3BRANCH_3X3X2;

  std::vector<TestResults> myTestResults(nHTGs);

  vtkNew<vtkTestHTGGenerator> myGenerator;

  // Loop over HTGs
  for (int iHTG = 0; iHTG < nHTGs; iHTG++)
  {
    std::cout << "iHTG: " << iHTG << "\n\n";
    // Generate HTG
    myGenerator->SetHTGMode(myHTGTypes[iHTG]);
    myGenerator->Update();
    vtkHyperTreeGrid* thisHTG = vtkHyperTreeGrid::SafeDownCast(myGenerator->GetOutput());

    myLocator->SetHTG(thisHTG);

    // Run tests on HTG
    success = RunTests(myLocator, &(myTestResults[iHTG])) && success;
    std::cout << "\n";
  }

  if (!success)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
} // TestHyperTreeGridGeometricLocator
