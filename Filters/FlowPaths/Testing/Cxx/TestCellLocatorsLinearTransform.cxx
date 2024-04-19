// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellLocator.h"
#include "vtkCellTreeLocator.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkDebugLeaks.h"
#include "vtkLinearTransformCellLocator.h"
#include "vtkModifiedBSPTree.h"
#include "vtkNew.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCellLocator.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"

#include <random>

void GenerateRandomPoints(vtkIdType npts, vtkPoints* points, double bound)
{
  points->SetNumberOfPoints(npts);
  std::uniform_real_distribution<double> dist(-bound, bound);
  std::default_random_engine randomEngine; // the number is a seed
  std::mt19937 gen(randomEngine());
  double point[3];
  for (vtkIdType pointId = 0; pointId < npts; ++pointId)
  {
    point[0] = dist(gen);
    point[1] = dist(gen);
    point[2] = dist(gen);
    points->SetPoint(pointId, point);
  }
}

bool TestCellLocators(vtkUnstructuredGrid* dataset, vtkPointSet* transformedDataset,
  vtkPoints* transformedRandomPoints, vtkAbstractCellLocator* locatorType,
  const double acceptableAccuracyPercentage)
{
  vtkAbstractCellLocator* locator = locatorType->NewInstance();
  // build locator with non-rotated dataset
  locator->CacheCellBoundsOn();
  locator->UseExistingSearchStructureOn();
  locator->SetDataSet(dataset);
  locator->BuildLocator();

  // create a shallowCopiedLocator
  vtkAbstractCellLocator* shallowCopiedLocator = locator->NewInstance();
  shallowCopiedLocator->SetDataSet(dataset);
  shallowCopiedLocator->ShallowCopy(locator);
  // free locator's structure to ensure correctness of the shallow copy
  locator->Delete();
  const auto before = shallowCopiedLocator->GetBuildTime();
  shallowCopiedLocator->BuildLocator();
  const auto after = shallowCopiedLocator->GetBuildTime();
  if (before != after)
  {
    std::cout << "Build time should not change after Build Locator" << std::endl;
    return false;
  }

  // create a vtkLinearTransformCellLocator with shallowCopiedLocator
  vtkNew<vtkLinearTransformCellLocator> linearTransformLocator;
  linearTransformLocator->SetCellLocator(shallowCopiedLocator);
  linearTransformLocator->SetDataSet(transformedDataset);
  linearTransformLocator->BuildLocator();

  const vtkIdType numberOfRandomPointsPoints = transformedRandomPoints->GetNumberOfPoints();

  // find the cells for the transformed random points using the original locator
  std::vector<vtkIdType> cellIds(static_cast<size_t>(numberOfRandomPointsPoints), -1);
  double transformedPoint[3];
  for (vtkIdType i = 0; i < numberOfRandomPointsPoints; ++i)
  {
    transformedRandomPoints->GetPoint(i, transformedPoint);
    cellIds[i] = linearTransformLocator->FindCell(transformedPoint);
  }

  // create a locator with the transformed dataset
  vtkAbstractCellLocator* locator2 = shallowCopiedLocator->NewInstance();
  locator2->SetDataSet(transformedDataset);
  locator2->BuildLocator();

  // find the cells for the transformed random points using the new locator
  vtkIdType cellId;
  vtkIdType cellIdsMatchedCounter = 0;
  for (vtkIdType i = 0; i < numberOfRandomPointsPoints; ++i)
  {
    transformedRandomPoints->GetPoint(i, transformedPoint);
    cellId = locator2->FindCell(transformedPoint);
    if (cellIds[i] == cellId)
    {
      ++cellIdsMatchedCounter;
    }
  }
  shallowCopiedLocator->Delete();
  locator2->Delete();

  double matchAccuracyPercentage = 100 * static_cast<double>(cellIdsMatchedCounter) /
    static_cast<double>(numberOfRandomPointsPoints);
  std::cout << locatorType->GetClassName() << ": Match accuracy: " << matchAccuracyPercentage
            << "%, Acceptable accuracy: " << acceptableAccuracyPercentage << "%" << std::endl;

  return matchAccuracyPercentage >= acceptableAccuracyPercentage;
}

int TestCellLocatorsLinearTransform(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  const double bound = 10;
  const vtkIdType numberOfRandomPointsPoints = 100000;
  // Generally the accuracy is around 99% except bsp tree
  const double acceptableAccuracyPercentage = 90;

  // create a dataset
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-bound, bound, -bound, bound, -bound, bound);
  wavelet->SetCenter(0.0, 0.0, 0.0);
  vtkNew<vtkDataSetTriangleFilter> triangleFilter;
  triangleFilter->SetInputConnection(wavelet->GetOutputPort());
  triangleFilter->Update();
  auto dataset = triangleFilter->GetOutput();

  // create random points
  vtkNew<vtkPoints> randomPoints;
  GenerateRandomPoints(numberOfRandomPointsPoints, randomPoints, bound);

  // create a transform
  vtkNew<vtkTransform> transform;
  transform->RotateX(30);
  transform->RotateZ(45);
  transform->Translate(5, 5, 5);

  // transform the points
  vtkNew<vtkPoints> transformedRandomPoints;
  transform->TransformPoints(randomPoints, transformedRandomPoints);

  // transform the dataset
  vtkNew<vtkTransformFilter> transformFilterDataset;
  transformFilterDataset->SetInputData(dataset);
  transformFilterDataset->SetTransform(transform);
  transformFilterDataset->Update();
  auto transformedDataset = transformFilterDataset->GetOutput();

  // test locators' accuracy using SupportLinearTransformation
  bool testPassed = true;
  vtkNew<vtkCellLocator> cl;
  testPassed &= TestCellLocators(
    dataset, transformedDataset, transformedRandomPoints, cl, acceptableAccuracyPercentage);
  vtkNew<vtkStaticCellLocator> scl;
  testPassed &= TestCellLocators(
    dataset, transformedDataset, transformedRandomPoints, scl, acceptableAccuracyPercentage);
  vtkNew<vtkCellTreeLocator> ctl;
  testPassed &= TestCellLocators(
    dataset, transformedDataset, transformedRandomPoints, ctl, acceptableAccuracyPercentage);
  vtkNew<vtkModifiedBSPTree> bsp;
  testPassed &= TestCellLocators(
    dataset, transformedDataset, transformedRandomPoints, bsp, acceptableAccuracyPercentage);
  return testPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}
