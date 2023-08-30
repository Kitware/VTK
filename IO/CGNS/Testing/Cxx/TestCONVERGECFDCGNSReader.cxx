// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCONVERGECFDCGNSReader.h"

#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

namespace
{
float TOLF = 0.0001f;
double TOLD = 0.0001;
}

int TestCONVERGECFDCGNSReader(int argc, char* argv[])
{
  // Dataset contains two simple structured zones with UserDefinedData_t nodes
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/EngineSector.cgns");
  vtkNew<vtkCONVERGECFDCGNSReader> reader;
  reader->SetFileName(fname);
  delete[] fname;
  reader->Update();

  vtkPartitionedDataSetCollection* dataset = reader->GetOutput();

  if (!dataset)
  {
    std::cerr << "Empty reader output!" << std::endl;
    return EXIT_FAILURE;
  }

  if (dataset->GetNumberOfPartitionedDataSets() != 3)
  {
    std::cerr << "Dataset should have 3 partitioned datasets but got "
              << dataset->GetNumberOfPartitionedDataSets() << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Check that the CGNS reader properly created the mesh and boundaries
  vtkUnstructuredGrid* mesh = vtkUnstructuredGrid::SafeDownCast(dataset->GetPartition(0, 0));

  if (!mesh)
  {
    std::cerr << "Mesh is missing." << std::endl;
    return EXIT_FAILURE;
  }

  if (mesh->GetNumberOfPoints() != 7556 && mesh->GetNumberOfCells() != 1956)
  {
    std::cerr << "Mesh should have 7556 points and 1956 cells, but found "
              << mesh->GetNumberOfPoints() << " and " << mesh->GetNumberOfCells() << "."
              << std::endl;
    return EXIT_FAILURE;
  }

  vtkUnstructuredGrid* boundary = vtkUnstructuredGrid::SafeDownCast(dataset->GetPartition(1, 0));

  if (!boundary)
  {
    std::cerr << "Boundary is missing." << std::endl;
    return EXIT_FAILURE;
  }

  if (boundary->GetNumberOfPoints() != 7556 && boundary->GetNumberOfCells() != 6209)
  {
    std::cerr << "Boundary should have 7556 points and 6209 cells, but found "
              << boundary->GetNumberOfPoints() << " and " << boundary->GetNumberOfCells() << "."
              << std::endl;
    return EXIT_FAILURE;
  }

  // Check parcels
  vtkPolyData* parcels = vtkPolyData::SafeDownCast(dataset->GetPartition(2, 0));

  if (!parcels)
  {
    std::cerr << "Parcels are missing." << std::endl;
    return EXIT_FAILURE;
  }

  if (parcels->GetNumberOfPoints() != 15 && parcels->GetNumberOfCells() != 15)
  {
    std::cerr << "Parcels should have 15 points and 15 cells, but found "
              << parcels->GetNumberOfPoints() << " and " << parcels->GetNumberOfCells() << "."
              << std::endl;
    return EXIT_FAILURE;
  }

  // Check scalar array
  vtkFloatArray* array = vtkFloatArray::SafeDownCast(parcels->GetPointData()->GetArray("TEMP"));

  if (!array)
  {
    std::cerr << "Missing 'TEMP' array from parcel data." << std::endl;
    return EXIT_FAILURE;
  }

  if (!vtkMathUtilities::FuzzyCompare(array->GetValue(2), 643.982f, ::TOLF))
  {
    std::cerr << "Expected value equal to 643.982, but got " << array->GetValue(2) << "."
              << std::endl;
    return EXIT_FAILURE;
  }

  // Check vector array
  array = vtkFloatArray::SafeDownCast(parcels->GetPointData()->GetArray("VELOCITY"));

  if (!array)
  {
    std::cerr << "Missing 'VELOCITY' array from parcel data." << std::endl;
    return EXIT_FAILURE;
  }

  double* values = array->GetTuple3(7);

  if (!vtkMathUtilities::FuzzyCompare(values[0], 2.59626, ::TOLD) ||
    !vtkMathUtilities::FuzzyCompare(values[1], 0.0353042, ::TOLD) ||
    !vtkMathUtilities::FuzzyCompare(values[2], -7.99531, ::TOLD))
  {
    std::cerr << "Wrong value(s). Expected (2.59626, 0.0353042, -7.99531) but got (" << values[0]
              << ", " << values[1] << ", " << values[2] << ")." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
