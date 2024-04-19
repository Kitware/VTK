// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkContourFilter.h>
#include <vtkEndFor.h>
#include <vtkForEach.h>
#include <vtkImageDataToPointSet.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPlaneCutter.h>
#include <vtkRandomAttributeGenerator.h>
#include <vtkSpatioTemporalHarmonicsSource.h>

#include <cstdlib>

namespace
{

constexpr std::size_t NB_SOURCE_TIME_STEPS = 20;

bool TestNoPipeline()
{
  vtkNew<vtkSpatioTemporalHarmonicsSource> source;

  vtkNew<vtkForEach> forEach;
  forEach->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkEndFor> endFor;
  endFor->SetInputConnection(forEach->GetOutputPort());

  endFor->Update();

  auto output = endFor->GetOutput();
  auto pdsc = vtkPartitionedDataSetCollection::SafeDownCast(output);
  if (!pdsc)
  {
    std::cerr << "Output was not partitioned data set collection" << std::endl;
    return false;
  }

  if (pdsc->GetNumberOfPartitionedDataSets() != ::NB_SOURCE_TIME_STEPS)
  {
    std::cerr << "Output did not have correct number of blocks" << std::endl;
    return false;
  }
  return true;
}

bool TestSimplePipeline()
{
  vtkNew<vtkSpatioTemporalHarmonicsSource> source;

  vtkNew<vtkForEach> forEach;
  forEach->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkImageDataToPointSet> toPointSet;
  toPointSet->SetInputConnection(forEach->GetOutputPort());

  vtkNew<vtkEndFor> endFor;
  endFor->SetInputConnection(toPointSet->GetOutputPort());

  endFor->Update();

  auto output = endFor->GetOutput();
  auto pdsc = vtkPartitionedDataSetCollection::SafeDownCast(output);
  if (!pdsc)
  {
    std::cerr << "Output was not partitioned data set collection" << std::endl;
    return false;
  }

  if (pdsc->GetNumberOfPartitionedDataSets() != ::NB_SOURCE_TIME_STEPS)
  {
    std::cerr << "Output did not have correct number of blocks" << std::endl;
    return false;
  }
  return true;
}

bool TestComplexPipeline()
{
  vtkNew<vtkSpatioTemporalHarmonicsSource> source;

  vtkNew<vtkForEach> forEach;
  forEach->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkPlaneCutter> slice;
  slice->SetInputConnection(forEach->GetOutputPort());

  vtkNew<vtkContourFilter> contour;
  contour->SetInputConnection(slice->GetOutputPort());
  contour->SetNumberOfContours(1);
  contour->SetValue(0, 1);

  vtkNew<vtkEndFor> endFor;
  endFor->SetInputConnection(contour->GetOutputPort());

  endFor->Update();

  auto output = endFor->GetOutput();
  auto pdsc = vtkPartitionedDataSetCollection::SafeDownCast(output);
  if (!pdsc)
  {
    std::cerr << "Output was not partitioned data set collection" << std::endl;
    return false;
  }

  if (pdsc->GetNumberOfPartitionedDataSets() != ::NB_SOURCE_TIME_STEPS)
  {
    std::cerr << "Output did not have correct number of blocks" << std::endl;
    return false;
  }

  // Check temporal change
  vtkDataSet* part1 = pdsc->GetPartition(0, 0);
  vtkDataSet* part2 = pdsc->GetPartition(1, 0);

  if (part1->GetNumberOfPoints() == part2->GetNumberOfPoints())
  {
    std::cerr
      << "Partitions have the same number of points, time not updated correctly in vtkEndFor "
      << std::endl;
    return false;
  }

  return true;
}

}

int TestForEach(int, char*[])
{
  bool res = true;
  res &= ::TestNoPipeline();
  res &= ::TestSimplePipeline();
  res &= ::TestComplexPipeline();
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
