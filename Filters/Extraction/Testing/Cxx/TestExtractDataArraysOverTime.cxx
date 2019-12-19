/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractDataArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractDataArraysOverTime.h"

#include "vtkExodusIIReader.h"
#include "vtkExtractSelection.h"
#include "vtkExtractTimeSteps.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSource.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"

#define expect(x, msg)                                                                             \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << __LINE__ << ": " msg << endl;                                                          \
    return false;                                                                                  \
  }

namespace
{
bool Validate0(vtkMultiBlockDataSet* mb, int num_timesteps)
{
  expect(mb != nullptr, "expecting a vtkMultiBlockDataSet.");
  expect(mb->GetNumberOfBlocks() == 2, "expecting 2 blocks, got " << mb->GetNumberOfBlocks());

  vtkTable* b0 = vtkTable::SafeDownCast(mb->GetBlock(0));
  expect(b0 != nullptr, "expecting a vtkTable for block 0");
  expect(b0->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows());
  expect(b0->GetNumberOfColumns() > 100, "mismatched columns");

  vtkTable* b1 = vtkTable::SafeDownCast(mb->GetBlock(1));
  expect(b1 != nullptr, "expecting a vtkTable for block 1");
  expect(b1->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b1->GetNumberOfRows());
  expect(b1->GetNumberOfColumns() > 100, "mismatched columns");
  return true;
}

bool Validate1(vtkMultiBlockDataSet* mb, int num_timesteps, const char* bname)
{
  expect(mb != nullptr, "expecting a vtkMultiBlockDataSet.");
  expect(mb->GetNumberOfBlocks() == 1, "expecting 1 block, got " << mb->GetNumberOfBlocks());

  vtkTable* b0 = vtkTable::SafeDownCast(mb->GetBlock(0));
  expect(b0 != nullptr, "expecting a vtkTable for block 0");
  expect(b0->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows());
  expect(b0->GetNumberOfColumns() >= 5, "mismatched columns");

  const char* name = mb->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME());
  expect(name != nullptr, "expecting non-null name.");
  expect(strcmp(name, bname) == 0,
    "block name not matching,"
    " expected '"
      << bname << "', got '" << name << "'");
  return true;
}
}

int TestExtractDataArraysOverTime(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");

  vtkNew<vtkExodusIIReader> reader;
  reader->SetFileName(fname);
  reader->UpdateInformation();
  reader->SetAllArrayStatus(vtkExodusIIReader::NODAL, 1);
  reader->SetAllArrayStatus(vtkExodusIIReader::ELEM_BLOCK, 1);
  reader->SetGenerateGlobalElementIdArray(true);
  reader->SetGenerateGlobalNodeIdArray(true);
  delete[] fname;

  // lets limit to 10 timesteps to reduce test time.
  vtkNew<vtkExtractTimeSteps> textracter;
  textracter->SetInputConnection(reader->GetOutputPort());
  textracter->UpdateInformation();
  textracter->GenerateTimeStepIndices(1, 11, 1);
  const int num_timesteps = 10;

  vtkNew<vtkExtractDataArraysOverTime> extractor;
  extractor->SetReportStatisticsOnly(true);
  extractor->SetInputConnection(textracter->GetOutputPort());
  extractor->Update();

  if (!Validate0(
        vtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)), num_timesteps))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  // let's try non-summary extraction.

  vtkNew<vtkSelectionSource> selSource;
  selSource->SetContentType(vtkSelectionNode::GLOBALIDS);
  selSource->SetFieldType(vtkSelectionNode::CELL);
  selSource->AddID(0, 100);

  vtkNew<vtkExtractSelection> iextractor;
  iextractor->SetInputConnection(0, textracter->GetOutputPort());
  iextractor->SetInputConnection(1, selSource->GetOutputPort());

  extractor->SetReportStatisticsOnly(false);
  extractor->SetInputConnection(iextractor->GetOutputPort());
  extractor->SetFieldAssociation(vtkDataObject::CELL);
  extractor->Update();
  if (!Validate1(vtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
        num_timesteps, "gid=100"))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  // this time, simply use element id.
  extractor->SetUseGlobalIDs(false);
  extractor->Update();
  if (!Validate1(vtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
        num_timesteps, "originalId=99 block=2"))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  // this time, request using vtkOriginalCellIds to id the elements.
  extractor->SetUseGlobalIDs(false);
  extractor->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "vtkOriginalCellIds");
  extractor->Update();
  if (!Validate1(vtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
        num_timesteps, "originalId=99 block=2"))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
