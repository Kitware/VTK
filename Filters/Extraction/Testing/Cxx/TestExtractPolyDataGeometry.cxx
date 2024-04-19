// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtractPolyDataGeometry.h"

#include "vtkBox.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"

int TestExtractPolyDataGeometry(int argc, char* argv[])
{
  // Construct clipping box
  vtkNew<vtkBox> box;
  box->SetBounds(0.0, 1.5, 0.0, 0.25, 0.0, 0.5);

  // Load data set
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cad_cubes.vtp");
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  vtkPolyData* input = reader->GetOutput();

  // Setup extractor and execute
  vtkNew<vtkExtractPolyDataGeometry> extractor;
  extractor->SetInputData(input);
  extractor->SetImplicitFunction(box);
  extractor->Update();

  // Retrieve and check output
  vtkPolyData* output = extractor->GetOutput();

  if (!output)
  {
    std::cerr << "Wrong output." << std::endl;
    return EXIT_FAILURE;
  }

  if (output->GetNumberOfVerts() != 8)
  {
    std::cerr << "Wrong number of vertices." << std::endl;
    return EXIT_FAILURE;
  }

  if (output->GetNumberOfLines() != 8)
  {
    std::cerr << "Wrong number of lines." << std::endl;
    return EXIT_FAILURE;
  }

  if (output->GetNumberOfPolys() != 24)
  {
    std::cerr << "Wrong number of polys." << std::endl;
    return EXIT_FAILURE;
  }

  if (output->GetNumberOfPoints() != 34)
  {
    std::cerr << "Wrong number of points." << std::endl;
    return EXIT_FAILURE;
  }

  if (output->GetCellData()->GetArray("Solid id")->GetNumberOfTuples() != 40)
  {
    std::cerr << "Wrong size for cell array 'Solid id'." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
