// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendFilter.h"
#include "vtkExtractGeometry.h"
#include "vtkForceStaticMesh.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSpatioTemporalHarmonicsSource.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <cstdlib>
#include <string>

namespace
{
//------------------------------------------------------------------------------
bool TestDataSet(int argc, char* argv[], bool unstructured, const std::string& baseline)
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkSpatioTemporalHarmonicsSource> harmonics;
  harmonics->SetWholeExtent(0, 10, 0, 10, 0, 10);
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(4.5, 5, 5);
  plane->SetNormal(1, 0, 0);

  vtkNew<vtkExtractGeometry> extractor;
  if (unstructured)
  {
    vtkNew<vtkAppendFilter> toUGConverter;
    toUGConverter->SetInputConnection(harmonics->GetOutputPort());
    extractor->SetInputConnection(toUGConverter->GetOutputPort(0));
  }
  else
  {
    extractor->SetInputConnection(harmonics->GetOutputPort());
  }

  extractor->SetImplicitFunction(plane);
  extractor->Update();
  vtkUnstructuredGrid* output = extractor->GetOutput();

  int expectedNbOfCells = 400;
  vtkLogIf(ERROR, output->GetNumberOfCells() != expectedNbOfCells,
    "ExtractInsideOff: Wrong number of output cells (has " << output->GetNumberOfCells() << ")");

  extractor->SetExtractInside(false);
  extractor->SetExtractOnlyBoundaryCells(false);
  extractor->SetExtractBoundaryCells(false);
  extractor->Update();
  expectedNbOfCells = 500;
  vtkLogIf(ERROR, output->GetNumberOfCells() != expectedNbOfCells,
    "ExtractInsideOn: Wrong number of output cells (has " << output->GetNumberOfCells() << ")");

  extractor->SetExtractBoundaryCells(true);
  extractor->Update();
  expectedNbOfCells = 600;
  vtkLogIf(ERROR, output->GetNumberOfCells() != expectedNbOfCells,
    "ExtractBoundaryCellsOn: Wrong number of output cells (has " << output->GetNumberOfCells()
                                                                 << ")");

  extractor->SetExtractInside(true);
  extractor->SetExtractOnlyBoundaryCells(true);
  extractor->Update();
  return vtkTestUtilities::RegressionTest(argc, argv, extractor->GetOutput(), baseline);
}

//------------------------------------------------------------------------------
bool TestComposite(int argc, char* argv[], const std::string& baseline)
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkRTAnalyticSource> wavelet1;
  wavelet1->SetWholeExtent(0, 10, 0, 5, 0, 10);
  vtkNew<vtkRTAnalyticSource> wavelet2;
  wavelet2->SetWholeExtent(0, 10, 5, 10, 0, 10);

  vtkNew<vtkGroupDataSetsFilter> groups;
  groups->AddInputConnection(wavelet1->GetOutputPort());
  groups->AddInputConnection(wavelet2->GetOutputPort());

  vtkNew<vtkPlane> plane;
  plane->SetOrigin(5, 5, 5);
  plane->SetNormal(1, 1, 0);

  vtkNew<vtkExtractGeometry> extractor;
  extractor->SetInputConnection(groups->GetOutputPort(0));
  extractor->SetImplicitFunction(plane);
  extractor->Update();

  extractor->SetExtractInside(true);
  extractor->SetExtractOnlyBoundaryCells(true);
  extractor->SetExtractBoundaryCells(true);
  extractor->Update();

  return vtkTestUtilities::RegressionTest(argc, argv, extractor->GetOutputDataObject(0), baseline);
}

//------------------------------------------------------------------------------
bool TestStaticMesh(int argc, char* argv[], const std::string& baseline)
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkSpatioTemporalHarmonicsSource> harmonics;
  harmonics->SetWholeExtent(0, 10, 0, 10, 0, 10);
  harmonics->ClearTimeStepValues();
  harmonics->AddTimeStepValue(0);
  harmonics->AddTimeStepValue(0.1);
  harmonics->AddTimeStepValue(0.2);
  harmonics->AddTimeStepValue(0.3);
  // static mesh optimization occurs mainly on unstructured data.
  // Convert the harmonics (vtkImageData) into a vtkUnstructuredGrid
  vtkNew<vtkAppendFilter> toUGConverter;
  toUGConverter->SetInputConnection(harmonics->GetOutputPort());
  vtkNew<vtkForceStaticMesh> staticMesh;
  staticMesh->SetInputConnection(toUGConverter->GetOutputPort());

  vtkNew<vtkPlane> plane;
  plane->SetOrigin(4.5, 1, 1);
  plane->SetNormal(1, 0, 0);

  vtkNew<vtkExtractGeometry> extractor;
  extractor->SetInputConnection(staticMesh->GetOutputPort());
  extractor->SetImplicitFunction(plane);
  extractor->SetExtractInside(true);
  extractor->SetExtractBoundaryCells(true);
  extractor->SetExtractOnlyBoundaryCells(true);
  extractor->UpdateTimeStep(0.2);

  vtkUnstructuredGrid* output = extractor->GetOutput();
  auto firstMeshTime = output->GetMeshMTime();
  extractor->UpdateTimeStep(0.1);
  vtkLogIf(ERROR, output->GetMeshMTime() != firstMeshTime,
    "Error, mesh MTime should not change with static mesh input.");

  extractor->UpdateTimeStep(0);
  return vtkTestUtilities::RegressionTest(argc, argv, extractor->GetOutputDataObject(0), baseline);
}
}

//------------------------------------------------------------------------------
int TestExtractGeometry(int argc, char* argv[])
{
  bool ret = ::TestDataSet(argc, argv, false, "/Data/vtkHDF/TestExtractGeometryBaseline.vtkhdf");
  ret = ::TestDataSet(argc, argv, true, "/Data/vtkHDF/TestExtractGeometryBaseline.vtkhdf") && ret;
  ret =
    ::TestComposite(argc, argv, "/Data/vtkHDF/TestExtractGeometryCompositeBaseline.vtkhdf") && ret;
  ret = ::TestStaticMesh(argc, argv, "/Data/vtkHDF/TestExtractGeometryBaseline.vtkhdf") && ret;

  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
