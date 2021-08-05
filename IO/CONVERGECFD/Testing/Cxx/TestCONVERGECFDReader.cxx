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

  {
    // Read CONVERGE 3.1 file name
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/converge3.1-format.h5");
    reader->SetFileName(fname);
    reader->Update();
    delete[] fname;

    // Check on the structure of the output multiblock dataset
    auto mbds = reader->GetOutput();
    if (mbds->GetNumberOfBlocks() != 3)
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
    if (!mesh)
    {
      vtkLog(ERROR, "No mesh block found in stream 0.");
      return EXIT_FAILURE;
    }

    if (mesh->GetNumberOfPoints() != 176840)
    {
      vtkLog(ERROR,
        "Incorrect number of points in mesh. Should be 176840, got " << mesh->GetNumberOfPoints());
      return EXIT_FAILURE;
    }

    if (mesh->GetNumberOfCells() != 22016)
    {
      vtkLog(ERROR,
        "Incorrect number of cells in mesh. Should be 22016, got " << mesh->GetNumberOfCells());
      return EXIT_FAILURE;
    }

    std::vector<std::string> cellArrays = { "ASPECT_RATIO", "EPS", "EQUIV_RATIO", "FACE_WARPAGE",
      "NON-ORTHOGONALITY", "NUM_CARTESIAN_NBRS", "NUM_INLAID_NBRS", "PRESSURE", "RANK", "SKEWNESS",
      "STRETCH_RATIO", "TEMP_SGS", "TEMPERATURE", "TKE", "VEL_SGS", "VELOCITY" };

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

    auto surfaces = vtkMultiBlockDataSet::SafeDownCast(streamBlock->GetBlock(1));
    if (!surfaces)
    {
      vtkLog(ERROR, "No surfaces block found in file.");
      return EXIT_FAILURE;
    }

    std::vector<std::string> pointArrays = { "FILM_FLAG", "RADIUS", "TEMP", "VELOCITY" };

    if (surfaces->GetNumberOfPoints() != 157957)
    {
      vtkLog(ERROR,
        "Incorrect number of points in surfaces. Should be 157957, but got "
          << surfaces->GetNumberOfPoints());
      return EXIT_FAILURE;
    }

    if (surfaces->GetNumberOfCells() != 186929)
    {
      vtkLog(ERROR,
        "Incorrect number of cells in surfaces. Should be 186929, but got "
          << surfaces->GetNumberOfCells());
      return EXIT_FAILURE;
    }

    int numBlocks = surfaces->GetNumberOfBlocks();
    if (numBlocks != 42)
    {
      vtkLog(ERROR, "Incorrect number of surface blocks. Should be 42, got " << numBlocks);
      return EXIT_FAILURE;
    }
    // Just check the first 5 surface block number of points and number of cells
    std::string expectedBlockNames[] = { "PISTON1", "LINER1", "HEAD1", "SPARK PLUG1",
      "SPARK PLUG ELECTRODE1" };
    int expectedNumPoints[] = { 10095, 4159, 20202, 858, 10 };
    int expectedNumCells[] = { 11763, 3994, 25182, 1080, 7 };
    for (int i = 0; i < sizeof(expectedNumPoints) / sizeof(int); ++i)
    {
      std::string blockName(
        surfaces->GetMetaData(static_cast<unsigned int>(i))->Get(vtkCompositeDataSet::NAME()));
      if (blockName != expectedBlockNames[i])
      {
        vtkLog(ERROR,
          "Surface data block expected to be " << expectedBlockNames[i] << ", but was "
                                               << blockName);
        return EXIT_FAILURE;
      }

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

    auto parcels = vtkMultiBlockDataSet::SafeDownCast(streamBlock->GetBlock(2));
    if (!parcels)
    {
      vtkLog(ERROR, "No parcels in dataset.");
      return EXIT_FAILURE;
    }

    auto liquidParcelData = vtkMultiBlockDataSet::SafeDownCast(parcels->GetBlock(0));
    if (!liquidParcelData)
    {
      vtkLog(ERROR, "No liquid parcel data");
      return EXIT_FAILURE;
    }

    auto liqparcel_1 = vtkPolyData::SafeDownCast(liquidParcelData->GetBlock(0));
    if (!liqparcel_1)
    {
      vtkLog(ERROR, "No liqparcel_1 parcel data");
      return EXIT_FAILURE;
    }

    if (liqparcel_1->GetNumberOfPoints() != 1581)
    {
      vtkLog(ERROR,
        "Incorrect number of points in parcels. Should be 1581, got "
          << liqparcel_1->GetNumberOfPoints());
      return EXIT_FAILURE;
    }

    if (liqparcel_1->GetNumberOfCells() != 1581)
    {
      vtkLog(ERROR,
        "Incorrect number of cells in parcels. Should be 1581, got "
          << liqparcel_1->GetNumberOfCells());
      return EXIT_FAILURE;
    }

    // Check parcel type
    std::string expectedBlockName = "LIQUID_PARCEL_DATA";
    std::string blockName(parcels->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME()));
    if (blockName != expectedBlockName)
    {
      vtkLog(ERROR, "Expected block name " << expectedBlockName << " but got " << blockName);
      return EXIT_FAILURE;
    }

    expectedBlockName = "LIQPARCEL_1";
    blockName = liquidParcelData->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME());
    if (blockName != expectedBlockName)
    {
      vtkLog(ERROR, "Expected block name " << expectedBlockName << " but got " << blockName);
      return EXIT_FAILURE;
    }

    // Check second stream
    streamBlock = vtkMultiBlockDataSet::SafeDownCast(mbds->GetBlock(1));
    if (!streamBlock)
    {
      vtkLog(ERROR, "Stream block is not a vtkMultiBlockDataSet");
      return EXIT_FAILURE;
    }

    mesh = vtkUnstructuredGrid::SafeDownCast(streamBlock->GetBlock(0));
    if (!mesh)
    {
      vtkLog(ERROR, "No mesh block found in stream 1.");
      return EXIT_FAILURE;
    }

    if (mesh->GetNumberOfPoints() != 178273)
    {
      vtkLog(ERROR,
        "Incorrect number of points in mesh. Should be 178273, got " << mesh->GetNumberOfPoints());
      return EXIT_FAILURE;
    }

    if (mesh->GetNumberOfCells() != 22369)
    {
      vtkLog(ERROR,
        "Incorrect number of cells in mesh. Should be 22369, got " << mesh->GetNumberOfCells());
      return EXIT_FAILURE;
    }

    surfaces = vtkMultiBlockDataSet::SafeDownCast(streamBlock->GetBlock(1));
    if (!surfaces)
    {
      vtkLog(ERROR, "No surfaces block found in file.");
      return EXIT_FAILURE;
    }

    if (surfaces->GetNumberOfPoints() != 159054)
    {
      vtkLog(ERROR,
        "Incorrect number of points in surfaces. Should be 159054, but got "
          << surfaces->GetNumberOfPoints());
      return EXIT_FAILURE;
    }

    if (surfaces->GetNumberOfCells() != 188046)
    {
      vtkLog(ERROR,
        "Incorrect number of cells in surfaces. Should be 188046, but got "
          << surfaces->GetNumberOfCells());
      return EXIT_FAILURE;
    }

    numBlocks = surfaces->GetNumberOfBlocks();
    if (numBlocks != 42)
    {
      vtkLog(ERROR, "Incorrect number of surface blocks. Should be 42, got " << numBlocks);
      return EXIT_FAILURE;
    }

    parcels = vtkMultiBlockDataSet::SafeDownCast(streamBlock->GetBlock(2));
    if (!parcels)
    {
      vtkLog(ERROR, "No parcels in dataset.");
      return EXIT_FAILURE;
    }

    liquidParcelData = vtkMultiBlockDataSet::SafeDownCast(parcels->GetBlock(0));
    if (!liquidParcelData)
    {
      vtkLog(ERROR, "No liquid parcel data");
      return EXIT_FAILURE;
    }

    liqparcel_1 = vtkPolyData::SafeDownCast(liquidParcelData->GetBlock(0));
    if (!liqparcel_1)
    {
      vtkLog(ERROR, "No liqparcel_1 parcel data");
      return EXIT_FAILURE;
    }

    if (liqparcel_1->GetNumberOfPoints() != 1798)
    {
      vtkLog(ERROR,
        "Incorrect number of points in parcels. Should be 1798, got "
          << liqparcel_1->GetNumberOfPoints());
      return EXIT_FAILURE;
    }

    if (liqparcel_1->GetNumberOfCells() != 1798)
    {
      vtkLog(ERROR,
        "Incorrect number of cells in parcels. Should be 1798, got "
          << liqparcel_1->GetNumberOfCells());
      return EXIT_FAILURE;
    }

    // Check third stream
    streamBlock = vtkMultiBlockDataSet::SafeDownCast(mbds->GetBlock(2));
    if (!streamBlock)
    {
      vtkLog(ERROR, "Stream block is not a vtkMultiBlockDataSet");
      return EXIT_FAILURE;
    }

    mesh = vtkUnstructuredGrid::SafeDownCast(streamBlock->GetBlock(0));
    if (!mesh)
    {
      vtkLog(ERROR, "No mesh block found in stream 1.");
      return EXIT_FAILURE;
    }

    if (mesh->GetNumberOfPoints() != 3620)
    {
      vtkLog(ERROR,
        "Incorrect number of points in mesh. Should be 3620, got " << mesh->GetNumberOfPoints());
      return EXIT_FAILURE;
    }

    if (mesh->GetNumberOfCells() != 124)
    {
      vtkLog(ERROR,
        "Incorrect number of cells in mesh. Should be 124, got " << mesh->GetNumberOfCells());
      return EXIT_FAILURE;
    }

    surfaces = vtkMultiBlockDataSet::SafeDownCast(streamBlock->GetBlock(1));
    if (!surfaces)
    {
      vtkLog(ERROR, "No surfaces block found in file.");
      return EXIT_FAILURE;
    }

    if (surfaces->GetNumberOfPoints() != 3801)
    {
      vtkLog(ERROR,
        "Incorrect number of points in surfaces. Should be 3801, but got "
          << surfaces->GetNumberOfPoints());
      return EXIT_FAILURE;
    }

    if (surfaces->GetNumberOfCells() != 4114)
    {
      vtkLog(ERROR,
        "Incorrect number of cells in surfaces. Should be 4114, but got "
          << surfaces->GetNumberOfCells());
      return EXIT_FAILURE;
    }

    // Ensure there are no parcels in the third stream
    if (streamBlock->GetNumberOfBlocks() != 2)
    {
      vtkLog(ERROR,
        "Number of blocks should be 2, but is " << streamBlock->GetNumberOfBlocks() << " instead");
      return EXIT_FAILURE;
    }
  }

  {
    // Read file name (CONVERGE 3.0 file).
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/post_5016_spray.h5");
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
  }

  return EXIT_SUCCESS;
}
