// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkEndFor.h>
#include <vtkForEach.h>
#include <vtkImageDataToPointSet.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSetCollection.h>
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

  vtkNew<vtkImageDataToPointSet> toPointSet;
  toPointSet->SetInputConnection(forEach->GetOutputPort());

  vtkNew<vtkRandomAttributeGenerator> random;
  random->SetInputConnection(toPointSet->GetOutputPort());

  vtkNew<vtkEndFor> endFor;
  endFor->SetInputConnection(random->GetOutputPort());

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

}

int TestForEach(int, char*[])
{
  bool res = true;
  res &= ::TestNoPipeline();
  res &= ::TestSimplePipeline();
  res &= ::TestComplexPipeline();
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
