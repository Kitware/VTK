// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkCylinderSource.h"
#include "vtkDataArray.h"
#include "vtkGenerateRegionIds.h"
#include "vtkLogger.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkSuperquadricSource.h"

namespace utils
{
vtkDataArray* GetOutputArray(vtkGenerateRegionIds* filter, const std::string& arrayName)
{
  vtkPolyData* out = filter->GetOutput();
  vtkCellData* outCellData = out->GetCellData();
  vtkDataArray* generatedArray = outCellData->GetArray(arrayName.c_str());

  if (!generatedArray)
  {
    vtkLog(ERROR, "No array found with name <" << arrayName << ">");
  }

  return generatedArray;
}

bool TestRange(vtkDataArray* generatedArray, const int expectedRange[2])
{
  double idsRange[2] = { 0, 0 };
  generatedArray->GetRange(idsRange);
  if (idsRange[0] != expectedRange[0])
  {
    vtkLog(
      ERROR, "Regions ids should start at" << expectedRange[0] << " but starts at " << idsRange[0]);
    return false;
  }

  if (idsRange[1] != expectedRange[1])
  {
    vtkLog(
      ERROR, "Regions ids should end at" << expectedRange[1] << " but ends at " << idsRange[1]);
    return false;
  }

  return true;
}
}

namespace tests
{
//-----------------------------------------------------------------------------
bool CheckAngle(double maxAngle, int expectedNumberOfRegions)
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkGenerateRegionIds> generateRegionIds;
  generateRegionIds->SetMaxAngle(maxAngle);
  generateRegionIds->SetInputConnection(sphere->GetOutputPort());
  generateRegionIds->Update();

  vtkDataArray* generatedArray =
    utils::GetOutputArray(generateRegionIds, generateRegionIds->GetRegionIdsArrayName());

  if (!generatedArray)
  {
    vtkLogScopeFunction(ERROR);
    return false;
  }

  int expectedRange[2] = { 0, expectedNumberOfRegions - 1 };
  return utils::TestRange(generatedArray, expectedRange);
}

//-----------------------------------------------------------------------------
bool CheckArrayName()
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkGenerateRegionIds> generateRegionIds;
  generateRegionIds->SetInputConnection(sphere->GetOutputPort());

  std::string newName = "testing_region_ids";
  generateRegionIds->SetRegionIdsArrayName(newName);
  generateRegionIds->Update();

  vtkDataArray* generatedArray = utils::GetOutputArray(generateRegionIds, newName);

  return generatedArray != nullptr;
}

//-----------------------------------------------------------------------------
bool CheckDefaults()
{
  const std::string expectedName = "vtkRegionIds";
  const int expectedNumberOfRegions = 1;

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkGenerateRegionIds> generateRegionIds;
  generateRegionIds->SetInputConnection(sphere->GetOutputPort());
  generateRegionIds->Update();

  vtkDataArray* generatedArray = utils::GetOutputArray(generateRegionIds, expectedName);
  if (!generatedArray)
  {
    vtkLogScopeFunction(ERROR);
    return false;
  }

  int expectedRange[2] = { 0, expectedNumberOfRegions - 1 };
  return utils::TestRange(generatedArray, expectedRange);
}

//-----------------------------------------------------------------------------
bool CheckCellTypes()
{
  // cylinder has quad and polygon
  vtkNew<vtkCylinderSource> sphere;
  vtkNew<vtkGenerateRegionIds> generateRegionIds;
  generateRegionIds->SetInputConnection(sphere->GetOutputPort());
  generateRegionIds->Update();

  vtkDataArray* generatedArray =
    utils::GetOutputArray(generateRegionIds, generateRegionIds->GetRegionIdsArrayName());
  if (!generatedArray)
  {
    return false;
  }

  int expectedRange[2] = { 0, 7 };
  if (!utils::TestRange(generatedArray, expectedRange))
  {
    return false;
  }

  // has triangle strips
  vtkNew<vtkSuperquadricSource> superquadric;
  superquadric->ToroidalOn();
  generateRegionIds->SetInputConnection(superquadric->GetOutputPort());
  generateRegionIds->Update();

  generatedArray =
    utils::GetOutputArray(generateRegionIds, generateRegionIds->GetRegionIdsArrayName());
  if (!generatedArray)
  {
    return false;
  }

  expectedRange[1] = 31;
  return utils::TestRange(generatedArray, expectedRange);
}

}

//-----------------------------------------------------------------------------
int TestGenerateRegionIds(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  if (!tests::CheckDefaults())
  {
    vtkLog(ERROR, "CheckDefaults failed");
  }
  else if (!tests::CheckArrayName())
  {
    vtkLog(ERROR, "CheckArrayName failed");
  }
  else if (!tests::CheckAngle(30, 1) || !tests::CheckAngle(26, 10) || !tests::CheckAngle(0, 80) ||
    !tests::CheckAngle(1000, 1))
  {
    vtkLog(ERROR, "CheckAngle failed");
  }
  else if (!tests::CheckCellTypes())
  {
    vtkLog(ERROR, "CheckCellTypes failed");
  }
  else
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
