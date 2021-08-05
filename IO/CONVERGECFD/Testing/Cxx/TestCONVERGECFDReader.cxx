/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCONVERGECFDReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test of vtkCONVERGECFDReader

#include "vtkCONVERGECFDReader.h"
#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkDebugLeaks.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <cstdlib>

int TestCONVERGECFDReader(int argc, char* argv[])
{
  vtkNew<vtkCONVERGECFDReader> reader;

  // Read CONVERGE 3.1 file name
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/converge3.1-format.h5");
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  // Check on the structure of the output multiblock dataset

  // Read file name (CONVERGE 3.0 file).
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/post_5016_spray.h5");
  reader->GetCellDataArraySelection()->RemoveAllArrays();
  reader->GetParcelDataArraySelection()->RemoveAllArrays();
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  // Check on the structure of the output multiblock dataset
  auto mbds = reader->GetOutput();
  if (mbds->GetNumberOfBlocks() != 1)
  {
    vtkLog(ERROR, "Invalid number of streams in file.");
    return EXIT_FAILURE;
  }

  auto streamBlock = vtkMultiBlockDataSet::SafeDownCast(mbds->GetBlock(0));
  if (!streamBlock)
  {
    vtkLog(ERROR, "Stream block is not a vtkMultiBlockDataSet");
    return EXIT_FAILURE;
  }

  auto mesh = vtkUnstructuredGrid::SafeDownCast(streamBlock->GetBlock(0));
  auto surfaces = vtkMultiBlockDataSet::SafeDownCast(streamBlock->GetBlock(1));
  auto parcels = vtkPolyData::SafeDownCast(streamBlock->GetBlock(2));

  if (!mesh)
  {
    vtkLog(ERROR, "No mesh block found in file.");
    return EXIT_FAILURE;
  }

  if (mesh->GetNumberOfPoints() != 12242)
  {
    vtkLog(ERROR, "Incorrect number of points in mesh.");
    return EXIT_FAILURE;
  }

  if (mesh->GetNumberOfCells() != 3378)
  {
    vtkLog(ERROR, "Incorrect number of cells in mesh.");
    return EXIT_FAILURE;
  }

  std::vector<std::string> cellArrays = { "DENSITY", "EPS", "EQUIV_RATIO", "LAMBDA",
    "MASSFRAC_C7H16", "MASSFRAC_CO", "MASSFRAC_CO2", "MASSFRAC_H2", "MASSFRAC_H2O", "MASSFRAC_O2",
    "PRESSURE", "RANK", "REACT_RATIO", "SIE", "TEMPERATURE", "TKE", "VELOCITY", "VISC" };

  std::vector<std::string> pointArrays = { "FILM_FLAG", "RADIUS", "TEMP", "VELOCITY" };

  if (mesh->GetCellData()->GetNumberOfArrays() != static_cast<int>(cellArrays.size()))
  {
    vtkLog(ERROR, "Incorrect number of cell data arrays on mesh");
    return EXIT_FAILURE;
  }

  for (const auto& cellArrayName : cellArrays)
  {
    auto cellData = mesh->GetCellData();
    if (!cellData->HasArray(cellArrayName.c_str()))
    {
      vtkLog(ERROR, "Mesh is missing expected cell data array '" << cellArrayName << "'");
      return EXIT_FAILURE;
    }
  }

  if (!surfaces)
  {
    vtkLog(ERROR, "No surfaces block found in file.");
    return EXIT_FAILURE;
  }

  if (surfaces->GetNumberOfPoints() != 9085)
  {
    vtkLog(ERROR, "Incorrect number of points in surfaces.");
    return EXIT_FAILURE;
  }

  if (surfaces->GetNumberOfCells() != 9318)
  {
    vtkLog(ERROR, "Incorrect number of cells in surfaces.");
    return EXIT_FAILURE;
  }

  int numBlocks = surfaces->GetNumberOfBlocks();
  if (numBlocks != 7)
  {
    vtkLog(ERROR, "Incorrect number of surface blocks. Should be 7, got " << numBlocks);
    return EXIT_FAILURE;
  }
  int expectedNumPoints[] = { 5535, 837, 829, 510, 1374, 0, 0 };
  int expectedNumCells[] = { 6038, 770, 763, 461, 1286, 0, 0 };
  for (int i = 0; i < numBlocks; ++i)
  {
    vtkPolyData* surface = vtkPolyData::SafeDownCast(surfaces->GetBlock(i));
    if (!surface)
    {
      vtkLog(ERROR, "No polydata surface at block " << i);
      return EXIT_FAILURE;
    }
    if (surface->GetNumberOfPoints() != expectedNumPoints[i])
    {
      vtkLog(ERROR, "Incorrect number of points in surface block " << i);
      return EXIT_FAILURE;
    }
    if (surface->GetNumberOfCells() != expectedNumCells[i])
    {
      vtkLog(ERROR, "Incorrect number of cells in surface block " << i);
      return EXIT_FAILURE;
    }
    if (surface->GetCellData()->GetNumberOfArrays() != static_cast<int>(cellArrays.size()))
    {
      vtkLog(ERROR, "Incorrect number of cell data arrays on surface at block " << i);
      return EXIT_FAILURE;
    }
    for (const auto& cellArrayName : cellArrays)
    {
      auto cellData = surface->GetCellData();
      if (!cellData->HasArray(cellArrayName.c_str()))
      {
        vtkLog(ERROR,
          "surface " << i << " is missing expected cell data array '" << cellArrayName << "'");
        return EXIT_FAILURE;
      }
    }
  }

  if (!parcels)
  {
    vtkLog(ERROR, "No parcels block found in file.");
    return EXIT_FAILURE;
  }

  if (parcels->GetNumberOfPoints() != 185732)
  {
    vtkLog(ERROR, "Incorrect number of points in parcels.");
    return EXIT_FAILURE;
  }

  if (parcels->GetNumberOfCells() != 185732)
  {
    vtkLog(ERROR, "Incorrect number of cells in parcels.");
    return EXIT_FAILURE;
  }

  if (parcels->GetPointData()->GetNumberOfArrays() != static_cast<int>(pointArrays.size()))
  {
    vtkLog(ERROR, "Incorrect number of parcel data arrays");
    return EXIT_FAILURE;
  }
  for (const auto& pointArrayName : pointArrays)
  {
    auto pointData = parcels->GetPointData();
    if (!pointData->HasArray(pointArrayName.c_str()))
    {
      vtkLog(ERROR, "Parcels are missing expected point data array '" << pointArrayName << "'");
      return EXIT_FAILURE;
    }
  }

  // Test array selection
  auto cellArraySelection = reader->GetCellDataArraySelection();
  cellArraySelection->DisableArray("EPS");
  cellArraySelection->DisableArray("DENSITY");
  auto parcelArraySelection = reader->GetParcelDataArraySelection();
  parcelArraySelection->DisableArray("RADIUS");
  reader->Update();

  streamBlock = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput()->GetBlock(0));
  mesh = vtkUnstructuredGrid::SafeDownCast(streamBlock->GetBlock(0));
  surfaces = vtkMultiBlockDataSet::SafeDownCast(streamBlock->GetBlock(1));
  parcels = vtkPolyData::SafeDownCast(streamBlock->GetBlock(2));

  auto cellData = mesh->GetCellData();
  if (cellData->GetArray("EPS") != nullptr)
  {
    vtkLog(ERROR, "Data array 'EPS' should not have been read but is available");
    return EXIT_FAILURE;
  }
  if (cellData->GetArray("DENSITY") != nullptr)
  {
    vtkLog(ERROR, "Data array 'DENSITY' should not have been read but is available");
    return EXIT_FAILURE;
  }

  for (int i = 0; i < numBlocks; ++i)
  {
    vtkPolyData* surface = vtkPolyData::SafeDownCast(surfaces->GetBlock(i));
    if (!surface)
    {
      vtkLog(ERROR, "No polydata surface at block " << i);
      return EXIT_FAILURE;
    }

    cellData = surface->GetCellData();
    if (cellData->GetArray("EPS") != nullptr)
    {
      vtkLog(ERROR, "Data array 'EPS' should not have been read but is available");
      return EXIT_FAILURE;
    }
    if (cellData->GetArray("DENSITY") != nullptr)
    {
      vtkLog(ERROR, "Data array 'DENSITY' should not have been read but is available");
      return EXIT_FAILURE;
    }
  }

  auto parcelData = parcels->GetPointData();
  if (parcelData->GetArray("RADIUS") != nullptr)
  {
    vtkLog(ERROR, "Data array 'RADIUS' should not have been read but is available");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
