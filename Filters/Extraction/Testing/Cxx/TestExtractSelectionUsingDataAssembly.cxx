/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelectionUsingDataAssembly.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkDataAssemblyUtilities.h>
#include <vtkExtractSelection.h>
#include <vtkIossReader.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkSelectionNode.h>
#include <vtkSelectionSource.h>
#include <vtkTestUtilities.h>

static std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

int TestExtractSelectionUsingDataAssembly(int argc, char* argv[])
{
  vtkNew<vtkIossReader> reader;
  auto fname = GetFileName(argc, argv, std::string("Data/can.ex2"));
  reader->AddFileName(fname.c_str());

  // select cell 0 without any qualifiers.
  vtkNew<vtkSelectionSource> selSource;
  selSource->SetContentType(vtkSelectionNode::INDICES);
  selSource->SetFieldType(vtkSelectionNode::CELL);
  selSource->AddID(-1, 0);

  vtkNew<vtkExtractSelection> extractor;
  extractor->SetInputConnection(0, reader->GetOutputPort());
  extractor->SetInputConnection(1, selSource->GetOutputPort());
  extractor->Update();

  vtkLogIfF(ERROR, extractor->GetOutputDataObject(0)->GetNumberOfElements(vtkDataObject::CELL) != 2,
    "Incorrect selection without qualifiers!");

  // select cell 0 limited to "block_2" using hierarchy.
  selSource->SetAssemblyName(vtkDataAssemblyUtilities::HierarchyName());
  selSource->AddSelector("//*[@label='block_2']");

  extractor->Update();
  vtkLogIfF(ERROR, extractor->GetOutputDataObject(0)->GetNumberOfElements(vtkDataObject::CELL) != 1,
    "Incorrect selection for selector '//*[@label='block_2']'!");

  // select cell 0 limited to "element_blocks" using assembly.
  selSource->SetAssemblyName("Assembly");
  selSource->AddSelector("//element_blocks");

  extractor->Update();
  vtkLogIfF(ERROR, extractor->GetOutputDataObject(0)->GetNumberOfElements(vtkDataObject::CELL) != 2,
    "Incorrect selection for selection '//element_blocks'!");

  return EXIT_SUCCESS;
}
