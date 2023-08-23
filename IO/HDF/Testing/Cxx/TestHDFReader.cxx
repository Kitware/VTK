// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendDataSets.h"
#include "vtkArrayDispatch.h"
#include "vtkFloatArray.h"
#include "vtkHDFReader.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTesting.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLPUnstructuredGridReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLUniformGridAMRReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <cstdlib>
#include <iterator>
#include <string>

struct CompareVectorWorker
{
  CompareVectorWorker()
    : ExitValue(EXIT_SUCCESS)
  {
  }
  template <typename ArrayT, typename ExpectedArrayT>
  void operator()(ArrayT* array, ExpectedArrayT* expectedArray)
  {
    const auto range = vtk::DataArrayTupleRange(array);
    const auto expectedRange = vtk::DataArrayTupleRange(expectedArray);

    const vtk::TupleIdType numTuples = range.size();
    const vtk::ComponentIdType numComps = range.GetTupleSize();

    std::cout << "Compare " << array->GetName() << std::endl;
    this->ExitValue = EXIT_SUCCESS;
    for (vtk::TupleIdType tupleId = 0; tupleId < numTuples; ++tupleId)
    {
      const auto tuple = range[tupleId];
      auto expectedTuple = expectedRange[tupleId];

      for (vtk::ComponentIdType compId = 0; compId < numComps; ++compId)
      {
        if (tuple[compId] != expectedTuple[compId])
        {
          std::cerr << "Expecting " << expectedTuple[compId] << " for tuple/component: " << tupleId
                    << "/" << compId << " but got: " << tuple[compId] << std::endl;
          this->ExitValue = EXIT_FAILURE;
          break;
        }
      }
    }
  }
  int ExitValue;
};

int CompareVectors(vtkDataArray* array, vtkDataArray* expectedArray)
{
  using Dispatcher = vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::AllTypes>;

  // Create the functor:
  CompareVectorWorker worker;

  if (!Dispatcher::Execute(array, expectedArray, worker))
  {
    // If Execute(...) fails, the arrays don't match the constraints.
    // Run the algorithm using the slower vtkDataArray double API instead:
    worker(array, expectedArray);
  }
  return worker.ExitValue;
}

vtkSmartPointer<vtkImageData> ReadImageData(const std::string& fileName)
{
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkSmartPointer<vtkImageData> data = vtkImageData::SafeDownCast(reader->GetOutput());
  return data;
}

struct ArrayTypeTester
{
  template <class ArrayT1, class ArrayT2>
  void operator()(ArrayT1*, ArrayT2*)
  {
    using ValueType1 = typename ArrayT1::ValueType;
    using ValueType2 = typename ArrayT2::ValueType;
    this->ArraysArePointerCompatible = (sizeof(ValueType1) == sizeof(ValueType2)) &&
      (std::is_integral<ValueType1>::value == std::is_integral<ValueType2>::value);
  }

  vtkAbstractArray* Array;
  bool ArraysArePointerCompatible;
};

int TestDataSet(vtkDataSet* data, vtkDataSet* expectedData, bool includeFieldData = false)
{
  if (data == nullptr || expectedData == nullptr)
  {
    std::cerr << "Error: Data not in the format expected." << std::endl;
    return EXIT_FAILURE;
  }

  if (data->GetNumberOfPoints() != expectedData->GetNumberOfPoints())
  {
    std::cerr << "Expecting " << expectedData->GetNumberOfPoints()
              << " points but got: " << data->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }

  if (data->GetNumberOfCells() != expectedData->GetNumberOfCells())
  {
    std::cerr << "Expecting " << expectedData->GetNumberOfCells()
              << " cells but got: " << data->GetNumberOfCells() << std::endl;
    return EXIT_FAILURE;
  }
  for (int attributeType = 0; attributeType < vtkDataObject::FIELD + (includeFieldData ? 1 : 0);
       ++attributeType)
  {
    int numberRead = data->GetAttributesAsFieldData(attributeType)->GetNumberOfArrays();
    int numberExpected = expectedData->GetAttributesAsFieldData(attributeType)->GetNumberOfArrays();
    if (numberRead != numberExpected)
    {
      std::cerr << "Expecting " << numberExpected << " arrays of type " << attributeType
                << " but got " << numberRead << std::endl;
      return EXIT_FAILURE;
    }
    vtkFieldData* fieldData = data->GetAttributesAsFieldData(attributeType);
    vtkFieldData* expectedFieldData = expectedData->GetAttributesAsFieldData(attributeType);
    for (int i = 0; i < numberRead; ++i)
    {
      // the arrays are not in the same order because listing arrays in creation
      // order fails. See vtkHDFReader::Implementation::GetArrayNames
      vtkDataArray* expectedArray = expectedFieldData->GetArray(i);
      vtkDataArray* array = fieldData->GetArray(expectedArray->GetName());

      using Dispatcher = vtkArrayDispatch::Dispatch2;
      ArrayTypeTester tester;
      Dispatcher::Execute(array, expectedArray, tester);
      if (!tester.ArraysArePointerCompatible)
      {
        vtkLog(ERROR,
          "Read array and expected arrays do not have compatible pointers for "
            << expectedArray->GetName() << "."
            << " Read array: " << array->GetClassName()
            << " Expected array: " << expectedArray->GetClassName());
        return EXIT_FAILURE;
      }

      if (array->GetNumberOfTuples() != expectedArray->GetNumberOfTuples() ||
        array->GetNumberOfComponents() != expectedArray->GetNumberOfComponents())
      {
        std::cerr << "Array " << array->GetName() << " has a different number of "
                  << "tuples/components: " << array->GetNumberOfTuples() << "/"
                  << array->GetNumberOfComponents()
                  << " than expected: " << expectedArray->GetNumberOfTuples() << "/"
                  << expectedArray->GetNumberOfComponents() << std::endl;
        return EXIT_FAILURE;
      }
      vtkDataArray* a = vtkDataArray::SafeDownCast(array);
      vtkDataArray* ea = vtkDataArray::SafeDownCast(expectedArray);
      if (a)
      {
        if (CompareVectors(a, ea))
        {
          return EXIT_FAILURE;
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

int TestImageData(const std::string& dataRoot)
{
  // ImageData file
  // ------------------------------------------------------------
  std::string fileName = dataRoot + "/Data/mandelbrot-vti.hdf";
  std::cout << "Testing: " << fileName << std::endl;
  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(fileName.c_str()))
  {
    return EXIT_FAILURE;
  }
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkImageData* data = vtkImageData::SafeDownCast(reader->GetOutput());
  vtkSmartPointer<vtkImageData> expectedData = ReadImageData(dataRoot + "/Data/mandelbrot.vti");

  int* dims = data->GetDimensions();
  int* edims = expectedData->GetDimensions();
  if (dims[0] != edims[0] || dims[1] != edims[1] || dims[2] != edims[2])
  {
    std::cerr << "Error: vtkImageData with wrong dimensions: "
              << "expecting "
              << "[" << edims[0] << ", " << edims[1] << ", " << edims[2] << "]"
              << " got "
              << "[" << dims[0] << ", " << dims[1] << ", " << dims[2] << "]" << std::endl;
    return EXIT_FAILURE;
  }

  return TestDataSet(data, expectedData, true);
}

int TestImageCellData(const std::string& dataRoot)
{
  // ImageData file with cell data
  // ------------------------------------------------------------
  std::string fileName = dataRoot + "/Data/wavelet_cell_data.hdf";
  std::cout << "Testing: " << fileName << std::endl;
  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(fileName.c_str()))
  {
    return EXIT_FAILURE;
  }
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkImageData* data = vtkImageData::SafeDownCast(reader->GetOutput());
  vtkSmartPointer<vtkImageData> expectedData =
    ReadImageData(dataRoot + "/Data/wavelet_cell_data.vti");

  int* dims = data->GetDimensions();
  int* edims = expectedData->GetDimensions();
  if (dims[0] != edims[0] || dims[1] != edims[1] || dims[2] != edims[2])
  {
    std::cerr << "Error: vtkImageData with wrong dimensions: "
              << "expecting "
              << "[" << edims[0] << ", " << edims[1] << ", " << edims[2] << "]"
              << " got "
              << "[" << dims[0] << ", " << dims[1] << ", " << dims[2] << "]" << std::endl;
    return EXIT_FAILURE;
  }

  return TestDataSet(data, expectedData);
}

int TestUnstructuredGrid(const std::string& dataRoot, bool parallel)
{
  std::string fileName, expectedName;
  vtkNew<vtkHDFReader> reader;
  vtkNew<vtkXMLUnstructuredGridReader> expectedReader;
  vtkNew<vtkXMLPUnstructuredGridReader> expectedPReader;
  vtkXMLReader* oreader;
  if (parallel)
  {
    fileName = dataRoot + "/Data/can-pvtu.hdf";
    expectedName = dataRoot + "/Data/can.pvtu";
    oreader = expectedPReader;
  }
  else
  {
    fileName = dataRoot + "/Data/can-vtu.hdf";
    expectedName = dataRoot + "/Data/can.vtu";
    oreader = expectedReader;
  }
  std::cout << "Testing: " << fileName << std::endl;
  if (!reader->CanReadFile(fileName.c_str()))
  {
    return EXIT_FAILURE;
  }
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkUnstructuredGrid* data = vtkUnstructuredGrid::SafeDownCast(reader->GetOutputAsDataSet());

  oreader->SetFileName(expectedName.c_str());
  oreader->Update();
  vtkUnstructuredGrid* expectedData =
    vtkUnstructuredGrid::SafeDownCast(oreader->GetOutputAsDataSet());
  return TestDataSet(data, expectedData);
}

int TestPartitionedUnstructuredGrid(const std::string& dataRoot, bool parallel)
{
  std::string fileName, expectedName;
  vtkNew<vtkHDFReader> reader;
  vtkNew<vtkXMLUnstructuredGridReader> expectedReader;
  vtkNew<vtkXMLPUnstructuredGridReader> expectedPReader;
  vtkXMLReader* oreader;
  if (parallel)
  {
    fileName = dataRoot + "/Data/can-pvtu.hdf";
    expectedName = dataRoot + "/Data/can.pvtu";
    oreader = expectedPReader;
  }
  else
  {
    fileName = dataRoot + "/Data/can-vtu.hdf";
    expectedName = dataRoot + "/Data/can.vtu";
    oreader = expectedReader;
  }
  std::cout << "Testing: " << fileName << std::endl;
  if (!reader->CanReadFile(fileName.c_str()))
  {
    return EXIT_FAILURE;
  }
  reader->SetFileName(fileName.c_str());
  reader->SetMergeParts(false);
  reader->Update();

  auto pds = vtkPartitionedDataSet::SafeDownCast(reader->GetOutput());
  if (!pds)
  {
    return EXIT_FAILURE;
  }
  vtkNew<vtkAppendDataSets> appender;
  for (unsigned int iPiece = 0; iPiece < pds->GetNumberOfPartitions(); ++iPiece)
  {
    auto piece = vtkUnstructuredGrid::SafeDownCast(pds->GetPartition(iPiece));
    appender->AddInputData(piece);
  }
  appender->Update();

  auto data = vtkUnstructuredGrid::SafeDownCast(appender->GetOutput());

  oreader->SetFileName(expectedName.c_str());
  oreader->Update();
  vtkUnstructuredGrid* expectedData =
    vtkUnstructuredGrid::SafeDownCast(oreader->GetOutputAsDataSet());
  return TestDataSet(data, expectedData);
}

int TestPolyData(const std::string& dataRoot)
{
  const std::string expectedName = dataRoot + "/Data/hdf_poly_data_twin.vtp";
  vtkNew<vtkXMLPolyDataReader> expectedReader;
  expectedReader->SetFileName(expectedName.c_str());
  expectedReader->Update();
  auto expectedData = vtkPolyData::SafeDownCast(expectedReader->GetOutput());

  const std::string fileName = dataRoot + "/Data/test_poly_data.hdf";
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  auto data = vtkPolyData::SafeDownCast(reader->GetOutputAsDataSet());

  return TestDataSet(data, expectedData);
}

int TestPartitionedPolyData(const std::string& dataRoot)
{
  const std::string expectedName = dataRoot + "/Data/hdf_poly_data_twin.vtp";
  vtkNew<vtkXMLPolyDataReader> expectedReader;
  expectedReader->SetFileName(expectedName.c_str());
  expectedReader->Update();
  auto expectedData = vtkPolyData::SafeDownCast(expectedReader->GetOutput());

  const std::string fileName = dataRoot + "/Data/test_poly_data.hdf";
  vtkNew<vtkHDFReader> reader;
  reader->SetMergeParts(false);
  reader->SetFileName(fileName.c_str());
  reader->Update();

  auto pds = vtkPartitionedDataSet::SafeDownCast(reader->GetOutput());
  if (!pds)
  {
    return EXIT_FAILURE;
  }
  vtkNew<vtkAppendDataSets> appender;
  appender->SetOutputDataSetType(VTK_POLY_DATA);
  for (unsigned int iPiece = 0; iPiece < pds->GetNumberOfPartitions(); ++iPiece)
  {
    auto piece = vtkPolyData::SafeDownCast(pds->GetPartition(iPiece));
    appender->AddInputData(piece);
  }
  appender->Update();

  auto data = vtkPolyData::SafeDownCast(appender->GetOutput());

  return TestDataSet(data, expectedData);
}

int TestOverlappingAMR(const std::string& dataRoot)
{
  std::string fileName = dataRoot + "/Data/amr_gaussian_pulse.hdf";
  std::cout << "Testing: " << fileName << std::endl;
  vtkNew<vtkHDFReader> reader;
  if (!reader->CanReadFile(fileName.c_str()))
  {
    return EXIT_FAILURE;
  }
  reader->SetFileName(fileName.c_str());
  reader->Update();
  auto data = vtkOverlappingAMR::SafeDownCast(reader->GetOutput());

  vtkNew<vtkXMLUniformGridAMRReader> outputReader;
  std::string expectedFileName = dataRoot + "/Data/amr_gaussian_pulse.vthb";
  outputReader->SetFileName(expectedFileName.c_str());
  outputReader->SetMaximumLevelsToReadByDefault(0);
  outputReader->Update();
  auto expectedData = vtkOverlappingAMR::SafeDownCast(outputReader->GetOutput());

  if (data->GetNumberOfLevels() != expectedData->GetNumberOfLevels())
  {
    std::cerr << "Number of levels does not match. Expected: " << expectedData->GetNumberOfLevels()
              << " got: " << data->GetNumberOfLevels() << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned int levelIndex = 0; levelIndex < expectedData->GetNumberOfLevels(); ++levelIndex)
  {
    if (data->GetNumberOfDataSets(levelIndex) != expectedData->GetNumberOfDataSets(levelIndex))
    {
      std::cerr << "Number of datasets does not match for level " << levelIndex
                << ". Expected: " << expectedData->GetNumberOfDataSets(0)
                << " got: " << data->GetNumberOfDataSets(0) << std::endl;
      return EXIT_FAILURE;
    }

    for (unsigned int datasetIndex = 0;
         datasetIndex < expectedData->GetNumberOfDataSets(levelIndex); ++datasetIndex)
    {
      auto dataset = data->GetDataSet(levelIndex, datasetIndex);
      auto expectedDataset = expectedData->GetDataSet(levelIndex, datasetIndex);
      if (TestDataSet(dataset, expectedDataset))
      {
        std::cerr << "Datasets does not match for level " << levelIndex << " dataset "
                  << datasetIndex << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

int TestHDFReader(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();
  if (TestImageData(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (TestImageCellData(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (TestUnstructuredGrid(dataRoot, false))
  {
    return EXIT_FAILURE;
  }
  if (TestUnstructuredGrid(dataRoot, true))
  {
    return EXIT_FAILURE;
  }

  if (TestPolyData(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (TestOverlappingAMR(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (TestPartitionedPolyData(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (TestPartitionedUnstructuredGrid(dataRoot, false))
  {
    return EXIT_FAILURE;
  }

  if (TestPartitionedUnstructuredGrid(dataRoot, true))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
