// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGDALRasterReader.h"

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkErrorCode.h>
#include <vtkNew.h>
#include <vtkObject.h>
#include <vtkTestUtilities.h>
#include <vtkUniformGrid.h>

#include <iostream>

int TestGDALRasterReaderBadBands(int argc, char* argv[])
{
  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/GIS/badbands.tif");
  if (!fileName)
  {
    std::cerr << "Error: could not locate test file Data/GIS/badbands.tif" << std::endl;
    return 1;
  }

  vtkNew<vtkGDALRasterReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;

  // badbands.tif has valid structure but corrupt pixel data; GDAL RasterIO returns
  // CE_Failure when reading it. The reader must detect this and halt the pipeline
  // (RequestData returns 0) rather than silently producing output from a failed read.
  // The halted pipeline is the expected outcome here, not a test fault, so suppress
  // the output.
  vtkObject::GlobalWarningDisplayOff();
  reader->Update();
  vtkObject::GlobalWarningDisplayOn();

  if (reader->GetErrorCode() == vtkErrorCode::ErrorIds::NoError)
  {
    std::cerr << "Error: expected reader to have an error code, got NoError: "
              << reader->GetErrorCode() << std::endl;
    return 1;
  }

  vtkUniformGrid* output = vtkUniformGrid::SafeDownCast(reader->GetOutput());

  if (output->GetNumberOfCells() > 0)
  {
    std::cerr << "Error: expected empty output for malformed raster file, got "
              << output->GetNumberOfCells() << " cells" << std::endl;
    return 1;
  }

  return 0;
}
