// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellType.h"
#include "vtkCellTypeSource.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include "vtkPoints.h"

#include "vtkTestUtilities.h"
#include <string>

int TestXMLLargeUnstructuredGrid(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string fileName(tempDir);
  delete[] tempDir;

  fileName += "/XMLLargeUnstructuredGrid.vtu";

  // Large file is > 2Gb when written to disk.
  int blocksDimensions[] = { 200, 200, 75 };
  // Create a UnstructuredGrid with tets, write the UnstructuredGrid, read the UnstructuredGrid
  // and compare the cell count
  vtkSmartPointer<vtkCellTypeSource> cellSource = vtkSmartPointer<vtkCellTypeSource>::New();
  cellSource->SetBlocksDimensions(blocksDimensions);
  cellSource->SetCellType(VTK_TETRA);
  cellSource->SetOutputPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  cellSource->Update();
  std::cout << "Write to " << fileName << std::endl;

  // write the UnstructuredGrid
  vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
    vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  writer->SetInputData(cellSource->GetOutput());
  writer->SetFileName(fileName.c_str());
  // large files failed in binary mode on windows,
  // https://gitlab.kitware.com/paraview/paraview/-/issues/21145
  // File must be larger than 2^31 to trigger the bug, don't compress.
  writer->SetDataModeToBinary();
  writer->SetCompressorTypeToNone();
  writer->Write();

  // read back the UnstructuredGrid
  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
    vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
  reader->SetFileName(fileName.c_str());

  if (!reader->CanReadFile(fileName.c_str()))
  {
    std::cerr
      << "CanReadFile failed, likely cause: external Expat configured without XML_LARGE_SIZE"
      << std::endl;
    return EXIT_FAILURE;
  }
  reader->Update();

  if (cellSource->GetOutput()->GetNumberOfCells() != reader->GetOutput()->GetNumberOfCells())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
