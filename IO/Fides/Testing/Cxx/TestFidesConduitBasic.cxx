// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFidesReader.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"

#if TEST_HAS_MPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include <conduit.hpp>
#include <conduit_cpp_to_c.hpp>

#include <cmath>
#include <fstream>
#include <string>
#include <vector>

struct ScopedFileCleanup
{
  std::string Path;
  ~ScopedFileCleanup() { std::remove(Path.c_str()); }
};

const char* JSON_MODEL = R"({
  "VTK_Cartesian_grid": {
    "data_sources": [ { "name": "source", "type": "conduit" } ],
    "coordinate_system": {
      "array": {
        "array_type": "uniform_point_coordinates",
        "dimensions": { "source": "variable_dimensions", "data_source": "source", "variable": "density" },
        "origin": { "source": "array", "values": [0.0, 0.0, 0.0] },
        "spacing": { "source": "array", "values": [0.1, 0.1, 0.1] }
      }
    },
    "cell_set": {
      "cell_set_type": "structured",
      "dimensions": { "source": "variable_dimensions", "data_source": "source", "variable": "density" }
    },
    "fields": [
      {
        "name": "density",
        "association": "points",
        "array": { "array_type": "basic", "data_source": "source", "variable": "density" }
      }
    ],
    "variables": {
      "density": { "shape_path": "variables/density/shape", "data_path": "variables/density/data" }
    }
  }
})";

int TestFidesConduitBasic(int argc, char* argv[])
{
#if TEST_HAS_MPI
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
#else
  vtkNew<vtkDummyController> controller;
#endif

  vtkMultiProcessController::SetGlobalController(controller);
  int rank = controller->GetLocalProcessId();

  if (argc < 2)
  {
    if (rank == 0)
    {
      std::cerr << "Usage: " << argv[0] << " <temp-output-directory>" << std::endl;
    }
    controller->Finalize();
    return EXIT_FAILURE;
  }

  std::string tempDir = argv[1];
  std::string jsonPath = tempDir + "/fides_conduit_test_rank_" + std::to_string(rank) + ".json";
  ScopedFileCleanup cleanup{ jsonPath };

  std::ofstream out(jsonPath);
  out << JSON_MODEL;
  out.close();

  // Create a localized Conduit node based on the MPI rank
  conduit::Node node;
  std::vector<int64_t> shape = { 2, 2, 2 }; // 2x2x2 points -> 1 single uniform cell
  double expectedValue = 1.0f + static_cast<double>(rank);
  std::vector<double> densityData(8, expectedValue); // 8 points of data

  node["variables/density/shape"].set(shape);
  node["variables/density/data"].set(densityData);
  node["variables/density/dtype"].set("double");

  vtkNew<vtkFidesReader> reader;
  reader->ParseDataModel(jsonPath);

  // Use the standard Conduit C++ to C-API cast
  conduit_node* c_node = conduit::c_node(&node);
  reader->SetDataSourceNode("source", c_node);

  reader->Update();

  auto pdsc = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
  if (!pdsc || pdsc->GetNumberOfPartitionedDataSets() == 0)
  {
    std::cerr << "Rank " << rank << ": Output is missing or not a collection!" << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }

  // Note: Because Conduit data is local in-memory, Fides returns local
  // partition indices. Regardless of the MPI rank, the single local
  // block on this process will be located at partition index 0 of the
  // 0th partitioned dataset (the "VTK_Cartesian_grid" group defined in
  // the JSON).
  auto pds = pdsc->GetPartitionedDataSet(0);
  if (!pds || pds->GetNumberOfPartitions() == 0)
  {
    std::cerr << "Rank " << rank << ": No partitions found in the local dataset!" << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }

  auto ds = pds->GetPartition(0);

  // Get the abstract array first
  auto abstractArray = ds->GetPointData()->GetArray("density");
  if (!abstractArray)
  {
    std::cerr << "Rank " << rank << ": The 'density' array is completely missing from PointData!"
              << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }

  // Cast to the generic vtkDataArray interface so the underlying type doesn't matter
  auto density = vtkDataArray::SafeDownCast(abstractArray);
  if (!density)
  {
    std::cerr << "Rank " << rank << ": The array exists, but cannot be cast to vtkDataArray!"
              << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }

  // Check a known tuple value
  double actualValue = density->GetTuple1(0);
  if (std::abs(actualValue - expectedValue) > 1e-6)
  {
    std::cerr << "Rank " << rank << ": Data validation failed! Expected " << expectedValue
              << ", got " << actualValue << std::endl;
    controller->Finalize();
    return EXIT_FAILURE;
  }

  controller->Finalize();
  return EXIT_SUCCESS;
}
