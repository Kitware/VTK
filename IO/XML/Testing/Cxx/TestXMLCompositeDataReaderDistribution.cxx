/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLCompositeDataReaderDistribution.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXMLCompositeDataReader.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkPolyData.h"
#include "vtkXMLMultiBlockDataReader.h"

#include "vtkNew.h"

#include <iostream>
#include <set>
#include <sstream>

#define TEST_ASSERT(cond, message)                                                                 \
  do                                                                                               \
  {                                                                                                \
    if (!(cond))                                                                                   \
    {                                                                                              \
      std::cerr << "Failure at line " << __LINE__ << ":\n"                                         \
                << "\tCondition: " << #cond << "\n"                                                \
                << "\tError: " << message << "\n";                                                 \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (0) /* do-while swallows semicolons */

namespace
{
// Returns a multiset containing the number of points in each leaf dataset.
std::multiset<vtkIdType> PointCounts(vtkCompositeDataSet* cds)
{
  std::multiset<vtkIdType> result;

  using SmartIterator = vtkSmartPointer<vtkCompositeDataIterator>;
  auto it = SmartIterator::Take(cds->NewIterator());
  it->SkipEmptyNodesOn();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    vtkPolyData* pd = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
    if (pd)
    {
      result.insert(pd->GetNumberOfPoints());
    }
  }
  return result;
}

bool VerifyCounts(vtkCompositeDataSet* cds, const std::multiset<vtkIdType>& expected)
{
  return PointCounts(cds) == expected;
}

std::string DumpCounts(vtkCompositeDataSet* cds)
{
  const std::multiset<vtkIdType> ids = PointCounts(cds);
  std::ostringstream out;
  out << "{ ";
  for (auto id : ids)
  {
    out << id << " ";
  }
  out << "}";
  return out.str();
}
}

int TestXMLCompositeDataReaderDistribution(int argc, char* argv[])
{

  if (argc < 2)
  {
    std::cerr << "Missing argument.\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkXMLMultiBlockDataReader> in;
  in->SetFileName(argv[1]);

  // Verify that the dataset is what we expect: 10 leaves, each with a unique
  // number of points spanning 1-10 inclusive.
  {
    in->Update();
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));
  }

  // Simulate a request for a partitioning across 3 processors:
  vtkNew<vtkInformation> req;
  req->Set(vtkCompositeDataPipeline::UPDATE_NUMBER_OF_PIECES(), 3);

  // Verify that block loading works as expected.
  {
    in->SetPieceDistribution(vtkXMLMultiBlockDataReader::Block);

    // First processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 0);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 1, 2, 3, 4 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));

    // Second processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 1);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 5, 6, 7 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));

    // Third processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 2);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 8, 9, 10 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));
  }

  // Verify that interleaved loading works as expected.
  {
    in->SetPieceDistribution(vtkXMLMultiBlockDataReader::Interleave);

    // First processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 0);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 1, 4, 7, 10 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));

    // Second processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 1);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 2, 5, 8 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));

    // Third processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 2);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 3, 6, 9 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));
  }

  // Add an update restriction to test that loading is balanced when datasets 2
  // and 7 (point counts, not ids) are ignored.
  req->Append(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), 0);
  req->Append(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), 2);
  req->Append(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), 3);
  req->Append(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), 4);
  req->Append(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), 5);
  req->Append(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), 7);
  req->Append(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), 8);
  req->Append(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), 9);

  // Verify that block loading works as expected.
  {
    in->SetPieceDistribution(vtkXMLMultiBlockDataReader::Block);

    // First processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 0);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 1, 3, 4 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));

    // Second processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 1);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 5, 6, 8 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));

    // Third processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 2);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 9, 10 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));
  }

  // Verify that interleaved loading works as expected.
  {
    in->SetPieceDistribution(vtkXMLMultiBlockDataReader::Interleave);

    // First processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 0);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 1, 5, 9 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));

    // Second processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 1);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 3, 6, 10 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));

    // Third processor:
    req->Set(vtkCompositeDataPipeline::UPDATE_PIECE_NUMBER(), 2);
    in->Update(req);
    TEST_ASSERT(VerifyCounts(in->GetOutput(), { 4, 8 }),
      "Incorrect partitioning: " << DumpCounts(in->GetOutput()));
  }

  return EXIT_SUCCESS;
}
