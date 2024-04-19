// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests an extraction of a block using first vtkExtractBlock then
// a block selection from a vtkSelection and vtkExtractSelection

#include <vtkCompositeDataIterator.h>
#include <vtkExtractBlock.h>
#include <vtkExtractSelection.h>
#include <vtkFieldData.h>
#include <vtkIntArray.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkSelectionSource.h>
#include <vtkSphereSource.h>

namespace
{
vtkSmartPointer<vtkDataObject> GetSphere(double x, double y, double z)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(x, y, z);
  sphere->Update();
  return sphere->GetOutputDataObject(0);
}

int TestExtractBlockMultiBlock()
{
  vtkNew<vtkMultiBlockDataSet> mb0;
  mb0->SetBlock(0, GetSphere(0, 0, -2));
  mb0->SetBlock(1, GetSphere(0, 0, 2));

  // Add a field data to the multiblock dataset
  vtkNew<vtkIntArray> fieldData;
  std::string globalIdName("GlobalID");
  fieldData->SetName(globalIdName.c_str());
  fieldData->SetNumberOfComponents(1);
  fieldData->SetNumberOfTuples(1);
  fieldData->SetValue(0, 5);
  mb0->GetFieldData()->AddArray(fieldData);

  // Test vtkExtractBlock
  vtkNew<vtkExtractBlock> extractBlock;
  extractBlock->AddIndex(2);
  extractBlock->SetPruneOutput(1);
  extractBlock->SetInputData(mb0);
  extractBlock->Update();

  auto output = vtkMultiBlockDataSet::SafeDownCast(extractBlock->GetOutput());
  if (output->GetBlock(0) == nullptr)
  {
    std::cerr << "Invalid block extracted by vtkExtractBlock. Should be block 0." << std::endl;
    std::cerr << *output << std::endl;
    return EXIT_FAILURE;
  }
  vtkIntArray* outputFDArray =
    vtkIntArray::SafeDownCast(output->GetFieldData()->GetArray(globalIdName.c_str()));
  if (outputFDArray == nullptr || outputFDArray->GetValue(0) != 5)
  {
    std::cerr << "Field data not copied to output. Should be." << std::endl;
    std::cerr << *output << std::endl;
    return EXIT_FAILURE;
  }

  // Now test a block selection
  vtkNew<vtkSelectionSource> selectionSource;
  selectionSource->SetContentType(vtkSelectionNode::BLOCKS);
  selectionSource->AddBlock(2);

  vtkNew<vtkExtractSelection> extract;
  extract->SetInputDataObject(mb0);
  extract->SetSelectionConnection(selectionSource->GetOutputPort());
  extract->Update();

  output = vtkMultiBlockDataSet::SafeDownCast(extract->GetOutput());
  if (output->GetBlock(0) != nullptr || output->GetBlock(1) == nullptr)
  {
    std::cerr << "Invalid block extracted. Should be block 1." << std::endl;
    std::cerr << *output << std::endl;
    return EXIT_FAILURE;
  }
  outputFDArray = vtkIntArray::SafeDownCast(output->GetFieldData()->GetArray(globalIdName.c_str()));
  if (outputFDArray == nullptr || outputFDArray->GetValue(0) != 5)
  {
    std::cerr << "Field data not copied to output. Should be." << std::endl;
    return EXIT_FAILURE;
  }

  // now extract non-leaf block.
  selectionSource->RemoveAllBlocks();
  selectionSource->AddBlock(1);

  vtkNew<vtkMultiBlockDataSet> mb1;
  mb1->SetBlock(0, mb0);
  mb1->SetBlock(1, GetSphere(0, 0, 3));

  extract->SetInputDataObject(mb1);
  extract->Update();

  output = vtkMultiBlockDataSet::SafeDownCast(extract->GetOutput());
  if (vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0)) == nullptr ||
    vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0))->GetBlock(0) == nullptr ||
    vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0))->GetBlock(1) == nullptr ||
    output->GetBlock(1) != nullptr)
  {
    std::cerr << "Incorrect non-leaf block extraction!" << std::endl;
    std::cerr << *output << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int TestExtractBlockPartitionedDataSetCollection()
{
  vtkNew<vtkPartitionedDataSetCollection> pdc;
  for (unsigned int part = 0; part < 5; ++part)
  {
    vtkNew<vtkPartitionedDataSet> pd;
    for (unsigned int cc = 0; cc <= part; ++cc)
    {
      pd->SetPartition(cc, ::GetSphere(cc, part, 0));
    }
    pdc->SetPartitionedDataSet(part, pd);
  }

  vtkNew<vtkExtractBlock> eb;
  eb->SetInputDataObject(pdc);
  eb->AddIndex(3);  // partitioned-dataset #2, which has 2 partitions
  eb->AddIndex(13); // partitioned-dataset #4, partition #2
  eb->AddIndex(14); // partitioned-dataset #4, partition #3
  eb->Update();

  auto output = vtkPartitionedDataSetCollection::SafeDownCast(eb->GetOutput());
  int count = 0;
  auto iter = output->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    ++count;
  }
  iter->Delete();

  if (count != 4)
  {
    std::cerr << "Incorrect blocks extracted for vtkPartitionedDataSetCollection." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
}

int TestExtractBlock(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  if (TestExtractBlockMultiBlock() != EXIT_SUCCESS ||
    TestExtractBlockPartitionedDataSetCollection() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
