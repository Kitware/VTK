// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtractExodusGlobalTemporalVariables.h"

#include "vtkExodusIIReader.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"

#include <vector>
int TestExtractExodusGlobalTemporalVariables(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  vtkNew<vtkExodusIIReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  reader->UpdateInformation();
  const std::vector<int> types{ vtkExodusIIReader::GLOBAL_TEMPORAL, vtkExodusIIReader::GLOBAL,
    vtkExodusIIReader::QA_RECORDS, vtkExodusIIReader::INFO_RECORDS };

  for (const auto& type : types)
  {
    for (int cc = 0; cc < reader->GetNumberOfGlobalResultArrays(); ++cc)
    {
      reader->SetObjectArrayStatus(type, reader->GetObjectArrayName(type, cc), 1);
    }
  }

  vtkNew<vtkExtractExodusGlobalTemporalVariables> extractor;
  extractor->SetInputConnection(reader->GetOutputPort());
  extractor->Update();

  auto output = vtkTable::SafeDownCast(extractor->GetOutputDataObject(0));
  if (!output || output->GetNumberOfRows() != 44 || output->GetNumberOfColumns() != 7)
  {
    vtkLogF(ERROR, "Failed for AutoDetectGlobalTemporalDataArrays=true");
    return EXIT_FAILURE;
  }

  extractor->SetAutoDetectGlobalTemporalDataArrays(false);
  extractor->Update();
  output = vtkTable::SafeDownCast(extractor->GetOutputDataObject(0));
  if (!output || output->GetNumberOfRows() != 44 || output->GetNumberOfColumns() != 3)
  {
    vtkLogF(ERROR, "Failed for AutoDetectGlobalTemporalDataArrays=false");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
