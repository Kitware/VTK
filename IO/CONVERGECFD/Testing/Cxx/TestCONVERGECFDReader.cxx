// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test of vtkCONVERGECFDReader

#include "vtkCONVERGECFDReader.h"
#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDebugLeaks.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkPartitionedDataSetCollection.h"
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

    // Check on the structure of the output partitioned dataset collection's assembly
    auto pdc = reader->GetOutput();
    auto assembly = pdc->GetDataAssembly();
    if (assembly->GetNumberOfChildren(0) != 3)
    {
      vtkLog(ERROR, "Invalid number of streams in file.");
      return EXIT_FAILURE;
    }

    int stream0NodeId = assembly->GetChild(0, 0);
    int meshNodeId = assembly->GetChild(stream0NodeId, 0);
    int meshId = assembly->GetDataSetIndices(meshNodeId, false)[0];
    auto mesh = vtkUnstructuredGrid::SafeDownCast(pdc->GetPartition(meshId, 0));
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

    int surfacesNodeId = assembly->GetChild(stream0NodeId, 1);

    std::vector<std::string> pointArrays = { "FILM_FLAG", "RADIUS", "TEMP", "VELOCITY" };

    int numBlocks = assembly->GetNumberOfChildren(surfacesNodeId);
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
    for (int i = 0; i < static_cast<int>(sizeof(expectedNumPoints) / sizeof(int)); ++i)
    {
      int surfaceNodeId = assembly->GetChild(surfacesNodeId, i);
      int surfaceId = assembly->GetDataSetIndices(surfaceNodeId, false)[0];
      std::string blockName(
        pdc->GetMetaData(static_cast<unsigned int>(surfaceId))->Get(vtkCompositeDataSet::NAME()));
      if (blockName != expectedBlockNames[i])
      {
        vtkLog(ERROR,
          "Surface data block expected to be " << expectedBlockNames[i] << ", but was "
                                               << blockName);
        return EXIT_FAILURE;
      }

      vtkPolyData* surface = vtkPolyData::SafeDownCast(pdc->GetPartition(surfaceId, 0));
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

    int parcelNodeId = assembly->GetChild(stream0NodeId, 2);
    int liquidParcelNodeId = assembly->GetChild(parcelNodeId, 0);
    int liqParcel1NodeId = assembly->GetChild(liquidParcelNodeId, 0);
    int liqParcel1Id = assembly->GetDataSetIndices(liqParcel1NodeId, false)[0];

    auto liqparcel_1 = vtkPolyData::SafeDownCast(pdc->GetPartition(liqParcel1Id, 0));
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
    std::string expectedBlockName("LIQPARCEL_1");
    std::string blockName(pdc->GetMetaData(liqParcel1Id)->Get(vtkCompositeDataSet::NAME()));
    if (blockName != expectedBlockName)
    {
      vtkLog(
        ERROR, "Expected block name '" << expectedBlockName << "' but got '" << blockName << "'");
      return EXIT_FAILURE;
    }

    // Check second stream
    int stream1NodeId = assembly->GetChild(0, 1);
    meshNodeId = assembly->GetChild(stream1NodeId, 0);
    meshId = assembly->GetDataSetIndices(meshNodeId, false)[0];
    mesh = vtkUnstructuredGrid::SafeDownCast(pdc->GetPartition(meshId, 0));
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

    surfacesNodeId = assembly->GetChild(stream1NodeId, 1);

    numBlocks = assembly->GetNumberOfChildren(surfacesNodeId);
    if (numBlocks != 42)
    {
      vtkLog(ERROR, "Incorrect number of surface blocks. Should be 42, got " << numBlocks);
      return EXIT_FAILURE;
    }

    parcelNodeId = assembly->GetChild(stream1NodeId, 2);
    liquidParcelNodeId = assembly->GetChild(parcelNodeId, 0);
    liqParcel1NodeId = assembly->GetChild(liquidParcelNodeId, 0);
    liqParcel1Id = assembly->GetDataSetIndices(liqParcel1NodeId, false)[0];

    liqparcel_1 = vtkPolyData::SafeDownCast(pdc->GetPartition(liqParcel1Id, 0));
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
    int stream2NodeId = assembly->GetChild(0, 2);
    meshNodeId = assembly->GetChild(stream2NodeId, 0);
    meshId = assembly->GetDataSetIndices(meshNodeId, false)[0];
    mesh = vtkUnstructuredGrid::SafeDownCast(pdc->GetPartition(meshId, 0));
    if (!mesh)
    {
      vtkLog(ERROR, "No mesh block found in stream 2.");
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

    // Ensure there are no parcels in the third stream
    if (assembly->GetNumberOfChildren(stream2NodeId) != 2)
    {
      vtkLog(ERROR,
        "Number of children should be 2, but is " << assembly->GetNumberOfChildren(stream2NodeId)
                                                  << " instead");
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
    auto pdc = reader->GetOutput();
    auto assembly = pdc->GetDataAssembly();
    if (assembly->GetNumberOfChildren(0) != 1)
    {
      vtkLog(ERROR, "Invalid number of streams in file.");
      return EXIT_FAILURE;
    }

    int stream0NodeId = assembly->GetChild(0, 0);
    int meshNodeId = assembly->GetChild(stream0NodeId, 0);
    int meshId = assembly->GetDataSetIndices(meshNodeId, false)[0];
    auto mesh = vtkUnstructuredGrid::SafeDownCast(pdc->GetPartition(meshId, 0));
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

    int surfacesNodeId = assembly->GetChild(stream0NodeId, 1);
    int numBlocks = assembly->GetNumberOfChildren(surfacesNodeId);
    if (numBlocks != 7)
    {
      vtkLog(ERROR, "Incorrect number of surface blocks. Should be 7, got " << numBlocks);
      return EXIT_FAILURE;
    }
    int expectedNumPoints[] = { 5535, 837, 829, 510, 1374, 0, 0 };
    int expectedNumCells[] = { 6038, 770, 763, 461, 1286, 0, 0 };
    for (int i = 0; i < numBlocks; ++i)
    {
      int surfaceNodeId = assembly->GetChild(surfacesNodeId, i);
      int surfaceId = assembly->GetDataSetIndices(surfaceNodeId, false)[0];
      vtkPolyData* surface = vtkPolyData::SafeDownCast(pdc->GetPartition(surfaceId, 0));
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

    int parcelsNodeId = assembly->GetChild(stream0NodeId, 2);
    int parcelsId = assembly->GetDataSetIndices(parcelsNodeId, false)[0];
    auto parcels = vtkPolyData::SafeDownCast(pdc->GetPartition(parcelsId, 0));
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

    pdc = reader->GetOutput();
    assembly = pdc->GetDataAssembly();
    mesh = vtkUnstructuredGrid::SafeDownCast(pdc->GetPartition(meshId, 0));
    parcels = vtkPolyData::SafeDownCast(pdc->GetPartition(parcelsId, 0));

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
      int surfaceNodeId = assembly->GetChild(surfacesNodeId, i);
      int surfaceId = assembly->GetDataSetIndices(surfaceNodeId, false)[0];
      vtkPolyData* surface = vtkPolyData::SafeDownCast(pdc->GetPartition(surfaceId, 0));
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
