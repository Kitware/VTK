// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Description:
// This tests quadratic hexahedron reading with
// vtkXdmfReader

#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXdmfReader.h"

int TestQuadraticHexaXdmfReader(int argc, char* argv[])
{
  // Read the input data file
  char* filePath = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/XDMF/QuadraticHexa.xmf");
  vtkNew<vtkXdmfReader> reader;
  reader->SetFileName(filePath);
  delete[] filePath;
  reader->Update();

  auto vtuOutput = vtkUnstructuredGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (vtuOutput == nullptr)
  {
    std::cerr << "Error: null output." << std::endl;
    return 1;
  }

  if (vtuOutput->GetNumberOfPoints() != 27)
  {
    std::cerr << "Error: number of points should be 27." << std::endl;
    return 1;
  }

  return 0;
}
