// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataObject.h"
#include "vtkHDFReader.h"

#include "vtkAppendDataSets.h"
#include "vtkAppendFilter.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkSphereSource.h"
#include "vtkStringFormatter.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLPartitionedDataSetCollectionReader.h"
#include "vtkXMLPartitionedDataSetReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLUniformGridAMRReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace
{
constexpr double CHECK_TOLERANCE = 1e-3;
constexpr int EXPECTED_SHAPE_AT_TIMESTEP[3][2] = { { 3, 1 }, { 1, 2 }, { 2, 2 } };

int TestUGTemporal(const std::string& dataRoot);
int TestImageDataTemporal(const std::string& dataRoot);
int TestPolyDataTemporal(const std::string& dataRoot);
int TestPartitionedPolyDataTemporalWithOffset(const std::string& dataRoot);
int TestPartitionedUGTemporal(const std::string& dataRoot);
int TestUGTemporalPolyhedron(const std::string& dataRoot);
int TestPartitionedPolyDataTemporal(const std::string& dataRoot);
int TestPolyDataTemporalFieldData(const std::string& dataRoot);
int TestHyperTreeGridTemporal(const std::string& dataRoot, unsigned int depthLimit);
int TestHyperTreeGridPartitionedTemporal(const std::string& dataRoot);
int TestOverlappingAMRTemporal(const std::string& dataRoot);
int TestOverlappingAMRTemporalLegacy(const std::string& dataRoot);
int TestPartialArrayWithCompositeDataset(const std::string& dataRoot);
int TestPDCPolyDataUGTemporal(const std::string& dataRoot);
int TestMBPolyDataUGTemporal(const std::string& dataRoot);
}

//------------------------------------------------------------------------------
int TestHDFReaderTemporal(int argc, char* argv[])
{
  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();
  int res = ::TestUGTemporal(dataRoot);
  res |= ::TestImageDataTemporal(dataRoot);
  res |= ::TestPolyDataTemporal(dataRoot);
  res |= ::TestPartitionedPolyDataTemporalWithOffset(dataRoot);
  res |= ::TestPartitionedUGTemporal(dataRoot);
  res |= ::TestUGTemporalPolyhedron(dataRoot);
  res |= ::TestPartitionedPolyDataTemporal(dataRoot);
  res |= ::TestPolyDataTemporalFieldData(dataRoot);
  res |= ::TestHyperTreeGridTemporal(dataRoot, 3);
  res |= ::TestHyperTreeGridTemporal(dataRoot, 1);
  res |= ::TestHyperTreeGridPartitionedTemporal(dataRoot);
  res |= ::TestOverlappingAMRTemporalLegacy(dataRoot);
  res |= ::TestOverlappingAMRTemporal(dataRoot);
  res |= ::TestPartialArrayWithCompositeDataset(dataRoot);
  res |= ::TestPDCPolyDataUGTemporal(dataRoot);
  res |= ::TestMBPolyDataUGTemporal(dataRoot);

  return res;
}

namespace
{
//------------------------------------------------------------------------------
int TestUGTemporal(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/temporal_ug.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  if (reader->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  std::array<vtkMTimeType, 2> meshMTime{ 0, 0 };

  for (std::size_t i = 0; i < 11; ++i)
  {
    // Loop over to make sure cache can be rewritten
    std::size_t iStep = i % 10;

    reader->SetStep(iStep);
    reader->Update();
    vtkDataSet* data = vtkDataSet::SafeDownCast(reader->GetOutputDataObject(0));

    vtkNew<vtkXMLUnstructuredGridReader> refReader;
    refReader->SetFileName(
      (dataRoot + "/Data/vtkHDF/temporal_ug_" + vtk::to_string(iStep) + ".vtu").c_str());
    refReader->Update();
    vtkDataSet* refData = vtkDataSet::SafeDownCast(refReader->GetOutputDataObject(0));

    // Local Time Checks
    double readerTime = reader->GetTimeValue();
    if (!vtkMathUtilities::FuzzyCompare(
          readerTime, static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "Property: TimeValue is wrong: " << readerTime
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    double dataTime = data->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
    if (readerTime != dataTime)
    {
      std::cerr << "Output DATA_TIME_STEP is wrong: " << dataTime << " != " << readerTime
                << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = data->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cerr << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkTestUtilities::CompareDataObjects(data, refData))
    {
      std::cerr << "Unstructured grids are not the same for timestep " << iStep << std::endl;
      return EXIT_FAILURE;
    }

    meshMTime[1] = meshMTime[0];
    meshMTime[0] = data->GetMeshMTime();
    if (iStep > 0 && iStep < 7) // timestep 8, 9 and 10 change the mesh
    {
      if (meshMTime[0] != meshMTime[1])
      {
        std::cerr << "MTime: Unstructured Grid Failed MeshMTime check - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << std::endl;
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestPartitionedUGTemporal(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/temporal_partitioned_ug.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  if (reader->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  std::array<vtkMTimeType, 2> meshMTime{ 0, 0 };
  for (std::size_t iStep = 0; iStep < 10; ++iStep)
  {
    // Open data at right time
    reader->SetStep(iStep);
    reader->Update();
    vtkPartitionedDataSet* data =
      vtkPartitionedDataSet::SafeDownCast(reader->GetOutputDataObject(0));

    // Reference Geometry
    vtkNew<vtkXMLPartitionedDataSetReader> refReader;
    refReader->SetFileName(
      (dataRoot + "/Data/vtkHDF/temporal_partitioned_ug_" + vtk::to_string(iStep) + ".vtpd")
        .c_str());
    refReader->Update();
    vtkPartitionedDataSet* refData =
      vtkPartitionedDataSet::SafeDownCast(refReader->GetOutputDataObject(0));

    // Local Time Checks
    if (!vtkMathUtilities::FuzzyCompare(
          reader->GetTimeValue(), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "Property: TimeValue is wrong: " << reader->GetTimeValue()
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = data->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cerr << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkTestUtilities::CompareDataObjects(data, refData))
    {
      std::cerr << "Unstructured grids are not the same for timestep " << iStep << std::endl;
      return EXIT_FAILURE;
    }

    int maxMeshMTimePartition = -1;
    for (unsigned int i = 0; i < data->GetNumberOfPartitions(); i++)
    {
      auto dataPartition = vtkUnstructuredGrid::SafeDownCast(data->GetPartition(i));
      maxMeshMTimePartition =
        std::max(static_cast<int>(vtkUnstructuredGrid::SafeDownCast(dataPartition)->GetMeshMTime()),
          maxMeshMTimePartition);
    }

    meshMTime[1] = meshMTime[0];
    meshMTime[0] = maxMeshMTimePartition;
    if (iStep > 0 && iStep < 10)
    {
      if (meshMTime[0] != meshMTime[1])
      {
        std::cerr << "MTime: Failed MeshMTime check - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << " at timestep :" << iStep << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestUGTemporalPolyhedron(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/polyhedron_temporal.vtkhdf").c_str());
  reader->Update();

  for (int step = 0; step <= 1; step++)
  {
    // Open data at right time
    reader->SetStep(step);
    reader->Update();
    vtkUnstructuredGrid* readData =
      vtkUnstructuredGrid::SafeDownCast(reader->GetOutputDataObject(0));

    std::string vtuFile =
      dataRoot + "/Data/vtkHDF/polyhedron_temporal_" + vtk::to_string(step) + ".vtu";
    vtkNew<vtkXMLUnstructuredGridReader> readerXML;
    readerXML->SetFileName(vtuFile.c_str());
    readerXML->Update();
    vtkUnstructuredGrid* readDataXML = vtkUnstructuredGrid::SafeDownCast(readerXML->GetOutput());
    vtkNew<vtkFieldData> fd;
    readDataXML->SetFieldData(fd);

    if (!vtkTestUtilities::CompareDataObjects(readDataXML, readData))
    {
      std::cerr << "Unstructured grids with polyhedrons are not the same for timestep " << step
                << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestImageDataTemporal(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/temporal_image.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  if (reader->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  for (std::size_t iStep = 0; iStep < 10; ++iStep)
  {
    // Open data at right time
    reader->SetStep(iStep);
    reader->Update();
    vtkDataObject* data = reader->GetOutputDataObject(0);

    // open reference at right time
    std::string vtiFile =
      dataRoot + "/Data/vtkHDF/temporal_image_" + vtk::to_string(iStep) + ".vti";
    vtkNew<vtkXMLImageDataReader> readerXML;
    readerXML->SetFileName(vtiFile.c_str());
    readerXML->Update();
    vtkDataObject* refData = readerXML->GetOutputDataObject(0);

    // Remove TimeValue added by XML reader
    refData->GetFieldData()->RemoveArray("TimeValue");

    // Local Time Checks
    if (!vtkMathUtilities::FuzzyCompare(
          reader->GetTimeValue(), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "Property: Time Value is wrong: " << reader->GetTimeValue()
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = data->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cerr << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkTestUtilities::CompareDataObjects(refData, data))
    {
      std::cerr << "Image data are not the same for timestep " << iStep << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestPolyDataTemporal(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/temporal_polydata.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  if (reader->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  std::array<vtkMTimeType, 2> meshMTime{ 0, 0 };

  for (std::size_t i = 0; i < 11; ++i)
  {
    // Loop over to make sure cache can be rewritten
    std::size_t iStep = i % 10;

    reader->SetStep(iStep);
    reader->Update();
    vtkDataSet* data = vtkDataSet::SafeDownCast(reader->GetOutputDataObject(0));

    vtkNew<vtkXMLPolyDataReader> refReader;
    refReader->SetFileName(
      (dataRoot + "/Data/vtkHDF/temporal_polydata_" + vtk::to_string(iStep) + ".vtp").c_str());
    refReader->Update();
    vtkDataSet* refData = vtkDataSet::SafeDownCast(refReader->GetOutputDataObject(0));

    // Local Time Checks
    double readerTime = reader->GetTimeValue();
    if (!vtkMathUtilities::FuzzyCompare(
          readerTime, static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "Property: TimeValue is wrong: " << readerTime
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    double dataTime = data->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
    if (readerTime != dataTime)
    {
      std::cerr << "Output DATA_TIME_STEP is wrong: " << dataTime << " != " << readerTime
                << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = data->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cerr << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkTestUtilities::CompareDataObjects(data, refData))
    {
      std::cerr << "Unstructured grids are not the same for timestep " << iStep << std::endl;
      return EXIT_FAILURE;
    }

    meshMTime[1] = meshMTime[0];
    meshMTime[0] = data->GetMeshMTime();
    if (iStep > 0 && iStep < 7) // timestep 8, 9 and 10 change the mesh
    {
      if (meshMTime[0] != meshMTime[1])
      {
        std::cerr << "MTime: Unstructured Grid Failed MeshMTime check - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << std::endl;
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestPartitionedPolyDataTemporal(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(
    (dataRoot + "/Data/vtkHDF/temporal_partitioned_polydata_cache.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  if (reader->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  std::array<int, 2> meshMTime{ 0, 0 };
  for (std::size_t iStep = 0; iStep < 10; ++iStep)
  {
    // Open data at right time
    reader->SetStep(iStep);
    reader->Update();
    vtkPartitionedDataSet* data =
      vtkPartitionedDataSet::SafeDownCast(reader->GetOutputDataObject(0));

    // Reference Geometry
    vtkNew<vtkXMLPartitionedDataSetReader> refReader;
    refReader->SetFileName((dataRoot + "/Data/vtkHDF/temporal_partitioned_polydata_cache_" +
      vtk::to_string(iStep) + ".vtpd")
                             .c_str());
    refReader->Update();
    vtkDataObject* refData = refReader->GetOutputDataObject(0);

    if (!vtkTestUtilities::CompareDataObjects(data, refData))
    {
      std::cerr << "Partitioned polydata are not the same for timestep " << iStep << std::endl;
      return EXIT_FAILURE;
    }

    int maxMeshMTimePartition = -1;
    for (unsigned int i = 0; i < data->GetNumberOfPartitions(); i++)
    {
      maxMeshMTimePartition =
        std::max(static_cast<int>(data->GetPartition(i)->GetMeshMTime()), maxMeshMTimePartition);
    }
    meshMTime[0] = meshMTime[1];
    meshMTime[1] = maxMeshMTimePartition;
    if (iStep > 0 && iStep < 6)
    {
      if (meshMTime[0] != meshMTime[1])
      {
        std::cerr << "MTime: Failed MeshMTime check - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << std::endl;
        return EXIT_FAILURE;
      }
    }
    else if (iStep == 6)
    {
      if (meshMTime[0] == meshMTime[1])
      {
        std::cerr << "MTime: Failed MeshMTime shouldn't be equal - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << std::endl;
        return EXIT_FAILURE;
      }
    }
    // Local Time Checks
    if (!vtkMathUtilities::FuzzyCompare(
          reader->GetTimeValue(), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "Property: TimeValue is wrong: " << reader->GetTimeValue()
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = data->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cerr << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestPartitionedPolyDataTemporalWithOffset(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/temporal_partitioned_polydata.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  if (reader->GetNumberOfSteps() != 12)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << 12 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.719948, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.719948) != (" << tRange[0] << ", " << tRange[1]
              << ")" << std::endl;
    return EXIT_FAILURE;
  }

  for (int iStep = 0; iStep < 12; iStep += 5)
  {
    // Open data at right time
    reader->SetStep(iStep);
    reader->Update();
    vtkDataObject* data = reader->GetOutputDataObject(0);

    vtkNew<vtkXMLPartitionedDataSetReader> refReader;
    refReader->SetFileName(
      (dataRoot + "/Data/vtkHDF/temporal_partitioned_polydata_" + vtk::to_string(iStep) + ".vtpd")
        .c_str());
    refReader->Update();
    vtkDataObject* refData = refReader->GetOutputDataObject(0);

    if (!vtkTestUtilities::CompareDataObjects(data, refData))
    {
      std::cerr << "Partitioned Polydata with offsects are not the same for timestep " << iStep
                << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestPolyDataTemporalFieldData(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/temporal_polydata_field_data.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  if (reader->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  for (int iStep = 0; iStep < 10; iStep++)
  {
    // Open data at right time
    reader->SetStep(iStep);
    reader->Update();
    vtkDataObject* data = reader->GetOutputDataObject(0);

    auto* polyData = vtkPolyData::SafeDownCast(data);
    if (!polyData)
    {
      std::cerr << "The data isn't a polydata." << std::endl;
      return EXIT_FAILURE;
    }

    vtkFieldData* fdData = polyData->GetFieldData();
    if (!fdData)
    {
      std::cerr << "The data should contains field data." << std::endl;
      return EXIT_FAILURE;
    }
    vtkAbstractArray* testArray = fdData->GetAbstractArray("Test");
    if (!testArray)
    {
      std::cerr << "The data should contains field data a field data array \"Test\"." << std::endl;
      return EXIT_FAILURE;
    }

    int expectedNbComponents = EXPECTED_SHAPE_AT_TIMESTEP[iStep % 3][0];
    int expectedNbTuples = EXPECTED_SHAPE_AT_TIMESTEP[iStep % 3][1];
    if (testArray->GetNumberOfComponents() != expectedNbComponents ||
      testArray->GetNumberOfTuples() != expectedNbTuples)
    {
      std::cerr << "The field data's shape doesn't match the expected (" << expectedNbComponents
                << ", " << expectedNbTuples << ") for step " << iStep << ", instead got ("
                << testArray->GetNumberOfComponents() << ", " << testArray->GetNumberOfTuples()
                << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestPartialArrayWithCompositeDataset(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/composite_partial_array.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  if (reader->GetNumberOfSteps() != 7)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << 7 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if ((tRange[0] != 0.0) || (tRange[1] != 9.0))
  {
    std::cerr << "Time range is incorrect: (0, 9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  std::array<double, 7> expectedFirstValues = { 1.14685, 1.97722, -4.61971, 0.317693, 1.70716,
    3.10965, -3.86293 };

  constexpr vtkIdType numberOfSteps = 7;
  for (vtkIdType iStep = 0; iStep < numberOfSteps; iStep++)
  {
    reader->SetStep(iStep);
    reader->Update();
    vtkMultiBlockDataSet* data = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));

    if (!data)
    {
      std::cerr << "The data isn't a multi block." << std::endl;
      return EXIT_FAILURE;
    }

    if (data->GetNumberOfBlocks() != 2)
    {
      std::cerr << "The multiblock should contain 2 blocks but has " << data->GetNumberOfBlocks()
                << " instead." << std::endl;
      return EXIT_FAILURE;
    }

    auto block0 = vtkPolyData::SafeDownCast(data->GetBlock(0));
    auto block1 = vtkPolyData::SafeDownCast(data->GetBlock(1));
    if (!block0 || !block1)
    {
      std::cerr << "Both blocks should be polydata." << std::endl;
      return EXIT_FAILURE;
    }

    // only the first block contains this data array
    if (block0->GetPointData()->GetNumberOfArrays() != 1)
    {
      std::cout << "Block0 should contain point data arrays." << std::endl;
      return EXIT_FAILURE;
    }
    if (block1->GetPointData()->GetNumberOfArrays() != 0)
    {
      std::cout << "Block1 should not contain point data arrays." << std::endl;
      return EXIT_FAILURE;
    }

    // check at least the first value at each time step
    auto spatioTemporalArray =
      vtkDoubleArray::SafeDownCast(block0->GetPointData()->GetArray("SpatioTemporalHarmonics"));
    if (!vtkMathUtilities::FuzzyCompare(
          spatioTemporalArray->GetValue(0), expectedFirstValues[iStep], CHECK_TOLERANCE))
    {
      std::cerr << "The first value of the array at step " << iStep
                << " is incorrect: " << spatioTemporalArray->GetValue(0)
                << " != " << expectedFirstValues[iStep] << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestHyperTreeGridTemporal(const std::string& dataRoot, unsigned int depthLimit)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/temporal_htg.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  constexpr vtkIdType numberOfSteps = 5;
  if (reader->GetNumberOfSteps() != numberOfSteps)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << numberOfSteps << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], (numberOfSteps - 1) * 0.1, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, " << (numberOfSteps - 1) * 0.1 << ") != ("
              << tRange[0] << ", " << tRange[1] << ")" << std::endl;
    return EXIT_FAILURE;
  }

  // Create HTG Source to compare data to.
  const std::array descriptors = { "....", ".R.. | ....", "RR.. | .... ....", "RR.. | .... ....",
    "RRRR | .... R... .... .... | ...." };
  vtkNew<vtkHyperTreeGridSource> htgSource;
  htgSource->SetBranchFactor(2);
  htgSource->SetDimensions(3, 3, 1);
  htgSource->SetMaxDepth(depthLimit);

  reader->SetMaximumLevelsToReadByDefaultForAMR(depthLimit);

  for (int iStep = 0; iStep < numberOfSteps; iStep++)
  {
    // Open data at right time
    reader->SetStep(iStep);
    reader->Update();
    vtkDataObject* data = reader->GetOutputDataObject(0);

    htgSource->SetDescriptor(descriptors[iStep]);
    htgSource->Update();
    vtkHyperTreeGrid* expectedHTG = htgSource->GetHyperTreeGridOutput();
    vtkHyperTreeGrid* readHTG = vtkHyperTreeGrid::SafeDownCast(data);

    // Generated HTG Source is not temporal, so it will not have a time field array
    vtkNew<vtkFieldData> field;
    readHTG->SetFieldData(field);

    if (!vtkTestUtilities::CompareDataObjects(expectedHTG, readHTG))
    {
      std::cerr << "HyperTreeGrids are not the same for time step " << iStep << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestHyperTreeGridPartitionedTemporal(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/multipiece_temporal_htg.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  constexpr vtkIdType numberOfSteps = 2;
  if (reader->GetNumberOfSteps() != numberOfSteps)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << numberOfSteps << std::endl;
    return EXIT_FAILURE;
  }

  // Create HTG Source to compare data to.
  const std::array descriptorsPart1 = {
    "... .R. ... ... ... | ....",
    "... RRR ... ... ... | .... ...R .... | ....",
  };
  const std::array descriptorsPart2 = {
    "... ... ... .R. ... | ....",
    "... ... ... .RR ... | .... ....",
  };
  const std::array masksPart1 = {
    "111 111 111 000 000 | 1111",
    "111 111 111 000 000 | 1111 1111 1111 | 1111",
  };
  const std::array masksPart2 = { "000 000 000 111 111 | 1111", "000 000 000 111 111 | 1111 1111" };

  vtkNew<vtkHyperTreeGridSource> htgSource;
  htgSource->SetBranchFactor(2);
  htgSource->SetDimensions(6, 4, 1);
  htgSource->SetMaxDepth(3);
  htgSource->SetUseMask(true);

  for (int iStep = 0; iStep < numberOfSteps; iStep++)
  {
    // Open data at right time
    reader->SetStep(iStep);
    reader->Update();
    vtkPartitionedDataSet* data =
      vtkPartitionedDataSet::SafeDownCast(reader->GetOutputDataObject(0));

    htgSource->SetDescriptor(descriptorsPart1[iStep]);
    htgSource->SetMask(masksPart1[iStep]);
    htgSource->Update();
    vtkHyperTreeGrid* expectedHTG = htgSource->GetHyperTreeGridOutput();
    vtkHyperTreeGrid* readHTG = vtkHyperTreeGrid::SafeDownCast(data->GetPartitionAsDataObject(0));

    // Generated HTG Source is not temporal, so it will not have a time field array
    vtkNew<vtkFieldData> field;
    readHTG->SetFieldData(field);

    if (!vtkTestUtilities::CompareDataObjects(expectedHTG, readHTG))
    {
      std::cerr << "HyperTreeGrids are not the same for part 0 of time step " << iStep << std::endl;
      return EXIT_FAILURE;
    }

    htgSource->SetDescriptor(descriptorsPart2[iStep]);
    htgSource->SetMask(masksPart2[iStep]);
    htgSource->Update();
    expectedHTG = htgSource->GetHyperTreeGridOutput();
    readHTG = vtkHyperTreeGrid::SafeDownCast(data->GetPartitionAsDataObject(1));
    readHTG->SetFieldData(field);
    if (!vtkTestUtilities::CompareDataObjects(expectedHTG, readHTG))
    {
      std::cerr << "HyperTreeGrids are not the same for part 1 of time step " << iStep << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestOverlappingAMRTemporalBase(const std::string& dataRoot, const std::string& dataName)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + dataName).c_str());
  reader->Update();

  // Generic Time data checks
  vtkIdType nbSteps = 3;
  if (reader->GetNumberOfSteps() != nbSteps)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << nbSteps << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 1.0, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 1.0) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  for (vtkIdType iStep = 0; iStep < nbSteps; iStep++)
  {
    // Open data at right time
    reader->SetStep(iStep);
    reader->Update();
    vtkOverlappingAMR* data = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));

    // Both usage look to the same file for comparison
    vtkNew<vtkXMLUniformGridAMRReader> outputReader;
    std::string expectedFileName = dataRoot +
      "/Data/vtkHDF/Transient/transient_expected_overlapping_amr_" + vtk::to_string(iStep) +
      ".vthb";
    outputReader->SetFileName(expectedFileName.c_str());
    outputReader->SetMaximumLevelsToReadByDefault(0);
    outputReader->Update();
    auto expectedData = vtkOverlappingAMR::SafeDownCast(outputReader->GetOutput());

    if (data == nullptr || expectedData == nullptr)
    {
      std::cerr << "Input dataset is empty at timestep " << iStep << std::endl;
      return EXIT_FAILURE;
    }

    unsigned int numLevels = data->GetNumberOfLevels();
    if (numLevels != expectedData->GetNumberOfLevels())
    {
      std::cerr << "Expected " << expectedData->GetNumberOfLevels() << " levels but got "
                << numLevels << std::endl;
      return EXIT_FAILURE;
    }

    auto origin = data->GetOrigin();
    auto expectedOrigin = expectedData->GetOrigin();
    bool wrongOriginX =
      !vtkMathUtilities::FuzzyCompare(origin[0], expectedOrigin[0], CHECK_TOLERANCE);
    bool wrongOriginY =
      !vtkMathUtilities::FuzzyCompare(origin[1], expectedOrigin[1], CHECK_TOLERANCE);
    bool wrongOriginZ =
      !vtkMathUtilities::FuzzyCompare(origin[2], expectedOrigin[2], CHECK_TOLERANCE);

    if (wrongOriginX || wrongOriginY || wrongOriginZ)
    {
      std::cerr << "Wrong origin, it should be {" << expectedOrigin[0] << "," << expectedOrigin[1]
                << "," << expectedOrigin[2] << "} but got {" << origin[0] << "," << origin[1] << ","
                << origin[2] << "}." << std::endl;
      return EXIT_FAILURE;
    }

    for (unsigned int levelIndex = 0; levelIndex < expectedData->GetNumberOfLevels(); ++levelIndex)
    {
      if (data->GetNumberOfBlocks(levelIndex) != expectedData->GetNumberOfBlocks(levelIndex))
      {
        std::cerr << "Number of blocks does not match for level " << levelIndex
                  << ". Expected: " << expectedData->GetNumberOfBlocks(0)
                  << " got: " << data->GetNumberOfBlocks(0) << std::endl;
        return EXIT_FAILURE;
      }

      for (unsigned int datasetIndex = 0;
           datasetIndex < expectedData->GetNumberOfBlocks(levelIndex); ++datasetIndex)
      {
        vtkImageData* dataset = data->GetDataSetAsImageData(levelIndex, datasetIndex);
        vtkImageData* expectedDataset =
          expectedData->GetDataSetAsImageData(levelIndex, datasetIndex);
        if (!vtkTestUtilities::CompareDataObjects(dataset, expectedDataset))
        {
          std::cerr << "Datasets does not match for level " << levelIndex << " dataset "
                    << datasetIndex << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestOverlappingAMRTemporal(const std::string& dataRoot)
{
  return ::TestOverlappingAMRTemporalBase(
    dataRoot, "/Data/vtkHDF/test_temporal_overlapping_amr.vtkhdf");
}

//------------------------------------------------------------------------------
// Ensures retro-compatibility with the VTKHDF specification v2.2 which has a typo in the
// Point/Cell/FieldDataOffset name arrays.
int TestOverlappingAMRTemporalLegacy(const std::string& dataRoot)
{
  return ::TestOverlappingAMRTemporalBase(
    dataRoot, "/Data/vtkHDF/test_temporal_overlapping_amr_version_2_2.vtkhdf");
}

//------------------------------------------------------------------------------
int TestPDCPolyDataUGTemporal(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/temporal_pdc_polydata_ug.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  if (reader->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  std::array<vtkMTimeType, 2> meshMTime{ 0, 0 };

  // Test only first three timesteps
  for (int iStep = 0; iStep < 2; iStep++)
  {
    // Open data at right time
    reader->SetStep(iStep);
    reader->Update();
    vtkDataObject* data = reader->GetOutputDataObject(0);

    vtkNew<vtkXMLPartitionedDataSetCollectionReader> refReader;
    refReader->SetFileName(
      (dataRoot + "/Data/vtkHDF/temporal_pdc_polydata_ug_" + vtk::to_string(iStep) + ".vtpc")
        .c_str());
    refReader->Update();
    vtkDataObject* refData = refReader->GetOutputDataObject(0);

    if (!vtkTestUtilities::CompareDataObjects(data, refData))
    {
      std::cerr << "Partitioned Polydata with offsects are not the same for timestep " << iStep
                << std::endl;
      return EXIT_FAILURE;
    }

    // First block is not changing over time, check its mtime is not changing either
    meshMTime[1] = meshMTime[0];
    meshMTime[0] =
      vtkPartitionedDataSetCollection::SafeDownCast(data)->GetPartition(0, 0)->GetMeshMTime();
    if (iStep > 0)
    {
      if (meshMTime[0] != meshMTime[1])
      {
        std::cerr << "MTime: PDC Failed MeshMTime check - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << std::endl;
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestMBPolyDataUGTemporal(const std::string& dataRoot)
{
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName((dataRoot + "/Data/vtkHDF/temporal_mb_polydata_ug.vtkhdf").c_str());
  reader->Update();

  // Generic Time data checks
  if (reader->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << reader->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = reader->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  std::array<vtkMTimeType, 2> meshMTime{ 0, 0 };

  // Test only first three timesteps
  for (int iStep = 0; iStep < 2; iStep++)
  {
    // Open data at right time
    reader->SetStep(iStep);
    reader->Update();
    vtkDataObject* data = reader->GetOutputDataObject(0);

    vtkNew<vtkXMLMultiBlockDataReader> refReader;
    refReader->SetFileName(
      (dataRoot + "/Data/vtkHDF/temporal_mb_polydata_ug_" + vtk::to_string(iStep) + ".vtm")
        .c_str());
    refReader->Update();
    vtkDataObject* refData = refReader->GetOutputDataObject(0);

    if (!vtkTestUtilities::CompareDataObjects(data, refData))
    {
      std::cerr << "Partitioned Polydata with offsects are not the same for timestep " << iStep
                << std::endl;
      return EXIT_FAILURE;
    }

    // First block is not changing over time, check its mtime is not changing either
    meshMTime[1] = meshMTime[0];
    meshMTime[0] = vtkDataSet::SafeDownCast(vtkMultiBlockDataSet::SafeDownCast(data)->GetBlock(0))
                     ->GetMeshMTime();
    if (iStep > 0)
    {
      if (meshMTime[0] != meshMTime[1])
      {
        std::cerr << "MTime: PDC Failed MeshMTime check - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << std::endl;
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}
}
