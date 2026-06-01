// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAffineArray.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkDoubleArray.h"
#include "vtkExplodeDataSet.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkTestErrorObserver.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataReader.h"

#include <iostream>

namespace details
{
const int INPUT_NB_OF_SCALARS = 6;
const std::string INPUT_SCALARS_NAME = "Material";

//------------------------------------------------------------------------------
bool CheckOutput(vtkDataSet* input, vtkPartitionedDataSetCollection* output,
  const std::vector<std::string>& expectedPartNames)
{
  if (output->GetNumberOfPartitionedDataSets() != INPUT_NB_OF_SCALARS)
  {
    vtkLog(ERROR, << "Output has " << output->GetNumberOfPartitionedDataSets()
                  << " parts instead of " << INPUT_NB_OF_SCALARS);
    return false;
  }

  if (output->GetNumberOfCells() != input->GetNumberOfCells())
  {
    vtkLog(ERROR, << "Output has wrong number of cells");
    return false;
  }

  const int nbOfPointsArrays = input->GetPointData()->GetNumberOfArrays();

  for (unsigned int cc = 0; cc < INPUT_NB_OF_SCALARS; ++cc)
  {
    vtkDataSet* outputDS = vtkDataSet::SafeDownCast(output->GetPartitionAsDataObject(cc, 0));
    auto partIdArray = outputDS->GetFieldData()->GetArray(details::INPUT_SCALARS_NAME.c_str());
    if (partIdArray->GetNumberOfValues() != 1)
    {
      vtkLog(ERROR, << "Partition should have a single value FieldData");
    }
    double partId = partIdArray->GetTuple1(0);
    const std::string& expectedBlockName = expectedPartNames[partId];
    auto name = output->GetMetaData(cc)->Get(vtkCompositeDataSet::NAME());
    if (name == nullptr || expectedBlockName != name)
    {
      vtkLog(ERROR, << "Mismatched block names. Has \"" << name << "\" but exptects \""
                    << expectedBlockName << "\"");
      return false;
    }

    if (outputDS->GetPointData()->GetNumberOfArrays() != nbOfPointsArrays)
    {
      vtkLog(ERROR, << "Output has wrong number of arrays.");
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestDataSet(vtkDataSet* dataset)
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkExplodeDataSet> split;
  split->SetInputDataObject(dataset);
  split->SetInputArrayToProcess(details::INPUT_SCALARS_NAME.c_str(), vtkDataObject::CELL);
  split->Update();

  vtkPartitionedDataSetCollection* output = split->GetOutput();
  std::vector<std::string> expectedPartNames;
  for (int partId = 0; partId < INPUT_NB_OF_SCALARS; partId++)
  {
    auto blockname = details::INPUT_SCALARS_NAME + "_" + vtk::to_string(partId);
    expectedPartNames.push_back(blockname);
  }
  return details::CheckOutput(dataset, output, expectedPartNames);
}

//------------------------------------------------------------------------------
bool TestCorrectPartNames(vtkDataSet* dataset, vtkStringArray* names, vtkDataArray* values)
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkExplodeDataSet> split;
  split->SetInputDataObject(dataset);
  split->SetInputArrayToProcess(details::INPUT_SCALARS_NAME.c_str(), vtkDataObject::CELL);
  split->UsePartitionNamesFromFieldDataOn();
  vtkPartitionedDataSetCollection* output = split->GetOutput();

  split->SetPartitionValuesArray(values->GetName());
  split->SetPartitionNamesArray(names->GetName());
  dataset->GetFieldData()->Initialize();
  dataset->GetFieldData()->AddArray(names);
  dataset->GetFieldData()->AddArray(values);

  std::vector<std::string> expectedPartNames;
  expectedPartNames.reserve(INPUT_NB_OF_SCALARS);
  for (int partId = 0; partId < INPUT_NB_OF_SCALARS; partId++)
  {
    expectedPartNames.push_back(names->GetValue(partId));
  }

  split->Update();
  bool result = details::CheckOutput(dataset, output, expectedPartNames);

  // longer arrays are not an issue
  vtkNew<vtkStringArray> longerNames;
  longerNames->DeepCopy(names);
  longerNames->InsertNextValue("extraName");
  vtkNew<vtkDoubleArray> longerValues;
  longerValues->DeepCopy(values);
  longerValues->InsertNextValue(42);

  dataset->GetFieldData()->Initialize();
  dataset->GetFieldData()->AddArray(longerNames);
  dataset->GetFieldData()->AddArray(values);
  split->Update();
  result = details::CheckOutput(dataset, output, expectedPartNames) && result;

  dataset->GetFieldData()->Initialize();
  dataset->GetFieldData()->AddArray(names);
  dataset->GetFieldData()->AddArray(longerValues);
  split->Update();
  result = details::CheckOutput(dataset, output, expectedPartNames) && result;

  return result;
}

//------------------------------------------------------------------------------
bool TestArrayNotFound(vtkDataSet* dataset, vtkStringArray* names, vtkDataArray* values,
  const std::vector<std::string>& defaultPartNames)
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkExplodeDataSet> explode;
  explode->SetInputDataObject(dataset);
  explode->SetInputArrayToProcess(details::INPUT_SCALARS_NAME.c_str(), vtkDataObject::CELL);
  explode->UsePartitionNamesFromFieldDataOn();
  vtkPartitionedDataSetCollection* output = explode->GetOutput();

  dataset->GetFieldData()->Initialize();
  dataset->GetFieldData()->AddArray(names);
  dataset->GetFieldData()->AddArray(values);

  vtkNew<vtkTest::ErrorObserver> observer;
  explode->AddObserver(vtkCommand::WarningEvent, observer);

  // inexesting arrays
  explode->SetPartitionNamesArray("wrong_name");
  explode->SetPartitionValuesArray("also_wrong_name");
  explode->Update();
  vtkLogIf(ERROR, observer->GetNumberOfWarnings() != 1,
    "vtkExplodeDataSet should warn about inexisting name array.");
  observer->Clear();
  if (!details::CheckOutput(dataset, output, defaultPartNames))
  {
    return false;
  }

  // array mismatch: wrong name array
  explode->SetPartitionNamesArray(values->GetName());
  explode->SetPartitionValuesArray(values->GetName());
  explode->Update();
  vtkLogIf(ERROR, observer->GetNumberOfWarnings() != 1,
    "vtkExplodeDataSet should warn about incorrect name array (vtkDataArray is not a "
    "vtkStringArray).");
  observer->Clear();
  if (!details::CheckOutput(dataset, output, defaultPartNames))
  {
    return false;
  }

  // array mismatch: wrong value array. Blindly use name array, but warn about it.
  std::vector<std::string> expectedPartNames;
  expectedPartNames.reserve(names->GetNumberOfValues());
  for (int partId = 0; partId < details::INPUT_NB_OF_SCALARS; partId++)
  {
    expectedPartNames.push_back(names->GetValue(partId));
  }

  explode->SetPartitionNamesArray(names->GetName());
  explode->SetPartitionValuesArray(names->GetName());
  explode->Update();
  vtkLogIf(ERROR, observer->GetNumberOfWarnings() != 1,
    "vtkExplodeDataSet should warn about incorrect value array (vtkStringArray is not a "
    "vtkDataArray).");
  observer->Clear();
  if (!details::CheckOutput(dataset, output, expectedPartNames))
  {
    return false;
  }

  // No value array. Silently use name array.
  explode->SetPartitionNamesArray(names->GetName());
  explode->SetPartitionValuesArray("");
  explode->Update();
  vtkLogIf(ERROR, observer->GetNumberOfWarnings() != 0,
    "vtkExplodeDataSet should fallback to linear ordering of names without warning.");
  observer->Clear();
  return details::CheckOutput(dataset, output, expectedPartNames);
}

//------------------------------------------------------------------------------
bool TestPartialArray(vtkDataSet* dataset, vtkStringArray* names, vtkDataArray* values,
  const std::vector<std::string>& defaultPartNames)
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkExplodeDataSet> explode;
  explode->SetInputDataObject(dataset);
  explode->SetInputArrayToProcess(details::INPUT_SCALARS_NAME.c_str(), vtkDataObject::CELL);
  explode->UsePartitionNamesFromFieldDataOn();
  vtkPartitionedDataSetCollection* output = explode->GetOutput();

  const vtkIdType truncatedSize = details::INPUT_NB_OF_SCALARS - 2;
  vtkNew<vtkStringArray> truncatedNames;
  truncatedNames->DeepCopy(names);
  truncatedNames->SetNumberOfTuples(truncatedSize);
  vtkNew<vtkDoubleArray> truncatedValues;
  truncatedValues->DeepCopy(values);
  truncatedValues->SetNumberOfTuples(truncatedSize);

  explode->SetPartitionValuesArray(values->GetName());
  explode->SetPartitionNamesArray(names->GetName());

  std::vector<std::string> expectedPartNames;
  for (int partId = 0; partId < details::INPUT_NB_OF_SCALARS; partId++)
  {
    if (partId < truncatedSize)
    {
      expectedPartNames.push_back(truncatedNames->GetValue(partId));
    }
    else
    {
      const std::string& blockname = defaultPartNames[partId];
      expectedPartNames.push_back(blockname);
    }
  }

  // missing entry in value list
  dataset->GetFieldData()->Initialize();
  dataset->GetFieldData()->AddArray(names);
  dataset->GetFieldData()->AddArray(truncatedValues);
  explode->Update();
  bool result = details::CheckOutput(dataset, output, expectedPartNames);

  // missing entry in name list
  dataset->GetFieldData()->Initialize();
  dataset->GetFieldData()->AddArray(truncatedNames);
  dataset->GetFieldData()->AddArray(values);
  explode->Update();
  result = details::CheckOutput(dataset, output, expectedPartNames) && result;

  return result;
}

//------------------------------------------------------------------------------
bool TestPartNames(vtkDataSet* dataset)
{
  vtkLogScopeFunction(INFO);
  const std::string partsNameArrayName = "PartsName";
  const std::string partsValueArrayName = "PartsValue";

  // create some arrays longer than needed
  vtkNew<vtkStringArray> names;
  names->SetName(partsNameArrayName.c_str());
  names->InsertNextValue("one");
  names->InsertNextValue("two");
  names->InsertNextValue("three");
  names->InsertNextValue("four");
  names->InsertNextValue("five");
  names->InsertNextValue("six");
  names->InsertNextValue("seven");
  names->InsertNextValue("height");
  names->InsertNextValue("nine");

  // linear list of same size than name
  vtkNew<vtkAffineArray<double>> values;
  values->SetName(partsValueArrayName.c_str());
  values->SetNumberOfTuples(names->GetNumberOfTuples());
  values->ConstructBackend(1, 0);

  std::vector<std::string> defaultPartNames;
  for (int partId = 0; partId < INPUT_NB_OF_SCALARS; partId++)
  {
    auto blockname = details::INPUT_SCALARS_NAME + "_" + vtk::to_string(partId);
    defaultPartNames.push_back(blockname);
  }

  if (!TestCorrectPartNames(dataset, names, values))
  {
    return false;
  }

  if (!TestArrayNotFound(dataset, names, values, defaultPartNames))
  {
    return false;
  }

  if (!TestPartialArray(dataset, names, values, defaultPartNames))
  {
    return false;
  }

  return true;
}
};

//------------------------------------------------------------------------------
int TestExplodeDataSet(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/waveletMaterial.vti");

  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  // add some data arrays: they should be forwarded
  vtkNew<vtkRandomAttributeGenerator> gen;
  gen->SetInputConnection(reader->GetOutputPort());
  gen->GenerateAllPointDataOff();
  gen->GeneratePointScalarsOn();
  gen->GenerateAllCellDataOff();
  gen->GenerateCellVectorsOn();
  gen->Update();
  vtkDataSet* data = vtkDataSet::SafeDownCast(gen->GetOutputDataObject(0));
  if (!data->GetCellData()->HasArray(details::INPUT_SCALARS_NAME.c_str()))
  {
    vtkLog(ERROR, << "Input does not have the expected scalars. Cannot test");
    return EXIT_FAILURE;
  }

  if (!details::TestDataSet(data))
  {
    vtkLog(ERROR, << "Split fails for image input");
    return EXIT_FAILURE;
  }

  vtkNew<vtkDataSetTriangleFilter> triangulate;
  triangulate->SetInputConnection(reader->GetOutputPort());
  triangulate->Update();

  if (!details::TestDataSet(triangulate->GetOutput()))
  {
    vtkLog(ERROR, << "Split fails for unstructured input");
    return EXIT_FAILURE;
  }

  vtkNew<vtkGeometryFilter> geom;
  geom->SetInputConnection(triangulate->GetOutputPort());
  geom->MergingOff();
  geom->Update();

  if (!details::TestDataSet(geom->GetOutput()))
  {
    vtkLog(ERROR, << "Split fails for polydata input");
    return EXIT_FAILURE;
  }

  if (!details::TestPartNames(data))
  {
    vtkLog(ERROR, << "Fails to name parts");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
