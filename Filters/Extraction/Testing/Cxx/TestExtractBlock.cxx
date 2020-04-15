/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests an extraction of a block using first vtkExtractBlock then
// a block selection from a vtkSelection and vtkExtractSelection

#include <vtkExtractBlock.h>
#include <vtkExtractSelection.h>
#include <vtkFieldData.h>
#include <vtkIntArray.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
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
}

int TestExtractBlock(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
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
