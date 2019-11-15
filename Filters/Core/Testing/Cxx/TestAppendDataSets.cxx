/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendDataSets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkAppendDataSets.h>
#include <vtkCellData.h>
#include <vtkDataObjectTypes.h>
#include <vtkDataSet.h>
#include <vtkDataSetAttributes.h>
#include <vtkIdList.h>
#include <vtkIdTypeArray.h>
#include <vtkIntArray.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

#include <numeric> // for iota

//////////////////////////////////////////////////////////////////////////////
namespace
{

class DataArrayInfo
{
public:
  std::string Name;
  int NumberOfComponents;
  std::vector<int> Value;

  DataArrayInfo()
  {
    this->Name = std::string("");
    this->NumberOfComponents = 1;
  }
};

//////////////////////////////////////////////////////////////////////////////
// Fill a component of a data array with random values
//////////////////////////////////////////////////////////////////////////////
void FillComponentWithRandom(vtkIntArray* array, int component)
{
  int numberOfComponents = array->GetNumberOfComponents();
  int* values = array->GetPointer(0);
  for (vtkIdType i = 0; i < array->GetNumberOfTuples(); ++i)
  {
    values[i * numberOfComponents + component] = vtkMath::Random() * 100000;
  }
}

//////////////////////////////////////////////////////////////////////////////
// Create a dataset for testing
//////////////////////////////////////////////////////////////////////////////
void CreateDataset(vtkPointSet* dataset, int numberOfPoints,
  const std::vector<DataArrayInfo>& pointArrayInfo, int numberOfCells,
  const std::vector<DataArrayInfo>& cellArrayInfo)
{
  for (size_t i = 0; i < pointArrayInfo.size(); ++i)
  {
    vtkSmartPointer<vtkIntArray> array = vtkSmartPointer<vtkIntArray>::New();
    if (pointArrayInfo[i].Name != "(null)")
    {
      array->SetName(pointArrayInfo[i].Name.c_str());
    }
    array->SetNumberOfComponents(pointArrayInfo[i].NumberOfComponents);
    array->SetNumberOfTuples(numberOfPoints);
    for (size_t c = 0; c < pointArrayInfo[i].Value.size(); ++c)
    {
      FillComponentWithRandom(array, static_cast<int>(c));
    }
    dataset->GetPointData()->AddArray(array);
  }

  for (size_t i = 0; i < cellArrayInfo.size(); ++i)
  {
    vtkSmartPointer<vtkIntArray> array = vtkSmartPointer<vtkIntArray>::New();
    if (cellArrayInfo[i].Name != "(null)")
    {
      array->SetName(cellArrayInfo[i].Name.c_str());
    }
    array->SetNumberOfComponents(cellArrayInfo[i].NumberOfComponents);
    array->SetNumberOfTuples(numberOfCells);
    for (size_t c = 0; c < cellArrayInfo[i].Value.size(); ++c)
    {
      FillComponentWithRandom(array, static_cast<int>(c));
    }
    dataset->GetCellData()->AddArray(array);
  }

  auto datasetPD = vtkPolyData::SafeDownCast(dataset);
  auto datasetUG = vtkUnstructuredGrid::SafeDownCast(dataset);
  if (datasetPD)
  {
    datasetPD->AllocateEstimate(numberOfPoints, 1);
  }
  if (datasetUG)
  {
    datasetUG->Allocate(numberOfPoints);
  }

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkNew<vtkIdList> ids;
  for (vtkIdType i = 0; i < numberOfPoints; ++i)
  {
    points->InsertNextPoint(vtkMath::Random(), vtkMath::Random(), vtkMath::Random());
    ids->InsertId(0, i);
  }

  for (vtkIdType i = 0; i < numberOfCells; ++i)
  {
    // Repeat references to points if needed.
    vtkIdType pointId = i % numberOfPoints;
    if (datasetPD)
    {
      datasetPD->InsertNextCell(VTK_VERTEX, 1, &pointId);
    }
    if (datasetUG)
    {
      datasetUG->InsertNextCell(VTK_VERTEX, 1, &pointId);
    }
  }

  dataset->SetPoints(points);

  // Add global point and cell ids.
  static int next_point_gid = 0;
  if (auto nodeids = vtkSmartPointer<vtkIntArray>::New())
  {
    nodeids->SetName("GlobalNodeIds");
    nodeids->SetNumberOfTuples(numberOfPoints);
    std::iota(nodeids->GetPointer(0), nodeids->GetPointer(0) + numberOfPoints, next_point_gid);
    dataset->GetPointData()->SetGlobalIds(nodeids);

    next_point_gid += numberOfPoints;
  }

  static int next_cell_gid = 0;
  if (auto elementids = vtkSmartPointer<vtkIntArray>::New())
  {
    elementids->SetName("GlobalElementIds");
    elementids->SetNumberOfTuples(numberOfCells);
    std::iota(elementids->GetPointer(0), elementids->GetPointer(0) + numberOfCells, next_cell_gid);
    dataset->GetCellData()->SetGlobalIds(elementids);

    next_cell_gid += numberOfCells;
  }
}

//////////////////////////////////////////////////////////////////////////////
// Utility to compare char arrays that may be nullptr. If one argument is nullptr,
// returns 0 if the other is nullptr too, 1 otherwise. If neither argument is nullptr,
// returns results of strcmp(s1, s2)
//////////////////////////////////////////////////////////////////////////////
int strcmp_null(const char* s1, const char* s2)
{
  if (s1 == nullptr)
  {
    return (s2 != nullptr);
  }
  if (s2 == nullptr)
  {
    return (s1 != nullptr);
  }

  return strcmp(s1, s2);
}

//////////////////////////////////////////////////////////////////////////////
// Prints and checks point/cell data
//////////////////////////////////////////////////////////////////////////////
int PrintAndCheck(const std::vector<vtkDataSet*>& inputs, vtkDataSet* output, int fieldType)
{
  vtkDataSetAttributes* dataArrays = output->GetAttributes(fieldType);
  std::cout << "Evaluating '" << dataArrays->GetClassName() << "'\n";

  for (int arrayIndex = 0; arrayIndex < dataArrays->GetNumberOfArrays(); ++arrayIndex)
  {
    vtkIntArray* outputArray = vtkArrayDownCast<vtkIntArray>(dataArrays->GetArray(arrayIndex));
    const char* outputArrayName = outputArray->GetName();
    std::cout << "Array " << arrayIndex << " - ";
    std::cout << (outputArrayName ? outputArrayName : "(null)") << ": [ ";
    int numTuples = outputArray->GetNumberOfTuples();
    int numComponents = outputArray->GetNumberOfComponents();
    for (int i = 0; i < numTuples; ++i)
    {
      if (numComponents > 1)
        std::cout << "(";
      for (int j = 0; j < numComponents; ++j)
      {
        std::cout << outputArray->GetComponent(i, j);
        if (j < numComponents - 1)
          std::cout << ", ";
      }
      if (numComponents > 1)
        std::cout << ")";
      if (i < numTuples - 1)
        std::cout << ", ";
    }
    std::cout << " ]\n";
  }

  // Test the output
  for (int arrayIndex = 0; arrayIndex < dataArrays->GetNumberOfArrays(); ++arrayIndex)
  {
    vtkIntArray* outputArray = vtkArrayDownCast<vtkIntArray>(dataArrays->GetArray(arrayIndex));
    const char* arrayName = outputArray->GetName();
    if (arrayName == nullptr)
    {
      // Arrays with nullptr names can only come out of the filter if they are designated an
      // attribute. We'll check those later.
      continue;
    }

    // Check that the number of tuples in the output match the sum of
    // the number of tuples in the input.
    vtkIdType numInputTuples = 0;
    for (size_t inputIndex = 0; inputIndex < inputs.size(); ++inputIndex)
    {
      vtkDataArray* array = inputs[inputIndex]->GetAttributes(fieldType)->GetArray(arrayName);
      if (!array)
      {
        std::cerr << "No array named '" << arrayName << "' in input " << inputIndex << "\n";
        return 0;
      }
      numInputTuples += array->GetNumberOfTuples();
    }
    if (numInputTuples != outputArray->GetNumberOfTuples())
    {
      std::cerr
        << "Number of tuples in output does not match total number of tuples in input arrays\n";
      std::cerr << "Expected " << numInputTuples << ", but got " << outputArray->GetNumberOfTuples()
                << std::endl;
      return 0;
    }

    // Now check that the filter placed the tuples in the correct order
    vtkIdType offset = 0;
    for (size_t inputIndex = 0; inputIndex < inputs.size(); ++inputIndex)
    {
      vtkDataArray* array = inputs[inputIndex]->GetAttributes(fieldType)->GetArray(arrayName);
      for (int i = 0; i < array->GetNumberOfTuples(); ++i)
      {
        for (int j = 0; j < array->GetNumberOfComponents(); ++j)
        {
          if (array->GetComponent(i, j) != outputArray->GetComponent(i + offset, j))
          {
            std::cerr << "Mismatched output at output tuple " << i << " component " << j
                      << " in input " << arrayIndex << "\n";
            return 0;
          }
        }
      }
      offset += array->GetNumberOfTuples();
    }
  }

  for (int attributeIndex = 0; attributeIndex < vtkDataSetAttributes::NUM_ATTRIBUTES;
       ++attributeIndex)
  {
    const char* attributeName = vtkDataSetAttributes::GetAttributeTypeAsString(attributeIndex);

    // Check if all of the inputs have an attribute
    vtkDataArray* outputAttributeArray = dataArrays->GetAttribute(attributeIndex);
    if (outputAttributeArray)
    {
      std::cout << "Active attribute '" << attributeName << "' in output: "
                << (outputAttributeArray->GetName() ? outputAttributeArray->GetName() : "(null)")
                << "\n";
    }

    for (size_t inputIndex = 0; inputIndex < inputs.size(); ++inputIndex)
    {
      vtkAbstractArray* inputAttributeArray =
        inputs[inputIndex]->GetAttributes(fieldType)->GetAbstractAttribute(attributeIndex);

      if (outputAttributeArray && !inputAttributeArray)
      {
        std::cerr << "Output had attribute array for '" << attributeName << "' but input "
                  << inputIndex << " did not.\n";
        return 0;
      }
      else if (outputAttributeArray && inputAttributeArray &&
        strcmp_null(outputAttributeArray->GetName(), inputAttributeArray->GetName()) != 0)
      {
        std::cerr << "Output had array '"
                  << (outputAttributeArray->GetName() ? outputAttributeArray->GetName() : "(null)")
                  << "' specified as attribute '" << attributeName << "'\n";
        return 0;
      }
    }

    // Now check whether we should a) have an output array if there isn't one specified
    // and b) the output array has the same name as the input attribute array names (maybe nullptr)
    bool allInputsHaveAttribute = true;
    bool allInputsHaveSameName = true;
    for (size_t inputIndex = 0; inputIndex < inputs.size(); ++inputIndex)
    {
      vtkAbstractArray* inputAttributeArray =
        inputs[inputIndex]->GetAttributes(fieldType)->GetAbstractAttribute(attributeIndex);
      if (!inputAttributeArray)
      {
        allInputsHaveAttribute = false;
        break;
      }
    }
    if (allInputsHaveAttribute)
    {
      vtkAbstractArray* firstAttributeArray =
        inputs[0]->GetAttributes(fieldType)->GetAbstractAttribute(attributeIndex);
      for (size_t inputIndex = 1; inputIndex < inputs.size(); ++inputIndex)
      {
        vtkAbstractArray* inputAttributeArray =
          inputs[inputIndex]->GetAttributes(fieldType)->GetAbstractAttribute(attributeIndex);
        if (strcmp_null(firstAttributeArray->GetName(), inputAttributeArray->GetName()) != 0)
        {
          allInputsHaveSameName = false;
          break;
        }
      }
    }

    if (allInputsHaveAttribute && allInputsHaveSameName)
    {
      const char* attributeArrayName =
        inputs[0]->GetAttributes(fieldType)->GetAbstractAttribute(attributeIndex)->GetName();
      if (!outputAttributeArray)
      {
        std::cerr << "Inputs all have the attribute '" << attributeName << "' set to the name '"
                  << (attributeArrayName ? attributeArrayName : "(null)")
                  << "', but the output does not "
                  << "have this attribute\n";
        return 0;
      }
      else if (strcmp_null(outputAttributeArray->GetName(), attributeArrayName) != 0)
      {
        std::cerr << "Inputs have attribute '" << attributeName << "' set to the name '"
                  << (attributeArrayName ? attributeArrayName : "(null)")
                  << "', but the output attribute "
                  << "has the attribute set to the name '"
                  << (outputAttributeArray->GetName() ? outputAttributeArray->GetName() : "(null)")
                  << "\n";
        return 0;
      }
      else
      {
        // Output attribute array exists and has the right name. Now check the contents
        vtkIdType offset = 0;
        for (size_t inputIndex = 0; inputIndex < inputs.size(); ++inputIndex)
        {
          vtkDataArray* attributeArray =
            inputs[inputIndex]->GetAttributes(fieldType)->GetAttribute(attributeIndex);
          if (!attributeArray)
          {
            continue;
          }
          for (int i = 0; i < attributeArray->GetNumberOfTuples(); ++i)
          {
            for (int j = 0; j < attributeArray->GetNumberOfComponents(); ++j)
            {
              if (attributeArray->GetComponent(i, j) !=
                outputAttributeArray->GetComponent(i + offset, j))
              {
                std::cerr << "Mismatched output in attribute at output tuple " << i << "component "
                          << j << " in input " << inputIndex << "\n";
                return 0;
              }
            }
          }
          offset += attributeArray->GetNumberOfTuples();
        }
      }
    }
  }

  return 1;
}

//////////////////////////////////////////////////////////////////////////////
// Returns 1 on success, 0 otherwise
//////////////////////////////////////////////////////////////////////////////
int AppendDatasetsAndCheckMergedArrayLengths(vtkAppendDataSets* append)
{
  append->MergePointsOn();
  append->Update();
  vtkDataSet* output = append->GetOutput();

  if (output->GetPointData()->GetNumberOfArrays() > 0 &&
    output->GetPointData()->GetArray(0)->GetNumberOfTuples() != output->GetNumberOfPoints())
  {
    std::cerr << "Wrong number of tuples in output point data arrays\n";
    return 0;
  }

  if (output->GetCellData()->GetNumberOfArrays() > 0 &&
    output->GetCellData()->GetArray(0)->GetNumberOfTuples() != output->GetNumberOfCells())
  {
    std::cerr << "Wrong number of tuples in output cell data arrays\n";
    return 0;
  }

  if (output->GetPointData()->GetGlobalIds() != nullptr)
  {
    std::cerr << "Point global ids should have been discarded after merge!\n";
    return 0;
  }
  if (output->GetCellData()->GetGlobalIds() == nullptr)
  {
    std::cerr << "Cell global ids should have been preserved after merge!\n";
    return 0;
  }

  return 1;
}

//////////////////////////////////////////////////////////////////////////////
// Returns 1 on success, 0 otherwise
//////////////////////////////////////////////////////////////////////////////
int AppendDatasetsAndPrint(const std::vector<vtkDataSet*>& inputs, const char* expectedDataSetType)
{
  vtkNew<vtkAppendDataSets> append;
  for (size_t inputIndex = 0; inputIndex < inputs.size(); ++inputIndex)
  {
    append->AddInputData(inputs[inputIndex]);
  }
  append->SetOutputDataSetType(vtkDataObjectTypes::GetTypeIdFromClassName(expectedDataSetType));
  append->Update();
  vtkDataSet* output = append->GetOutput();
  if (!output->IsA(expectedDataSetType))
  {
    std::cerr << "Output dataset type is " << output->GetClassName() << " but is expected to be "
              << expectedDataSetType << std::endl;
    return 0;
  }

  if (!PrintAndCheck(inputs, output, vtkDataObject::POINT))
  {
    return 0;
  }
  if (!PrintAndCheck(inputs, output, vtkDataObject::CELL))
  {
    return 0;
  }

  if (output->GetPointData()->GetGlobalIds() == nullptr)
  {
    std::cerr << "Point global ids should have been preserved!\n";
    return 0;
  }

  if (output->GetCellData()->GetGlobalIds() == nullptr)
  {
    std::cerr << "Cell global ids should have been preserved!\n";
    return 0;
  }

  return AppendDatasetsAndCheckMergedArrayLengths(append);
}

bool TestToleranceModes(vtkDataSet* dataset1, vtkDataSet* dataset2)
{
  // Set the tolerance to one quarter of the length of the data set, which is 4.0.
  // This equates to an absolute tolerance of 1.0, which should cause the first two
  // points in the dataset to be merged.

  std::cout << "Testing merging with relative tolerance for '" << dataset1->GetClassName() << "'\n";

  double tolerance = 0.25;
  vtkNew<vtkAppendDataSets> append;
  append->MergePointsOn();
  append->SetTolerance(tolerance);
  append->ToleranceIsAbsoluteOff();
  append->AddInputData(dataset1);
  append->AddInputData(dataset2);
  append->Update();

  auto output = append->GetOutput();
  for (vtkIdType i = 0; i < output->GetNumberOfPoints(); ++i)
  {
    double point[3];
    output->GetPoint(i, point);
    std::cout << "Point " << i << ": " << point[0] << ", " << point[1] << ", " << point[2]
              << std::endl;
  }

  if (output->GetNumberOfPoints() != 2)
  {
    std::cerr << "Point merging with relative tolerance yielded " << output->GetNumberOfPoints()
              << " points instead of 2.\n";
    return false;
  }

  // Test out absolute tolerance
  std::cout << "Testing merging with absolute tolerance." << std::endl;
  append->ToleranceIsAbsoluteOn();
  append->Update();

  output = append->GetOutput();
  for (vtkIdType i = 0; i < output->GetNumberOfPoints(); ++i)
  {
    double point[3];
    output->GetPoint(i, point);
    std::cout << "Point " << i << ": " << point[0] << ", " << point[1] << ", " << point[2]
              << std::endl;
  }

  if (output->GetNumberOfPoints() != 3)
  {
    std::cerr << "Point merging with absolute tolerance yielded " << output->GetNumberOfPoints()
              << " points instead of 3.\n";
    return false;
  }

  return true;
}

bool TestToleranceModes()
{
  vtkNew<vtkPoints> points1;
  points1->InsertNextPoint(0.0, 0.0, 0.0);
  points1->InsertNextPoint(0.0, 1.0, 0.0);

  vtkNew<vtkPoints> points2;
  points2->InsertNextPoint(0.0, 1.0, 0.0);
  points2->InsertNextPoint(0.0, 4.0, 0.0);

  vtkIdType ptIds[] = { 0, 1 };

  vtkNew<vtkPolyData> polydata1;
  polydata1->AllocateEstimate(3, 10);
  polydata1->SetPoints(points1);
  polydata1->InsertNextCell(VTK_LINE, 2, ptIds);

  vtkNew<vtkPolyData> polydata2;
  polydata2->AllocateEstimate(3, 10);
  polydata2->SetPoints(points2);
  polydata2->InsertNextCell(VTK_LINE, 2, ptIds);

  if (!TestToleranceModes(polydata1, polydata2))
  {
    std::cerr << "Failed testing tolerance mode for 'vtkPolyData'\n";
    return false;
  }

  vtkNew<vtkUnstructuredGrid> ugrid1;
  ugrid1->Allocate(3, 10);
  ugrid1->SetPoints(points1);
  ugrid1->InsertNextCell(VTK_LINE, 2, ptIds);

  vtkNew<vtkUnstructuredGrid> ugrid2;
  ugrid2->Allocate(3, 10);
  ugrid2->SetPoints(points2);
  ugrid2->InsertNextCell(VTK_LINE, 2, ptIds);

  if (!TestToleranceModes(ugrid1, ugrid2))
  {
    std::cerr << "Failed testing tolerance mode for 'vtkUnstructuredGrid'\n";
    return false;
  }

  return true;
}

} // end anonymous namespace

//////////////////////////////////////////////////////////////////////////////
int TestAppendDataSets(int, char*[])
{
  // Set up d1 data object
  std::vector<DataArrayInfo> d1PointInfo(2, DataArrayInfo());
  d1PointInfo[0].Name = "A";
  d1PointInfo[0].Value = std::vector<int>(1, 1);

  d1PointInfo[1].Name = "B";
  d1PointInfo[1].Value = std::vector<int>(1, 2);

  std::vector<DataArrayInfo> d1CellInfo(2, DataArrayInfo());
  d1CellInfo[0].Name = "a";
  d1CellInfo[0].Value = std::vector<int>(1, 1);

  d1CellInfo[1].Name = "b";
  d1CellInfo[1].Value = std::vector<int>(1, 2);

  vtkNew<vtkPolyData> d1;
  int d1NumberOfPoints = 3;
  int d1NumberOfCells = 7;
  CreateDataset(d1, d1NumberOfPoints, d1PointInfo, d1NumberOfCells, d1CellInfo);

  // Set up d2 data object
  std::vector<DataArrayInfo> d2PointInfo(3, DataArrayInfo());
  d2PointInfo[0].Name = "A";
  d2PointInfo[0].Value = std::vector<int>(1, 3);

  d2PointInfo[1].Name = "B";
  d2PointInfo[1].Value = std::vector<int>(1, 4);

  d2PointInfo[2].Name = "C";
  d2PointInfo[2].Value = std::vector<int>(1, 5);

  std::vector<DataArrayInfo> d2CellInfo(2, DataArrayInfo());
  d2CellInfo[0].Name = "b";
  d2CellInfo[0].Value = std::vector<int>(1, 4);

  d2CellInfo[1].Name = "a";
  d2CellInfo[1].Value = std::vector<int>(1, 3);

  vtkNew<vtkPolyData> d2;
  int d2NumberOfPoints = 7;
  int d2NumberOfCells = 9;
  CreateDataset(d2, d2NumberOfPoints, d2PointInfo, d2NumberOfCells, d2CellInfo);

  // This tests that the active attributes are ignored when appending data sets, but
  // that the active attributes in the output are set to the active attributes in
  // the input only if all inputs designate the same active attribute.

  // Now append these datasets and print the results
  std::cout << "===========================================================\n";
  std::cout << "Append result with no active scalars: " << std::endl;
  std::vector<vtkDataSet*> inputs(2, static_cast<vtkDataSet*>(nullptr));
  inputs[0] = d1;
  inputs[1] = d2;
  if (!AppendDatasetsAndPrint(inputs, "vtkPolyData"))
  {
    std::cerr << "vtkAppendDataSets failed with no active scalars\n";
    return EXIT_FAILURE;
  }

  // Set the active scalars in the first dataset to "A" and the active scalars in
  // the second dataset to "B".
  d1->GetPointData()->SetActiveScalars("A");
  d1->GetCellData()->SetActiveScalars("a");
  d2->GetPointData()->SetActiveScalars("B");
  d2->GetCellData()->SetActiveScalars("b");

  std::cout << "===========================================================\n";
  std::cout << "Append result with 'A' active scalar in D1, 'B' active scalar in D2: " << std::endl;
  if (!AppendDatasetsAndPrint(inputs, "vtkPolyData"))
  {
    std::cerr << "vtkAppendDataSets failed with active scalar 'A' in D1, active scalar 'B' in D2\n";
    return EXIT_FAILURE;
  }

  // Set the active scalars in the first dataset to "B" and the active scalars in
  // the second dataset to "A".
  d1->GetPointData()->SetActiveScalars("B");
  d1->GetCellData()->SetActiveScalars("b");
  d2->GetPointData()->SetActiveScalars("A");
  d2->GetCellData()->SetActiveScalars("a");

  std::cout << "===========================================================\n";
  std::cout << "Append result with 'B' active scalar in D1, 'A' active scalar in D2: " << std::endl;
  if (!AppendDatasetsAndPrint(inputs, "vtkPolyData"))
  {
    std::cerr << "vtkAppendDataSets failed with active scalar 'B' in D1, active scalar 'A' in D2\n";
    return EXIT_FAILURE;
  }

  // Set the active scalars in both datasets to "A"
  d1->GetPointData()->SetActiveScalars("A");
  d1->GetCellData()->SetActiveScalars("a");
  d2->GetPointData()->SetActiveScalars("A");
  d2->GetCellData()->SetActiveScalars("a");

  std::cout << "===========================================================\n";
  std::cout << "Append result with A active scalar in D1 and D2: " << std::endl;
  if (!AppendDatasetsAndPrint(inputs, "vtkPolyData"))
  {
    std::cerr << "vtkAppendDataSets failed with active scalar 'A' in D1, active scalar 'A' in D2\n";
    return EXIT_FAILURE;
  }

  // Set the active scalars in both datasets to "B"
  d1->GetPointData()->SetActiveScalars("B");
  d1->GetCellData()->SetActiveScalars("b");
  d2->GetPointData()->SetActiveScalars("B");
  d2->GetCellData()->SetActiveScalars("b");

  std::cout << "===========================================================\n";
  std::cout << "Append result with B active scalar in D1 and D2: " << std::endl;
  if (!AppendDatasetsAndPrint(inputs, "vtkPolyData"))
  {
    std::cerr << "vtkAppendDataSets failed with active scalar 'B' in D1, active scalar 'B' in D2\n";
    return EXIT_FAILURE;
  }

  std::vector<DataArrayInfo> d3PointInfo(3, DataArrayInfo());
  d3PointInfo[0].Name = "3";
  d3PointInfo[0].Value = std::vector<int>(1, 3);

  d3PointInfo[1].Name = "4";
  d3PointInfo[1].Value = std::vector<int>(1, 4);

  d3PointInfo[2].Name = "5";
  d3PointInfo[2].Value = std::vector<int>(1, 5);

  std::vector<DataArrayInfo> d3CellInfo(2, DataArrayInfo());
  d3CellInfo[0].Name = "3";
  d3CellInfo[0].Value = std::vector<int>(1, 3);

  d3CellInfo[1].Name = "4";
  d3CellInfo[1].Value = std::vector<int>(1, 4);

  vtkNew<vtkPolyData> d3;
  int d3NumberOfPoints = 4;
  int d3NumberOfCells = 8;
  CreateDataset(d3, d3NumberOfPoints, d3PointInfo, d3NumberOfCells, d3CellInfo);

  // No common arrays
  std::cout << "===========================================================\n";
  std::cout << "Append result with no common array names and no active scalars: " << std::endl;
  inputs[0] = d1;
  inputs[1] = d3;
  if (!AppendDatasetsAndPrint(inputs, "vtkPolyData"))
  {
    std::cerr << "vtkAppendDataSets failed with no common array names and no active scalars\n";
    return EXIT_FAILURE;
  }

  // Test appending of nullptr array names with active scalars
  std::vector<DataArrayInfo> d4PointInfo(2, DataArrayInfo());
  d4PointInfo[0].Name = "(null)";
  d4PointInfo[0].Value = std::vector<int>(1, 10);

  d4PointInfo[1].Name = "Q";
  d4PointInfo[1].Value = std::vector<int>(1, 11);

  std::vector<DataArrayInfo> d4CellInfo(2, DataArrayInfo());
  d4CellInfo[0].Name = "(null)";
  d4CellInfo[0].Value = std::vector<int>(1, 10);

  d4CellInfo[1].Name = "Q";
  d4CellInfo[1].Value = std::vector<int>(1, 11);

  vtkNew<vtkPolyData> d4;
  int d4NumberOfPoints = 6;
  int d4NumberOfCells = 10;
  CreateDataset(d4, d4NumberOfPoints, d4PointInfo, d4NumberOfCells, d4CellInfo);

  // Set scalars to array whose name is nullptr
  d4->GetPointData()->SetScalars(d4->GetPointData()->GetArray(0));
  d4->GetCellData()->SetScalars(d4->GetCellData()->GetArray(0));

  std::vector<DataArrayInfo> d5PointInfo(2, DataArrayInfo());
  d5PointInfo[0].Name = "Q";
  d5PointInfo[0].Value = std::vector<int>(1, 12);

  d5PointInfo[1].Name = "(null)";
  d5PointInfo[1].Value = std::vector<int>(1, 13);

  std::vector<DataArrayInfo> d5CellInfo(2, DataArrayInfo());
  d5CellInfo[0].Name = "Q";
  d5CellInfo[0].Value = std::vector<int>(1, 12);

  d5CellInfo[1].Name = "(null)";
  d5CellInfo[1].Value = std::vector<int>(1, 13);

  vtkNew<vtkPolyData> d5;
  int d5NumberOfPoints = 6;
  int d5NumberOfCells = 3;
  CreateDataset(d5, d5NumberOfPoints, d5PointInfo, d5NumberOfCells, d5CellInfo);

  // Set scalars to array whose name is nullptr
  d5->GetPointData()->SetScalars(d5->GetPointData()->GetArray(1));
  d5->GetCellData()->SetScalars(d5->GetCellData()->GetArray(1));

  std::cout << "===========================================================\n";
  std::cout << "Append result of scalar arrays with nullptr names: " << std::endl;
  inputs[0] = d4;
  inputs[1] = d5;
  if (!AppendDatasetsAndPrint(inputs, "vtkPolyData"))
  {
    std::cerr << "vtkAppendDataSets failed with scalar arrays with nullptr names\n";
    return EXIT_FAILURE;
  }

  std::vector<DataArrayInfo> d6PointInfo(1, DataArrayInfo());
  d6PointInfo[0].Name = "Q";
  d6PointInfo[0].NumberOfComponents = 2;
  d6PointInfo[0].Value = std::vector<int>(2, 14);

  std::vector<DataArrayInfo> d6CellInfo(1, DataArrayInfo());
  d6CellInfo[0].Name = "Q";
  d6CellInfo[0].NumberOfComponents = 2;
  d6CellInfo[0].Value = std::vector<int>(2, 14);

  vtkNew<vtkPolyData> d6;
  int d6NumberOfPoints = 9;
  int d6NumberOfCells = 4;
  CreateDataset(d6, d6NumberOfPoints, d6PointInfo, d6NumberOfCells, d6CellInfo);

  std::vector<DataArrayInfo> d7PointInfo(1, DataArrayInfo());
  d7PointInfo[0].Name = "Q";
  d7PointInfo[0].NumberOfComponents = 2;
  d7PointInfo[0].Value = std::vector<int>(2, 15);

  std::vector<DataArrayInfo> d7CellInfo(1, DataArrayInfo());
  d7CellInfo[0].Name = "Q";
  d7CellInfo[0].NumberOfComponents = 2;
  d7CellInfo[0].Value = std::vector<int>(2, 15);

  vtkNew<vtkPolyData> d7;
  int d7NumberOfPoints = 5;
  int d7NumberOfCells = 7;
  CreateDataset(d7, d7NumberOfPoints, d7PointInfo, d7NumberOfCells, d7CellInfo);

  std::cout << "===========================================================\n";
  std::cout << "Append result of scalar arrays with 2 components: " << std::endl;
  inputs[0] = d6;
  inputs[1] = d7;
  if (!AppendDatasetsAndPrint(inputs, "vtkPolyData"))
  {
    std::cerr << "vtkAppendDataSets failed with scalar arrays with 2 components\n";
    return EXIT_FAILURE;
  }

  std::vector<DataArrayInfo> d8PointInfo(1, DataArrayInfo());
  d8PointInfo[0].Name = "Q";
  d8PointInfo[0].Value = std::vector<int>(1, 16);

  std::vector<DataArrayInfo> d8CellInfo(1, DataArrayInfo());
  d8CellInfo[0].Name = "Q";
  d8CellInfo[0].Value = std::vector<int>(1, 16);

  vtkNew<vtkPolyData> d8;
  int d8NumberOfPoints = 11;
  int d8NumberOfCells = 8;
  CreateDataset(d8, d8NumberOfPoints, d8PointInfo, d8NumberOfCells, d8CellInfo);

  std::cout << "===========================================================\n";
  std::cout << "Append result of scalar arrays with same name but different number of components: "
            << std::endl;
  inputs[0] = d7;
  inputs[1] = d8;
  if (!AppendDatasetsAndPrint(inputs, "vtkPolyData"))
  {
    std::cerr
      << "vtkAppendDataSets failed with scalar arrays with same name but different components\n";
    return EXIT_FAILURE;
  }

  std::cout << "===========================================================\n";
  std::cout << "Append result of deep copied dataset: " << std::endl;
  inputs[0] = d7;
  d8->DeepCopy(d7);
  inputs[1] = d8;
  if (!AppendDatasetsAndPrint(inputs, "vtkPolyData"))
  {
    std::cerr << "vtkAppendDataSets failed with deep copied datasets\n";
    return EXIT_FAILURE;
  }

  // Append unstructured grid with polydata
  std::vector<DataArrayInfo> d9PointInfo(1, DataArrayInfo());
  d9PointInfo[0].Name = "Q";
  d9PointInfo[0].Value = std::vector<int>(1, 16);

  std::vector<DataArrayInfo> d9CellInfo(1, DataArrayInfo());
  d9CellInfo[0].Name = "Q";
  d9CellInfo[0].Value = std::vector<int>(1, 16);

  vtkNew<vtkUnstructuredGrid> d9;
  int d9NumberOfPoints = 12;
  int d9NumberOfCells = 4;
  CreateDataset(d9, d9NumberOfPoints, d9PointInfo, d9NumberOfCells, d9CellInfo);

  std::cout << "===========================================================\n";
  std::cout << "Append unstructured grid and polydata: " << std::endl;
  inputs[0] = d9;
  inputs[1] = d8;
  if (!AppendDatasetsAndPrint(inputs, "vtkUnstructuredGrid"))
  {
    std::cerr << "vtkAppendDataSets failed with unstructured grid and polydata\n";
    return EXIT_FAILURE;
  }

  std::cout << "===========================================================\n";
  std::cout << "Testing tolerance modes:\n";
  if (!TestToleranceModes())
  {
    std::cerr << "vtkAppendFilter failed testing tolerances.\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
