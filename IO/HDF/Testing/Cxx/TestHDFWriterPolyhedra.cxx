// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This is a regression test that ensures polyhedra cells in vtkUnstructuredGrids
// survive HDF write/read using vtkHDFWriter and vtkHDFReader.
//
// Two types of tests are performed:
//
// * TestHDFWriterPolyhedraTemporal: tests that all timesteps are written and reread correctly when
// the
//   dataset contains polyhedra.
//
// * TestHDFWriterMixedCells: tests that polyhedra are written and reread
//   correctly when mixed with other cell types.

#include "vtkCellType.h"
#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

//------------------------------------------------------------------------------
bool TestHDFWriterPolyhedraTemporal(int argc, char* argv[])
{
  vtkLog(INFO, "Starting TestHDFWriterPolyhedraTemporal...");

  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();

  // Read the original polyhedron_temporal.vtkhdf file
  std::string inputFilePath = dataRoot + "/Data/vtkHDF/polyhedron_temporal.vtkhdf";
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(inputFilePath.c_str());
  reader->Update();

  // Get available timesteps
  vtkIdType numberOfTimesteps = reader->GetNumberOfSteps();
  vtkLog(INFO, "Number of timesteps available: " << numberOfTimesteps);

  if (numberOfTimesteps <= 1)
  {
    vtkLog(WARNING, "No timesteps found in dataset, processing single timestep");
    numberOfTimesteps = 1;
  }

  bool overallSuccess = true;

  // Iterate through all available timesteps and store the original data for later comparison
  std::vector<vtkSmartPointer<vtkUnstructuredGrid>> originalDataSets;
  for (vtkIdType timeIndex = 0; timeIndex < numberOfTimesteps; ++timeIndex)
  {
    vtkLog(INFO, "Processing timestep " << timeIndex << " of " << numberOfTimesteps);

    reader->SetStep(timeIndex);
    reader->Update();
    vtkUnstructuredGrid* originalData = vtkUnstructuredGrid::SafeDownCast(reader->GetOutput());
    if (!originalData)
    {
      vtkLog(ERROR, "Failed to read timestep " << timeIndex << " as vtkUnstructuredGrid");
      overallSuccess = false;
      continue;
    }

    // Shallow copy the original data for this timestep and store it for later comparison
    originalDataSets.push_back(vtkSmartPointer<vtkUnstructuredGrid>::New());
    originalDataSets.back()->ShallowCopy(originalData);

    vtkLog(INFO,
      "  Timestep " << timeIndex << " - Points=" << originalData->GetNumberOfPoints()
                    << ", Cells=" << originalData->GetNumberOfCells());
  }

  // Write the dataset to a temporary file
  std::string tempDir = testUtils->GetTempDirectory();
  std::string tempFilePath = tempDir + "/TestHDFWriterPolyhedra.vtkhdf";

  vtkNew<vtkHDFWriter> writer;
  writer->SetInputConnection(reader->GetOutputPort());
  writer->SetFileName(tempFilePath.c_str());
  writer->SetWriteAllTimeSteps(true);
  writer->SetCompressionLevel(1);
  writer->Write();

  vtkLog(INFO, "  Wrote temporary file to: " << tempFilePath);

  // Read the temporary file back
  vtkNew<vtkHDFReader> rereadReader;
  if (!rereadReader->CanReadFile(tempFilePath.c_str()))
  {
    vtkLog(ERROR, "  vtkHDFReader cannot read temporary file: " << tempFilePath);
    overallSuccess = false;
  }

  rereadReader->SetFileName(tempFilePath.c_str());
  rereadReader->Update();

  vtkUnstructuredGrid* rereadData = vtkUnstructuredGrid::SafeDownCast(rereadReader->GetOutput());
  if (!rereadData)
  {
    vtkLog(ERROR, "  Failed to read temporary file as vtkUnstructuredGrid");
    overallSuccess = false;
  }

  // Compare datasets at all timesteps to ensure they were written and reread correctly
  vtkIdType numberOfRereadTimesteps = rereadReader->GetNumberOfSteps();
  if (numberOfRereadTimesteps != numberOfTimesteps)
  {
    vtkLog(ERROR,
      "Timestep count mismatch: original=" << numberOfTimesteps
                                           << ", reread=" << numberOfRereadTimesteps);
    overallSuccess = false;
  }

  for (vtkIdType timeIndex = 0; timeIndex < numberOfRereadTimesteps; ++timeIndex)
  {
    rereadReader->SetStep(timeIndex);
    rereadReader->Update();
    rereadData = vtkUnstructuredGrid::SafeDownCast(rereadReader->GetOutput());
    if (!rereadData)
    {
      vtkLog(ERROR, "Failed to read reread timestep " << timeIndex);
      overallSuccess = false;
      continue;
    }

    vtkUnstructuredGrid* originalData = originalDataSets[timeIndex];
    bool comparisonSuccess = vtkTestUtilities::CompareDataObjects(originalData, rereadData);

    vtkLog(INFO, "  Timestep " << timeIndex << " comparison:");
    vtkLog(INFO,
      "    Original data Points=" << originalData->GetNumberOfPoints()
                                  << ", Cells=" << originalData->GetNumberOfCells());
    vtkLog(INFO,
      "    Reread data Points=" << rereadData->GetNumberOfPoints()
                                << ", Cells=" << rereadData->GetNumberOfCells());

    if (!comparisonSuccess)
    {
      vtkLog(ERROR, "  Timestep " << timeIndex << " - Data objects do not match");
      overallSuccess = false;
      continue;
    }
  }

  return overallSuccess;
}

//------------------------------------------------------------------------------
bool TestHDFWriterMixedCells(int argc, char* argv[])
{
  vtkLog(INFO, "Starting TestHDFWriterMixedCells...");

  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);

  vtkNew<vtkUnstructuredGrid> originalGrid;
  vtkNew<vtkPoints> points;

  // Tetra points (0-3)
  points->InsertNextPoint(0.0, 0.0, 0.0); // 0
  points->InsertNextPoint(1.0, 0.0, 0.0); // 1
  points->InsertNextPoint(0.0, 1.0, 0.0); // 2
  points->InsertNextPoint(0.0, 0.0, 1.0); // 3

  // Polyhedron points (4-11), cube
  points->InsertNextPoint(2.0, 0.0, 0.0); // 4
  points->InsertNextPoint(3.0, 0.0, 0.0); // 5
  points->InsertNextPoint(3.0, 1.0, 0.0); // 6
  points->InsertNextPoint(2.0, 1.0, 0.0); // 7
  points->InsertNextPoint(2.0, 0.0, 1.0); // 8
  points->InsertNextPoint(3.0, 0.0, 1.0); // 9
  points->InsertNextPoint(3.0, 1.0, 1.0); // 10
  points->InsertNextPoint(2.0, 1.0, 1.0); // 11

  // Hexahedron points (12-19)
  points->InsertNextPoint(4.0, 0.0, 0.0); // 12
  points->InsertNextPoint(5.0, 0.0, 0.0); // 13
  points->InsertNextPoint(5.0, 1.0, 0.0); // 14
  points->InsertNextPoint(4.0, 1.0, 0.0); // 15
  points->InsertNextPoint(4.0, 0.0, 1.0); // 16
  points->InsertNextPoint(5.0, 0.0, 1.0); // 17
  points->InsertNextPoint(5.0, 1.0, 1.0); // 18
  points->InsertNextPoint(4.0, 1.0, 1.0); // 19

  originalGrid->SetPoints(points);

  const vtkIdType tetraPts[4] = { 0, 1, 2, 3 };
  originalGrid->InsertNextCell(VTK_TETRA, 4, tetraPts);

  const vtkIdType polyPts[8] = { 4, 5, 6, 7, 8, 9, 10, 11 };
  // Face stream format: [n0, p0..., n1, p1..., ...]
  const vtkIdType polyFaces[] = {
    4, 4, 5, 6, 7,   // bottom
    4, 8, 9, 10, 11, // top
    4, 4, 5, 9, 8,   // front
    4, 5, 6, 10, 9,  // right
    4, 6, 7, 11, 10, // back
    4, 7, 4, 8, 11   // left
  };
  originalGrid->InsertNextCell(VTK_POLYHEDRON, 8, polyPts, 6, polyFaces);

  const vtkIdType hexPts[8] = { 12, 13, 14, 15, 16, 17, 18, 19 };
  originalGrid->InsertNextCell(VTK_HEXAHEDRON, 8, hexPts);

  std::string tempFilePath =
    std::string(testUtils->GetTempDirectory()) + "/TestHDFWriterMixedCells.vtkhdf";

  vtkNew<vtkHDFWriter> writer;
  writer->SetFileName(tempFilePath.c_str());
  writer->SetInputData(originalGrid);
  writer->SetCompressionLevel(4);
  writer->Write();

  vtkLog(INFO, "  Wrote temporary file to: " << tempFilePath);

  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(tempFilePath.c_str()))
  {
    vtkLog(ERROR, "vtkHDFReader cannot read temporary file: " << tempFilePath);
    return false;
  }

  reader->SetFileName(tempFilePath.c_str());
  reader->Update();

  vtkUnstructuredGrid* rereadGrid = vtkUnstructuredGrid::SafeDownCast(reader->GetOutput());
  if (!rereadGrid)
  {
    vtkLog(ERROR, "Failed to read mixed-cell file as vtkUnstructuredGrid");
    return false;
  }

  if (reader->GetNumberOfSteps() > 1)
  {
    vtkLog(ERROR, "Expected a single timestep, got " << reader->GetNumberOfSteps());
    return false;
  }

  const bool same = vtkTestUtilities::CompareDataObjects(originalGrid, rereadGrid);
  if (!same)
  {
    vtkLog(ERROR, "Mixed-cell unstructured grid mismatch");
  }

  vtkLog(INFO,
    "    Original data Points=" << originalGrid->GetNumberOfPoints()
                                << ", Cells=" << originalGrid->GetNumberOfCells());
  vtkLog(INFO,
    "    Reread data Points=" << rereadGrid->GetNumberOfPoints()
                              << ", Cells=" << rereadGrid->GetNumberOfCells());

  return same;
}

//------------------------------------------------------------------------------
int TestHDFWriterPolyhedra(int argc, char* argv[])
{
  bool success = true;
  success &= TestHDFWriterPolyhedraTemporal(argc, argv);
  success &= TestHDFWriterMixedCells(argc, argv);

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
