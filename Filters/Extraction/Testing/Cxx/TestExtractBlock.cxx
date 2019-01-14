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
#include <vtkMultiBlockDataGroupFilter.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkSelectionSource.h>
#include <vtkSphereSource.h>

int TestExtractBlock(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkSphereSource> sphere1;
  sphere1->SetCenter(0, 0, -2);
  vtkNew<vtkSphereSource> sphere2;
  sphere2->SetCenter(0, 0, 2);
  vtkNew<vtkMultiBlockDataGroupFilter> group;
  group->AddInputConnection(sphere1->GetOutputPort());
  group->AddInputConnection(sphere2->GetOutputPort());
  group->Update();

  vtkNew<vtkSelectionSource> selectionSource;
  selectionSource->SetContentType(vtkSelectionNode::BLOCKS);
  selectionSource->AddBlock(2);

  vtkNew<vtkExtractSelection> extract;
  extract->SetInputConnection(group->GetOutputPort());
  extract->SetSelectionConnection(selectionSource->GetOutputPort());
  extract->Update();

  auto output = vtkMultiBlockDataSet::SafeDownCast(extract->GetOutput());
  if (output->GetBlock(0) != nullptr || output->GetBlock(1) == nullptr)
  {
    std::cerr << "Invalid block extracted. Should be block 1." << std::endl;
    std::cerr << *output << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
