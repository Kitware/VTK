// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFieldData.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkTimeSourceExample.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLDataObjectWriter.h"
#include "vtkXMLUnstructuredGridReader.h"

bool CheckHasTimeValue(vtkUnstructuredGrid* output)
{
  vtkFieldData* fd = output->GetFieldData();
  if (!fd)
  {
    return false;
  }
  vtkDataArray* array = fd->GetArray("TimeValue");
  if (!array)
  {
    return false;
  }
  return true;
}

int TestXMLWriteTimeValue(int argc, char* argv[])
{
  std::string tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string fileNameWithTimeValue(tempDir + "/TestWriteTimeValue.vtu");
  std::string fileNameWithoutTimeValue(tempDir + "/TestWriteNoTimeValue.vtu");

  vtkNew<vtkTimeSourceExample> timeSource;
  timeSource->SetXAmplitude(10);
  timeSource->SetYAmplitude(0);
  timeSource->Update();

  vtkNew<vtkXMLDataObjectWriter> writer;
  writer->SetInputConnection(timeSource->GetOutputPort());
  writer->SetFileName(fileNameWithTimeValue.c_str());
  writer->SetWriteTimeValue(true);
  writer->Write();

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(fileNameWithTimeValue.c_str());
  reader->Update();

  if (!CheckHasTimeValue(reader->GetOutput()))
  {
    std::cerr << "TimeValue field data not found!" << std::endl;
    return EXIT_FAILURE;
  }

  writer->SetFileName(fileNameWithoutTimeValue.c_str());
  writer->SetWriteTimeValue(false);
  writer->Write();

  reader->SetFileName(fileNameWithoutTimeValue.c_str());
  reader->Update();

  if (CheckHasTimeValue(reader->GetOutput()))
  {
    std::cerr << "TimeValue field data is found, but it should not have been written!" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
