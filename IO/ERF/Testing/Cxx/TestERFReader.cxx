// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataAssembly.h"
#include "vtkERFReader.h"
#include "vtkFieldData.h"
#include "vtkIndent.h"
#include "vtkInformation.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

#include <cmath>
#include <iostream>

namespace
{
//----------------------------------------------------------------------------
int CheckFieldDataAsString(
  vtkFieldData* fieldData, const std::string name, const std::string expected)
{
  if (!fieldData->HasArray(name.c_str()))
  {
    std::cerr << "Missing '" << name << "' field data." << std::endl;
    return EXIT_FAILURE;
  }

  std::string data = fieldData->GetAbstractArray(name.c_str())->GetVariantValue(0).ToString();
  if (data != expected)
  {
    std::cerr << "'" << name << "' should contains '" << expected << "' but got " << data << "."
              << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
template <typename ValueType>
int CheckFieldDataAsArray(
  vtkFieldData* fieldData, const std::string name, std::map<int, ValueType> expectedValues)
{
  if (!fieldData->HasArray(name.c_str()))
  {
    std::cerr << "Missing '" << name << "' field data." << std::endl;
    return EXIT_FAILURE;
  }

  vtkAbstractArray* array = fieldData->GetAbstractArray(name.c_str());

  if (array->GetNumberOfValues() != static_cast<vtkIdType>(expectedValues.size()))
  {
    std::cerr << "Array '" << name << "', expected " << expectedValues.size() << " values but got "
              << array->GetNumberOfValues() << "." << std::endl;
    return EXIT_FAILURE;
  }

  for (const auto& pair : expectedValues)
  {
    ValueType value = 0;
    vtkVariant variant = array->GetVariantValue(pair.first);
    if (variant.IsInt())
    {
      value = variant.ToInt();
    }

    if (variant.IsDouble())
    {
      value = variant.ToDouble();
    }

    if (variant.IsFloat())
    {
      value = variant.ToFloat();
    }

    if (value != pair.second)
    {
      std::cerr << "'" << name << "' should contains '" << pair.second << "' but got " << value
                << "." << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
/**
 * Specialization for std::string
 */
template <>
int CheckFieldDataAsArray<std::string>(
  vtkFieldData* fieldData, const std::string name, std::map<int, std::string> expectedValues)
{
  if (!fieldData->HasArray(name.c_str()))
  {
    std::cerr << "Missing '" << name << "' field data." << std::endl;
    return EXIT_FAILURE;
  }

  vtkStringArray* array = vtkStringArray::SafeDownCast(fieldData->GetAbstractArray(name.c_str()));
  if (array->GetNumberOfValues() != static_cast<vtkIdType>(expectedValues.size()))
  {
    std::cerr << "Array '" << name << "', expected " << expectedValues.size() << " values but got "
              << array->GetNumberOfValues() << "." << std::endl;
    return EXIT_FAILURE;
  }

  for (const auto& pair : expectedValues)
  {
    std::string value = array->GetValue(pair.first);
    if (value != pair.second)
    {
      std::cerr << "'" << name << "' should contains '" << pair.second << "' but got " << value
                << "." << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

}

//----------------------------------------------------------------------------
int TestERFReader(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();

  std::string fileName = dataRoot + "/Data/hdf_fpm_simulation.erfh5";
  vtkNew<vtkERFReader> reader;
  reader->SetFileName(fileName);
  reader->UpdateInformation();
  reader->GetOutputInformation(0)->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 0.001);
  reader->EnableAllVariables();
  reader->Update();

  // Check on the structure of the output partitioned dataset collection's assembly
  auto pdc = reader->GetOutput();
  auto assembly = pdc->GetDataAssembly();
  if (assembly->GetNumberOfChildren(0) != 2)
  {
    std::cerr << "Invalid number of streams in file." << std::endl;
    return EXIT_FAILURE;
  }

  // Shell data
  auto dataSet = vtkUnstructuredGrid::SafeDownCast(pdc->GetPartition(0, 0));
  if (!dataSet)
  {
    std::cerr << "dataSet is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  int success = EXIT_SUCCESS;

  // Check Field Data
  auto fieldData = pdc->GetFieldData();
  // System Block: attributes
  success &= ::CheckFieldDataAsString(fieldData, "solver_name", "PAM-CSM:Explicit_Transient:FPM");
  success &= ::CheckFieldDataAsString(fieldData, "sys", "LXIA");
  success &= ::CheckFieldDataAsString(fieldData, "title", "ITWM_FPM_Result");
  success &= ::CheckFieldDataAsString(fieldData, "solver_vers", "2014");

  // System Block: dataSets
  std::map<int, int> ubidValues = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 } };
  success &= CheckFieldDataAsArray<int>(fieldData, "ubid", ubidValues);

  std::map<int, float> ubconValues = { { 0, 1.0 }, { 1, 1.0 }, { 2, 1.0 }, { 3, 1.0 } };
  success &= CheckFieldDataAsArray<float>(fieldData, "ubcon", ubconValues);

  std::map<int, std::string> ubnamValues = { { 0, "m" }, { 1, "kg" }, { 2, "s" }, { 3, "K" } };
  success &= CheckFieldDataAsArray<std::string>(fieldData, "ubnam", ubnamValues);

  // We can already return failure at this step
  if (success == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  // Check the mesh
  if (dataSet->GetNumberOfPoints() != 1417)
  {
    std::cerr << "Should have 1417 points but got " << dataSet->GetNumberOfPoints() << std::endl;
    success = EXIT_FAILURE;
  }

  auto* points = dataSet->GetPoints();

  double* p0 = points->GetPoint(0);
  double tolerance = 4.0 * std::numeric_limits<double>::epsilon();
  if (!vtkMathUtilities::FuzzyCompare(p0[0], 9.52581884, tolerance) ||
    !vtkMathUtilities::FuzzyCompare(p0[1], 1.8635755987, tolerance) ||
    !vtkMathUtilities::FuzzyCompare(p0[2], 0.0, tolerance))
  {
    std::cerr
      << "Wrong points values at index 0 should have {9.52581884, 1.8635755987, 0.0} but got {"
      << p0[0] << ", " << p0[1] << ", " << p0[2] << "}" << std::endl;
    success = EXIT_FAILURE;
  }

  double* p26 = points->GetPoint(26);

  if (!vtkMathUtilities::FuzzyCompare(p26[0], 9.4748499769, tolerance) ||
    !vtkMathUtilities::FuzzyCompare(p26[1], -2.5, tolerance) ||
    !vtkMathUtilities::FuzzyCompare(p26[2], 0.6254521103, 0.00001))
  {
    std::cerr
      << "Wrong points values at index 26 should have {9.4748499769, -2.5, 0.6254521103} but got {"
      << p26[0] << ", " << p26[1] << ", " << p26[2] << "}" << std::endl;
    success = EXIT_FAILURE;
  }

  // Check entid (ERF Point ID) data array
  if (!dataSet->GetPointData()->HasArray("entid"))
  {
    std::cerr << "Missing 'entid' as point data array" << std::endl;
    return EXIT_FAILURE;
  }

  if (dataSet->GetNumberOfCells() != 2766)
  {
    std::cerr << "The dataset should have 2766 cells but got " << dataSet->GetNumberOfCells() << "."
              << std::endl;
    return EXIT_FAILURE;
  }

  // Check data array
  if (dataSet->GetPointData()->GetNumberOfArrays() != 1)
  {
    std::cerr << "The dataset should have 1 points data but got "
              << dataSet->GetPointData()->GetNumberOfArrays() << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Check the FPM mesh now
  dataSet = vtkUnstructuredGrid::SafeDownCast(pdc->GetPartition(1, 0));

  if (dataSet->GetPointData()->GetNumberOfArrays() != 19)
  {
    std::cerr << "The dataset should have 19 points data but got "
              << dataSet->GetPointData()->GetNumberOfArrays() << "." << std::endl;
    return EXIT_FAILURE;
  }

  if (!dataSet->GetPointData()->HasArray("GlblIndex"))
  {
    std::cerr << "The dataset should have a point array named 'GlblIndex'." << std::endl;
    return EXIT_FAILURE;
  }

  return success;
}
