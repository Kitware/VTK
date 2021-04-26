/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLWriteRead.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArrayDispatch.h"
#include "vtkFloatArray.h"
#include "vtkHDFReader.h"
#include "vtkImageData.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLPUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridReader.h"

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

int TestDataSet(vtkDataSet* data, vtkDataSet* expectedData)
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
  for (int attributeType = 0; attributeType < vtkDataObject::FIELD; ++attributeType)
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
      vtkAbstractArray* expectedArray = expectedFieldData->GetAbstractArray(i);
      vtkAbstractArray* array = fieldData->GetAbstractArray(expectedArray->GetName());
      if (std::string(expectedArray->GetClassName()) != array->GetClassName() &&
        // long long == long
        !(std::string(expectedArray->GetClassName()) == "vtkLongLongArray" &&
          std::string(array->GetClassName()) == "vtkLongArray" &&
          sizeof(long long) == sizeof(long)) &&
        // unsigned long long == unsigned long
        !(std::string(expectedArray->GetClassName()) == "vtkUnsignedLongLongArray" &&
          std::string(array->GetClassName()) == "vtkUnsignedLongArray" &&
          sizeof(unsigned long long) == sizeof(unsigned long)) &&
        // vtkIdType == long
        !(std::string(expectedArray->GetClassName()) == "vtkIdTypeArray" &&
          std::string(array->GetClassName()) == "vtkLongArray" &&
          sizeof(vtkIdType) == sizeof(long)) &&
        // vtkIdType == long long
        !(std::string(expectedArray->GetClassName()) == "vtkIdTypeArray" &&
          std::string(array->GetClassName()) == "vtkLongLongArray" &&
          sizeof(vtkIdType) == sizeof(long long)))
      {
        std::cerr << "Different array type: " << array->GetClassName() << " from expected "
                  << expectedArray->GetClassName() << " for array: " << expectedArray->GetName()
                  << std::endl
                  << "sizeof(long long): " << sizeof(long long) << std::endl
                  << "sizeof(long): " << sizeof(long) << std::endl;
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
    };
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

  return TestDataSet(data, expectedData);
}

template <bool parallel>
int TestUnstructuredGrid(const std::string& dataRoot)
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

  if (TestUnstructuredGrid<false /*parallel*/>(dataRoot))
  {
    return EXIT_FAILURE;
  }
  if (TestUnstructuredGrid<true /*parallel*/>(dataRoot))
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
