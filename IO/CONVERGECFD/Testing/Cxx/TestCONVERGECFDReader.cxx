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
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <cstdlib>

int TestCONVERGECFDReader(int argc, char* argv[])
{
  // Read file name.
  const char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/post_5016_spray.h5");

  vtkNew<vtkCONVERGECFDReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  // Check on the structure of the output multiblock dataset
  auto mbds = reader->GetOutput();
  if (mbds->GetNumberOfBlocks() != 1)
  {
    std::cerr << "Invalid number of streams in file." << std::endl;
    return EXIT_FAILURE;
  }

  auto streamBlock = vtkMultiBlockDataSet::SafeDownCast(mbds->GetBlock(0));
  if (!streamBlock)
  {
    std::cerr << "Stream block is not a vtkMultiBlockDataSet" << std::endl;
    return EXIT_FAILURE;
  }

  auto mesh = vtkUnstructuredGrid::SafeDownCast(streamBlock->GetBlock(0));
  auto surfaces = vtkMultiBlockDataSet::SafeDownCast(streamBlock->GetBlock(1));
  auto parcels = vtkPolyData::SafeDownCast(streamBlock->GetBlock(2));

  if (!mesh)
  {
    std::cerr << "No mesh block found in file." << std::endl;
    return EXIT_FAILURE;
  }

  if (mesh->GetNumberOfPoints() != 12242)
  {
    std::cerr << "Incorrect number of points in mesh." << std::endl;
    return EXIT_FAILURE;
  }

  if (mesh->GetNumberOfCells() != 3378)
  {
    std::cerr << "Incorrect number of cells in mesh." << std::endl;
    return EXIT_FAILURE;
  }

  std::vector<std::string> cellArrays = { "DENSITY", "EPS", "EQUIV_RATIO", "LAMBDA",
    "MASSFRAC_C7H16", "MASSFRAC_CO", "MASSFRAC_CO2", "MASSFRAC_H2", "MASSFRAC_H2O", "MASSFRAC_O2",
    "PRESSURE", "RANK", "REACT_RATIO", "SIE", "TEMPERATURE", "TKE", "VELOCITY", "VISC" };

  std::vector<std::string> pointArrays = { "FILM_FLAG", "RADIUS", "TEMP", "VELOCITY" };

  if (mesh->GetCellData()->GetNumberOfArrays() != static_cast<int>(cellArrays.size()))
  {
    std::cerr << "Incorrect number of cell data arrays on mesh" << std::endl;
    return EXIT_FAILURE;
  }

  for (const auto& cellArrayName : cellArrays)
  {
    auto cellData = mesh->GetCellData();
    if (!cellData->HasArray(cellArrayName.c_str()))
    {
      std::cerr << "Mesh is missing expected cell data array '" << cellArrayName << "'"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (!surfaces)
  {
    std::cerr << "No surfaces block found in file." << std::endl;
    return EXIT_FAILURE;
  }

  if (surfaces->GetNumberOfPoints() != 9085)
  {
    std::cerr << "Incorrect number of points in surfaces." << std::endl;
    return EXIT_FAILURE;
  }

  if (surfaces->GetNumberOfCells() != 9318)
  {
    std::cerr << "Incorrect number of cells in surfaces." << std::endl;
    return EXIT_FAILURE;
  }

  int numBlocks = surfaces->GetNumberOfBlocks();
  if (numBlocks != 7)
  {
    std::cerr << "Incorrect number of surface blocks. Should be 7, got " << numBlocks << std::endl;
    return EXIT_FAILURE;
  }
  int expectedNumPoints[] = { 5535, 837, 829, 510, 1374, 0, 0 };
  int expectedNumCells[] = { 6038, 770, 763, 461, 1286, 0, 0 };
  for (int i = 0; i < numBlocks; ++i)
  {
    vtkPolyData* surface = vtkPolyData::SafeDownCast(surfaces->GetBlock(i));
    if (!surface)
    {
      std::cerr << "No polydata surface at block " << i << std::endl;
      return EXIT_FAILURE;
    }
    if (surface->GetNumberOfPoints() != expectedNumPoints[i])
    {
      std::cerr << "Incorrect number of points in surface block " << i << std::endl;
      return EXIT_FAILURE;
    }
    if (surface->GetNumberOfCells() != expectedNumCells[i])
    {
      std::cerr << "Incorrect number of cells in surface block " << i << std::endl;
      return EXIT_FAILURE;
    }
    if (surface->GetCellData()->GetNumberOfArrays() != static_cast<int>(cellArrays.size()))
    {
      std::cerr << "Incorrect number of cell data arrays on surface at block " << i << std::endl;
      return EXIT_FAILURE;
    }
    for (auto cellArrayName : cellArrays)
    {
      auto cellData = surface->GetCellData();
      if (!cellData->HasArray(cellArrayName.c_str()))
      {
        std::cerr << "surface " << i << " is missing expected cell data array '" << cellArrayName
                  << "'" << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  if (!parcels)
  {
    std::cerr << "No parcels block found in file." << std::endl;
    return EXIT_FAILURE;
  }

  if (parcels->GetNumberOfPoints() != 185732)
  {
    std::cerr << "Incorrect number of points in parcels." << std::endl;
    return EXIT_FAILURE;
  }

  if (parcels->GetNumberOfCells() != 185732)
  {
    std::cerr << "Incorrect number of cells in parcels." << std::endl;
    return EXIT_FAILURE;
  }

  if (parcels->GetPointData()->GetNumberOfArrays() != static_cast<int>(pointArrays.size()))
  {
    std::cerr << "Incorrect number of parcel data arrays" << std::endl;
    return EXIT_FAILURE;
  }
  for (const auto& pointArrayName : pointArrays)
  {
    auto pointData = parcels->GetPointData();
    if (!pointData->HasArray(pointArrayName.c_str()))
    {
      std::cerr << "Parcels are missing expected point data array '" << pointArrayName << "'"
                << std::endl;
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
    std::cerr << "Data array 'EPS' should not have been read but is available" << std::endl;
    return EXIT_FAILURE;
  }
  if (cellData->GetArray("DENSITY") != nullptr)
  {
    std::cerr << "Data array 'DENSITY' should not have been read but is available" << std::endl;
    return EXIT_FAILURE;
  }

  for (int i = 0; i < numBlocks; ++i)
  {
    vtkPolyData* surface = vtkPolyData::SafeDownCast(surfaces->GetBlock(i));
    if (!surface)
    {
      std::cerr << "No polydata surface at block " << i << std::endl;
      return EXIT_FAILURE;
    }

    cellData = surface->GetCellData();
    if (cellData->GetArray("EPS") != nullptr)
    {
      std::cerr << "Data array 'EPS' should not have been read but is available" << std::endl;
      return EXIT_FAILURE;
    }
    if (cellData->GetArray("DENSITY") != nullptr)
    {
      std::cerr << "Data array 'DENSITY' should not have been read but is available" << std::endl;
      return EXIT_FAILURE;
    }
  }

  auto parcelData = parcels->GetPointData();
  if (parcelData->GetArray("RADIUS") != nullptr)
  {
    std::cerr << "Data array 'RADIUS' should not have been read but is available" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
