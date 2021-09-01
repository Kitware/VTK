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
#include <vtkConvertSelection.h>
#include <vtkDataAssemblyUtilities.h>
#include <vtkExtractSelection.h>
#include <vtkIOSSReader.h>
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
  vtkNew<vtkIOSSReader> reader;
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

  // reset selSource.
  selSource->RemoveAllSelectors();
  selSource->SetAssemblyName(nullptr);
  selSource->RemoveAllIDs();

  // Now test vtkSelectionNode::BLOCK_SELECTORS.
  selSource->SetContentType(vtkSelectionNode::BLOCK_SELECTORS);

  selSource->AddBlockSelector("//block_2");
  extractor->SetInputConnection(1, selSource->GetOutputPort());
  extractor->Update();
  vtkLogIfF(ERROR,
    extractor->GetOutputDataObject(0)->GetNumberOfElements(vtkDataObject::CELL) != 2352,
    "Incorrect selection for selection '//block_2'!");

  selSource->RemoveAllSelectors();
  selSource->AddBlockSelector("//element_blocks");
  selSource->SetArrayName("Assembly");
  selSource->SetFieldType(vtkSelectionNode::POINT);
  extractor->Update();
  vtkLogIfF(ERROR,
    extractor->GetOutputDataObject(0)->GetNumberOfElements(vtkDataObject::POINT) != 10088,
    "Incorrect selection for selection '//element_blocks'!");

  //------------------------------------------------------------------------
  // let's also test selection convertor.
  selSource->RemoveAllSelectors();
  selSource->RemoveAllBlockSelectors();
  selSource->SetCompositeIndex(3u);
  selSource->AddID(-1, 0);
  selSource->SetFieldType(vtkSelectionNode::CELL);
  selSource->SetContentType(vtkSelectionNode::INDICES);

  vtkNew<vtkConvertSelection> convertor;
  convertor->SetOutputType(vtkSelectionNode::BLOCK_SELECTORS);
  convertor->SetDataObjectConnection(reader->GetOutputPort());
  convertor->SetInputConnection(selSource->GetOutputPort());
  extractor->SetInputConnection(1, convertor->GetOutputPort());
  extractor->Update();
  vtkLogIfF(ERROR,
    extractor->GetOutputDataObject(0)->GetNumberOfElements(vtkDataObject::CELL) != 2352,
    "Incorrect selection after conversion for '//block_2'!");

  return EXIT_SUCCESS;
}
