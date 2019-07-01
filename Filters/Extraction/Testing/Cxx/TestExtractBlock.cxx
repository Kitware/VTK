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
// This tests an extraction of a block from vtkSelection

#include <vtkExtractSelection.h>
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

  vtkNew<vtkSelectionSource> selectionSource;
  selectionSource->SetContentType(vtkSelectionNode::BLOCKS);
  selectionSource->AddBlock(2);

  vtkNew<vtkExtractSelection> extract;
  extract->SetInputDataObject(mb0);
  extract->SetSelectionConnection(selectionSource->GetOutputPort());
  extract->Update();

  auto output = vtkMultiBlockDataSet::SafeDownCast(extract->GetOutput());
  if (output->GetBlock(0) != nullptr || output->GetBlock(1) == nullptr)
  {
    std::cerr << "Invalid block extracted. Should be block 1." << std::endl;
    std::cerr << *output << std::endl;
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
