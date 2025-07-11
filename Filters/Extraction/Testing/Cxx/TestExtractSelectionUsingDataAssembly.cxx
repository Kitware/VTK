// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkConvertSelection.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkExtractSelection.h"
#include "vtkIOSSReader.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSource.h"
#include "vtkTestUtilities.h"
#include "vtkUniformGrid.h"

namespace
{
std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

bool TestPDC(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  auto fname = ::GetFileName(argc, argv, std::string("Data/can.ex2"));
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
  // let's also test selection converter.
  selSource->RemoveAllSelectors();
  selSource->RemoveAllBlockSelectors();
  selSource->SetCompositeIndex(3u);
  selSource->AddID(-1, 0);
  selSource->SetFieldType(vtkSelectionNode::CELL);
  selSource->SetContentType(vtkSelectionNode::INDICES);

  vtkNew<vtkConvertSelection> converter;
  converter->SetOutputType(vtkSelectionNode::BLOCK_SELECTORS);
  converter->SetDataObjectConnection(reader->GetOutputPort());
  converter->SetInputConnection(selSource->GetOutputPort());
  extractor->SetInputConnection(1, converter->GetOutputPort());
  extractor->Update();
  vtkLogIfF(ERROR,
    extractor->GetOutputDataObject(0)->GetNumberOfElements(vtkDataObject::CELL) != 2352,
    "Incorrect selection after conversion for '//block_2'!");

  return true;
}

bool TestAMR()
{
  vtkNew<vtkNonOverlappingAMR> amr;

  // Create and populate the Non Overlapping AMR dataset.
  // The dataset should look like
  // Level 0
  //   uniform grid
  // Level 1
  //   uniform grid
  //   uniform grid
  //   empty node
  std::vector<unsigned int> blocksPerLevel{ 1, 3 };
  amr->Initialize(blocksPerLevel);

  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 1.0, 1.0, 1.0 };
  int dims[3] = { 11, 11, 6 };

  vtkNew<vtkUniformGrid> ug1;
  // Geometry
  ug1->SetOrigin(origin);
  ug1->SetSpacing(spacing);
  ug1->SetDimensions(dims);

  amr->SetDataSet(0, 0, ug1);

  double origin2[3] = { 0.0, 0.0, 5.0 };
  double spacing2[3] = { 1.0, 0.5, 1.0 };

  vtkNew<vtkUniformGrid> ug2;
  // Geometry
  ug2->SetOrigin(origin2);
  ug2->SetSpacing(spacing2);
  ug2->SetDimensions(dims);

  amr->SetDataSet(1, 0, ug2);

  double origin3[3] = { 0.0, 5.0, 5.0 };

  vtkNew<vtkUniformGrid> ug3;
  // Geometry
  ug3->SetOrigin(origin3);
  ug3->SetSpacing(spacing2);
  ug3->SetDimensions(dims);

  amr->SetDataSet(1, 1, ug3);

  // select cell 0 without any qualifiers.
  vtkNew<vtkSelectionSource> selSource;
  selSource->SetContentType(vtkSelectionNode::INDICES);
  selSource->SetFieldType(vtkSelectionNode::CELL);
  selSource->AddID(-1, 0);

  vtkNew<vtkExtractSelection> extractor;
  extractor->SetInputData(0, amr);
  extractor->SetInputConnection(1, selSource->GetOutputPort());
  extractor->Update();

  vtkPartitionedDataSetCollection* pdc =
    vtkPartitionedDataSetCollection::SafeDownCast(extractor->GetOutputDataObject(0));
  if (!pdc)
  {
    vtkLogF(ERROR, "Incorrect output type for selection extraction of an AMR!");
    return false;
  }

  if (extractor->GetOutputDataObject(0)->GetNumberOfElements(vtkDataObject::CELL) != 3)
  {
    vtkLogF(ERROR, "Incorrect selection without qualifiers for AMR!");
    return false;
  }

  // reset selSource.
  selSource->RemoveAllSelectors();
  selSource->SetAssemblyName(nullptr);
  selSource->RemoveAllIDs();

  // Now test vtkSelectionNode::BLOCK_SELECTORS.
  selSource->SetContentType(vtkSelectionNode::BLOCK_SELECTORS);

  selSource->AddBlockSelector("/Root/Level1");
  extractor->SetInputConnection(1, selSource->GetOutputPort());
  extractor->Update();

  if (extractor->GetOutputDataObject(0)->GetNumberOfElements(vtkDataObject::CELL) != 1000)
  {
    vtkLogF(ERROR, "Incorrect selection for selection '/Root/Level1' in AMR!");
    return false;
  }

  return true;
}
}

int TestExtractSelectionUsingDataAssembly(int argc, char* argv[])
{
  bool success = true;
  success &= ::TestPDC(argc, argv);
  success &= ::TestAMR();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
