// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "HDFTestUtilities.h"

#include "vtkAlgorithm.h"
#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkFieldData.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTableAlgorithm.h"
#include "vtkTestUtilities.h"

#include <string>

namespace HDFTestUtilities
{
vtkStandardNewMacro(vtkTemporalTableSource);
}

//----------------------------------------------------------------------------
bool TestHDFWriterTableTemporalBase(const std::string& tempDir)
{
  std::string filePath = tempDir + "/HDFWriterTableTemporal.vtkhdf";

  vtkNew<HDFTestUtilities::vtkTemporalTableSource> source;
  source->AddTemporalColumn("TemperatureInLyon",
    std::vector<std::vector<int>>{ { 35, 36, 34, 39 }, { 41 }, {}, { 43, 44 } });
  source->AddTemporalColumn("SweatinessRatio",
    std::vector<std::vector<double>>{ {
                                        0.6,
                                        0.7,
                                        0.4,
                                        0.8,
                                      },
      { 0.9 }, {}, { 0.95, 1.0 } });

  vtkNew<vtkHDFWriter> writer;
  writer->SetInputConnection(source->GetOutputPort());
  writer->SetFileName(filePath.c_str());
  writer->SetWriteAllTimeSteps(true);
  writer->Write();

  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(filePath.c_str());
  reader->UpdateInformation();

  constexpr int nbSteps = 4;
  if (reader->GetNumberOfSteps() != nbSteps)
  {
    vtkLog(ERROR, "Unexpected number of steps: " << reader->GetNumberOfSteps());
    return false;
  }

  for (int i = 0; i < nbSteps; i++)
  {
    reader->SetStep(i);
    reader->Update();
    vtkTable* tableRead = vtkTable::SafeDownCast(reader->GetOutputDataObject(0));

    source->GetOutputInformation(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), static_cast<double>(i));
    source->Modified();
    source->Update();
    vtkTable* tableSource = vtkTable::SafeDownCast(source->GetOutputDataObject(0));

    // Remove added "Time" field data so the comparison is correct
    vtkNew<vtkFieldData> fd;
    tableRead->SetFieldData(fd);

    if (!vtkTestUtilities::CompareDataObjects(tableRead, tableSource))
    {
      vtkLog(ERROR, << "Table does not match: " << filePath << " for time step " << i);
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
int TestHDFWriterTableTemporal(int argc, char* argv[])
{
  char* tempDirCStr =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tempDir{ tempDirCStr };
  delete[] tempDirCStr;

  bool testPasses = true;
  testPasses &= TestHDFWriterTableTemporalBase(tempDir);
  return testPasses ? EXIT_SUCCESS : EXIT_FAILURE;
}
