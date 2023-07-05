// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellArray.h"
#include "vtkDataSetWriter.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkQuad.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

int TestLegacyDataSetWriterSetFileVersion(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  // Create a grid
  vtkNew<vtkUnstructuredGrid> dataSet;
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> cellArray;

  // Add points
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(1, 1, 0);

  dataSet->SetPoints(points);

  // Add quad
  vtkNew<vtkQuad> quad;
  quad->GetPointIds()->SetId(0, 0);
  quad->GetPointIds()->SetId(1, 1);
  quad->GetPointIds()->SetId(2, 2);
  quad->GetPointIds()->SetId(3, 3);

  cellArray->InsertNextCell(quad);
  dataSet->SetCells(VTK_QUAD, cellArray);

  // Write file at version 4.2
  vtkNew<vtkDataSetWriter> writer;
  writer->SetFileVersion(42);

  std::string filename = testing->GetTempDirectory();
  filename += "/datasetwriteroutput.vtk";
  writer->SetFileName(filename.c_str());
  writer->SetInputData(dataSet);
  writer->Write();

  std::ifstream fileStream(filename);
  if (!fileStream.good())
  {
    std::cerr << "Can't open file: " << filename << std::endl;
    return EXIT_FAILURE;
  }

  std::string firstLine;
  std::getline(fileStream, firstLine);

  const std::string expectedFirstLine = "# vtk DataFile Version 4.2";
  if (firstLine != expectedFirstLine)
  {
    std::cerr << "Wrong file header:" << std::endl;
    std::cerr << firstLine << std::endl;
    std::cerr << "Expected header:" << std::endl;
    std::cerr << expectedFirstLine << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
