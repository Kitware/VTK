// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHDFReader.h"
#include "vtkHDFWriter.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
bool TestTransientData(const std::string& tempDir, const std::string& dataRoot,
  const std::vector<std::string>& baseNames)
{
  for (const auto& baseName : baseNames)
  {
    std::cout << "Writing " << baseName << std::endl;

    // Open original transient HDF UG data
    const std::string basePath = dataRoot + "/Data/" + baseName;
    vtkNew<vtkHDFReader> baseHDFReader;
    baseHDFReader->SetFileName(basePath.c_str());

    // Write the data to a file using the vtkHDFWriter
    vtkNew<vtkHDFWriter> HDFWriter;
    HDFWriter->SetInputConnection(baseHDFReader->GetOutputPort());
    std::string tempPath = tempDir + "/HDFWriter_" + baseName + ".vtkhdf";
    HDFWriter->SetFileName(tempPath.c_str());
    HDFWriter->SetWriteAllTimeSteps(true);
    HDFWriter->SetChunkSize(30);
    HDFWriter->Write();

    // Read the data just written
    vtkNew<vtkHDFReader> HDFReader;
    if (!HDFReader->CanReadFile(tempPath.c_str()))
    {
      std::cerr << "vtkHDFReader can not read file: " << tempPath << std::endl;
      return false;
    }
    HDFReader->SetFileName(tempPath.c_str());
    HDFReader->Update();

    // Read the original data from the beginning
    vtkNew<vtkHDFReader> HDFReaderBaseline;
    HDFReaderBaseline->SetFileName(basePath.c_str());
    HDFReaderBaseline->Update();

    // Make sure both have the same number of timesteps
    int totalTimeStepsXML = HDFReaderBaseline->GetNumberOfSteps();
    int totalTimeStepsHDF = HDFReader->GetNumberOfSteps();
    if (totalTimeStepsXML != totalTimeStepsHDF)
    {
      std::cerr << "total time steps in both HDF files do not match: " << totalTimeStepsHDF
                << " instead of " << totalTimeStepsXML << std::endl;
      return false;
    }

    // Compare the data at each timestep from both readers
    for (int i = 0; i < totalTimeStepsXML; i++)
    {
      std::cout << "Comparing timestep " << i << std::endl;
      HDFReaderBaseline->SetStep(i);
      HDFReaderBaseline->Update();

      HDFReader->SetStep(i);
      HDFReader->Update();

      // Time values must be the same
      if (HDFReader->GetTimeValue() != HDFReaderBaseline->GetTimeValue())
      {
        std::cerr << "timestep value does not match : " << HDFReader->GetTimeValue()
                  << " instead of " << HDFReaderBaseline->GetTimeValue() << std::endl;
        return false;
      }

      // Data is either PolyData or UG
      vtkPolyData* basepolyData = vtkPolyData::SafeDownCast(HDFReaderBaseline->GetOutput());
      vtkPolyData* hdfpolyData = vtkPolyData::SafeDownCast(HDFReader->GetOutput());
      if (basepolyData && hdfpolyData)
      {
        if (!vtkTestUtilities::CompareDataObjects(hdfpolyData, basepolyData))
        {
          std::cerr << "vtkDataset do not match" << std::endl;
          return false;
        }
      }
      else
      {
        vtkUnstructuredGrid* baseData =
          vtkUnstructuredGrid::SafeDownCast(HDFReaderBaseline->GetOutput());
        vtkUnstructuredGrid* hdfData = vtkUnstructuredGrid::SafeDownCast(HDFReader->GetOutput());
        if (!vtkTestUtilities::CompareDataObjects(hdfData, baseData))
        {
          std::cerr << "vtkDataset do not match" << std::endl;
          return false;
        }
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
int TestHDFWriterTransient(int argc, char* argv[])
{
  // Get temporary testing directory
  char* tempDirCStr =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tempDir{ tempDirCStr };
  delete[] tempDirCStr;

  // Get data directory
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified." << std::endl;
    return EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();

  // Run tests : read data, write it, read the written data and compare to the original
  std::vector<std::string> baseNames = { "transient_sphere.hdf", "transient_cube.hdf",
    "test_transient_poly_data.hdf", "transient_harmonics.hdf" };
  return TestTransientData(tempDir, dataRoot, baseNames) ? EXIT_SUCCESS : EXIT_FAILURE;
}
