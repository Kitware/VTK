/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositeDataSetRange.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCompositeDataIterator.h>
#include <vtkCompositeDataSetRange.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#define TEST_FAIL(msg) \
  std::cerr << "Test failed! " msg << "\n"; \
  return false

namespace {

// Test that the for-range iterators behave the same as the regular iterators.
bool TestConfig(vtkCompositeDataSet *cds,
                vtk::CompositeDataSetOptions opts)
{
  using SmartIterator = vtkSmartPointer<vtkCompositeDataIterator>;
  using Opts = vtk::CompositeDataSetOptions;

  auto refIter = SmartIterator::Take(cds->NewIterator());
  refIter->SetSkipEmptyNodes((opts & Opts::SkipEmptyNodes) != Opts::None);
  refIter->InitTraversal();

  for (auto dObj : vtk::Range(cds, opts))
  {
    if (refIter->IsDoneWithTraversal())
    {
      TEST_FAIL("vtkCompositeIterator finished before Range iterator.");
    }
    if (dObj != refIter->GetCurrentDataObject())
    {
      TEST_FAIL("Range iterator does not match vtkCompositeDataIterator.");
    }

    refIter->GoToNextItem();
  }

  if (!refIter->IsDoneWithTraversal())
  {
    TEST_FAIL("Range iterator did not completely traverse composite dataset.");
  }

  return true;
}

bool TestOptions(vtkCompositeDataSet *cds)
{
  using Opts = vtk::CompositeDataSetOptions;

  if (!TestConfig(cds, Opts::None))
  {
    TEST_FAIL("Error while testing options 'None'.");
  }
  if (!TestConfig(cds, Opts::SkipEmptyNodes))
  {
    TEST_FAIL("Error while testing options 'SkipEmptyNodes'.");
  }

  return true;
}

// Construct the following hierarchy for testing:
// M = MBDS; P = PolyData; 0 = null dataset
//
//  ------------------------M------------------------ // depth 0
//  | |                     |                       |
//  P 0  -------------------M--                     M // depth 1
//       |       | |          |                     |
//  -----M-----  0 P    ------M         ------------M // depth 2
//  |    |    |         |     |         |           |
//  0    0    0         P     0   ------M-----      0 // depth 3
//                                |     |    |
//                                M     0    P        // depth 4
//                                |
//                                P                   // depth 5
//
vtkSmartPointer<vtkCompositeDataSet> CreateDataSet()
{
  auto addPolyData = [](unsigned int blockNum, vtkMultiBlockDataSet *mbds)
      -> vtkSmartPointer<vtkPolyData>
  {
    vtkNew<vtkPolyData> pd;
    mbds->SetBlock(blockNum, pd);
    return {pd};
  };

  auto addMultiBlock = [](unsigned int blockNum, vtkMultiBlockDataSet *mbds)
      -> vtkSmartPointer<vtkMultiBlockDataSet>
  {
    auto newMbds = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    mbds->SetBlock(blockNum, newMbds);
    return newMbds;
  };

  auto addNullDataSet = [](unsigned int blockNum,
                           vtkMultiBlockDataSet *mbds) -> void
  {
    mbds->SetBlock(blockNum, nullptr);
  };

  auto cds00 = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  cds00->SetNumberOfBlocks(4);
  addPolyData(0, cds00);
  addNullDataSet(1, cds00);
  auto cds10 = addMultiBlock(2, cds00);
  auto cds11 = addMultiBlock(3, cds00);

  cds10->SetNumberOfBlocks(4);
  auto cds20 = addMultiBlock(0, cds10);
  addNullDataSet(1, cds10);
  addPolyData(2, cds10);
  auto cds21 = addMultiBlock(3, cds10);

  cds11->SetNumberOfBlocks(1);
  auto cds22 = addMultiBlock(0, cds11);

  cds20->SetNumberOfBlocks(3);
  addNullDataSet(0, cds20);
  addNullDataSet(1, cds20);
  addNullDataSet(2, cds20);

  cds21->SetNumberOfBlocks(2);
  addPolyData(0, cds21);
  addNullDataSet(1, cds21);

  cds22->SetNumberOfBlocks(2);
  auto cds30 = addMultiBlock(0, cds22);
  addNullDataSet(1, cds22);

  cds30->SetNumberOfBlocks(3);
  auto cds40 = addMultiBlock(0, cds30);
  addNullDataSet(1, cds30);
  addPolyData(2, cds30);

  cds40->SetNumberOfBlocks(1);
  addPolyData(0, cds40);

  // explicit move needed to silence warnings about C++11 defect
  return std::move(cds00);
}

} // end anon namespace

int TestCompositeDataSetRange(int, char*[])
{
  auto cds = CreateDataSet();
  return TestOptions(cds) ? EXIT_SUCCESS : EXIT_FAILURE;
}
