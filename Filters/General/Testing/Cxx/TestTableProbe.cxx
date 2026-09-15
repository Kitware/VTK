// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkDelimitedTextReader.h"
#include "vtkDoubleArray.h"
#include "vtkExtractVOI.h"
#include "vtkHDFReader.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTable.h"
#include "vtkTableProbe.h"
#include "vtkTestUtilities.h"
#include "vtkThreshold.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
const std::string FIELD_NAME = "Field";

const std::vector<double> DATA1 = { 0, 1, 1, 0, 1, 2 };
const std::vector<double> DATA2 = { 1, 1, 0, 0, 2, 2 };
const std::vector<double> DATA3 = { 4, 5, 6, 7, 8, 9 };

//------------------------------------------------------------------------------
void PrintArray(vtkDataArray* array)
{
  std::ostringstream stream;
  for (auto val : vtk::DataArrayValueRange(array))
  {
    stream << val << " ";
  }
  vtkLog(INFO, << "array " << array->GetName() << ": " << stream.str());
}

//------------------------------------------------------------------------------
bool CompareAndPrint(vtkDataArray* result, vtkDataArray* baseline)
{
  const bool compare = vtkTestUtilities::CompareAbstractArray(result, baseline);
  if (!compare)
  {
    vtkLog(INFO, << "result is:");
    PrintArray(result);
    vtkLog(INFO, << "but expected: ");
    PrintArray(result);
  }

  return compare;
}

//------------------------------------------------------------------------------
void AddColumn(const std::vector<double>& values, const std::string& name, vtkTable* table)
{
  vtkNew<vtkDoubleArray> col;
  col->SetName(name.c_str());
  col->SetArray(const_cast<double*>(values.data()), values.size(), 1);
  table->AddColumn(col);
}

/**
 * Create a table where data can be shown as this grid
 * - DATA1 is X axis
 * - DATA2 is Y axis
 * - DATA3 is some point data
 * - x represents missing input point.
 *  x --- 8 --- 9
 *  |     |     |
 *  |     |     |
 *  4 --- 5 --- x
 *  |     |     |
 *  |     |     |
 *  7 --- 6 --- x
 */
void Create2DTable(vtkTable* table)
{
  AddColumn(DATA1, "col1", table);
  AddColumn(DATA2, "col2", table);
  AddColumn(DATA3, FIELD_NAME, table);
}

/**
 * Initialize DataArraySelection with every DataSetAttributes array with enabled state.
 */
void EnableAllArrays(vtkDataArraySelection* selection, vtkDataSetAttributes* attributes)
{
  selection->RemoveAllArrays();
  for (vtkIdType id = 0; id < attributes->GetNumberOfArrays(); id++)
  {
    selection->AddArray(attributes->GetArrayName(id), true);
  }
}

/**
 * Use a source that contains exactly the database table rows.
 */
bool TestExactMatch()
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkTable> table;
  Create2DTable(table);

  vtkNew<vtkTable> inputData;
  inputData->ShallowCopy(table);
  inputData->RemoveColumnByName(FIELD_NAME.c_str());

  vtkNew<vtkTableProbe> tableProbe;
  EnableAllArrays(tableProbe->GetCoordinatesArray(), inputData->GetRowData());
  // first input data is copied to output and will receive the new data array
  tableProbe->SetInputData(inputData);
  // second input is the table where to look for values
  tableProbe->SetInputData(1, table);
  tableProbe->SetAttributeToProcess(vtkDataObject::ROW);
  tableProbe->SetInputArrayToProcess(FIELD_NAME.c_str(), vtkDataObject::ROW);
  tableProbe->Update();

  auto out = vtkTable::SafeDownCast(tableProbe->GetOutput());
  bool ret = vtkTestUtilities::CompareDataObjects(out, table);
  if (!ret)
  {
    PrintArray(vtkDataArray::SafeDownCast(out->GetColumnByName(FIELD_NAME.c_str())));
  }

  return ret;
}

/**
 * Use a source data that has a point on an edge of the database, i.e. one dimension has exact match
 * while another requires interpolation.
 */
bool TestEdgeInterpolation()
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkTable> table;
  Create2DTable(table);

  const int edgePt1Id = 0;
  const int edgePt2Id = 3;

  double edgeCoord[2] = { DATA1[edgePt1Id], (DATA2[edgePt1Id] + DATA2[edgePt2Id]) / 2 };
  double expectedValue = (DATA3[edgePt1Id] + DATA3[edgePt2Id]) / 2;

  vtkNew<vtkDoubleArray> expectedField;
  expectedField->SetName(FIELD_NAME.c_str());
  expectedField->InsertNextValue(expectedValue);

  vtkNew<vtkTable> inputData;
  std::vector<double> col1 = { edgeCoord[0] };
  AddColumn(col1, "col1", inputData);
  std::vector<double> col2 = { edgeCoord[1] };
  AddColumn(col2, "col2", inputData);

  vtkNew<vtkTableProbe> tableProbe;
  EnableAllArrays(tableProbe->GetCoordinatesArray(), inputData->GetRowData());
  // input data is copied to output and will receive the new data array
  tableProbe->SetInputData(inputData);
  tableProbe->SetInputData(1, table);
  tableProbe->SetAttributeToProcess(vtkDataObject::ROW);
  tableProbe->SetInputArrayToProcess(FIELD_NAME.c_str(), vtkDataObject::ROW);

  bool ret = true;
  {
    vtkLogScopeF(INFO, "middle edge");
    tableProbe->Update();

    auto out = tableProbe->GetOutput();
    auto outField = out->GetAttributes(vtkDataObject::ROW)->GetArray(FIELD_NAME.c_str());

    const bool middleEdge = CompareAndPrint(outField, expectedField);
    vtkLogIf(ERROR, !middleEdge, << "middle edge failed");

    ret = middleEdge;
  }

  {
    vtkLogScopeF(INFO, "random parametric pos");
    // try another position on the edge
    const double param = 0.1;
    edgeCoord[1] = DATA2[edgePt1Id] + param * (DATA2[edgePt2Id] - DATA2[edgePt1Id]);
    // replace previous column
    col2 = { edgeCoord[1] };
    inputData->Modified();
    tableProbe->Update();

    expectedField->SetValue(edgePt1Id, (1 - param) * DATA3[edgePt1Id] + param * DATA3[edgePt2Id]);

    auto out = tableProbe->GetOutput();
    auto outField = out->GetAttributes(vtkDataObject::ROW)->GetArray(FIELD_NAME.c_str());
    const bool paramEdge = CompareAndPrint(outField, expectedField);
    vtkLogIf(ERROR, !paramEdge, << "Param edge failed");
    ret &= paramEdge;
  }

  return ret;
}

/**
 * Use source that requires interpolation in all dimensions.
 */
bool TestCellInterpolation()
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkTable> table;
  Create2DTable(table);

  bool ret = true;
  constexpr double coord[2] = { 0.1, 0.2 };
  // parametric coordinates in cell looks same as coord because the cell has origin at 0,0
  constexpr double param[2] = { 0.1, 0.2 };
  // the picked point has 4 surrounding point in the grid. Compute their weights from parametric
  // coordinates.
  constexpr double weights[4] = { (1 - param[0]) * param[1], param[0] * param[1],
    param[0] * (1 - param[1]), (1 - param[0]) * (1 - param[1]) };

  // expected value is the weighted sum of each surrounding point contribution.
  double expectedValue = 0;
  for (int ptId = 0; ptId < 4; ptId++)
  {
    expectedValue += DATA3[ptId] * weights[ptId];
  }

  const bool assertValue = vtkMathUtilities::FuzzyCompare(6.34, expectedValue, 1e-6);
  vtkLogIf(ERROR, !assertValue, << "unexpected expected value computation. Has " << expectedValue
                                << " instead of 6.34");

  ret &= assertValue;

  vtkNew<vtkDoubleArray> expectedField;
  expectedField->SetName(FIELD_NAME.c_str());
  expectedField->InsertNextValue(expectedValue);

  vtkNew<vtkTable> inputData;
  std::vector<double> col1 = { coord[0] };
  AddColumn(col1, "col1", inputData);
  std::vector<double> col2 = { coord[1] };
  AddColumn(col2, "col2", inputData);

  vtkNew<vtkTableProbe> tableProbe;
  EnableAllArrays(tableProbe->GetCoordinatesArray(), inputData->GetRowData());
  // inputData is copied to output and will receive the new data array
  tableProbe->SetInputData(inputData);
  tableProbe->SetInputData(1, table);
  tableProbe->SetAttributeToProcess(vtkDataObject::ROW);
  tableProbe->SetInputArrayToProcess(FIELD_NAME.c_str(), vtkDataObject::ROW);
  tableProbe->Update();

  auto out = tableProbe->GetOutput();
  auto outField = out->GetAttributes(vtkDataObject::ROW)->GetArray(FIELD_NAME.c_str());

  const bool randomPoint = CompareAndPrint(outField, expectedField);
  vtkLogIf(ERROR, !randomPoint, << "random point");

  ret &= randomPoint;
  return ret;
}

/**
 * Use source that requires interpolation in all dimensions.
 */
bool TestOutOfBounds()
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkTable> table;
  Create2DTable(table);

  struct PickPoint
  {
    double X = 0;
    double Y = 0;
  };

  // around the cell at 0,1 that has one missing point.
  const PickPoint insideTriangleCell{ 0.9, 1.1 };
  const PickPoint insideGhostTriangleCell{ 0.1, 1.9 };
  const PickPoint onGhostTriangleEdge{ 0., 1.5 };
  const PickPoint onGhostTriangleVert{ 0., 2. };
  const PickPoint aboveGrid{ 0.1, 2.5 };
  const PickPoint fullyOutside{ -1, -1 };

  // around the cell at 1,0 that has two missing points.
  const PickPoint insideGhostQuad{ 1.5, 0.5 };
  const PickPoint onGhostQuadEdge{ 1.5, 0. };
  const PickPoint onMissingEdgeLine{ 2., 0.1 };

  std::vector<PickPoint> pickPoints = {
    insideTriangleCell,
    insideGhostTriangleCell,
    onGhostTriangleEdge,
    onGhostTriangleVert,
    aboveGrid,
    fullyOutside,
    insideGhostQuad,
    onGhostQuadEdge,
    onMissingEdgeLine,
  };

  vtkNew<vtkTable> tableSource;
  std::vector<double> col1(1);
  AddColumn(col1, "col1", tableSource);
  std::vector<double> col2(1);
  AddColumn(col2, "col2", tableSource);

  vtkNew<vtkDoubleArray> expectedField;
  expectedField->SetName(FIELD_NAME.c_str());
  expectedField->SetNumberOfTuples(1);

  vtkNew<vtkTableProbe> tableProbe;
  EnableAllArrays(tableProbe->GetCoordinatesArray(), tableSource->GetRowData());
  // table source is copied to output and will receive the new data array
  tableProbe->SetInputData(tableSource);
  tableProbe->SetInputData(1, table);
  tableProbe->SetAttributeToProcess(vtkDataObject::ROW);
  tableProbe->SetInputArrayToProcess(FIELD_NAME.c_str(), vtkDataObject::ROW);

  bool ret = true;
  for (const auto point : pickPoints)
  {
    col1[0] = point.X;
    col2[0] = point.Y;
    tableSource->Modified();

    tableProbe->Update();
    auto out = tableProbe->GetOutput();
    auto outField = out->GetAttributes(vtkDataObject::ROW)->GetArray(FIELD_NAME.c_str());
    bool outCheck = std::isnan(outField->GetTuple1(0));
    vtkLogIf(ERROR, !outCheck,
      "wrong value for outside point (" << point.X << ", " << point.Y
                                        << "). Expects NaN but got: " << outField->GetTuple1(0));
    ret &= outCheck;
  }

  return ret;
}

//------------------------------------------------------------------------------
bool TestInputTable()
{
  vtkLogScopeFunction(INFO);

  vtkNew<vtkTable> inputData;
  std::vector<double> col1 = { 0. };
  AddColumn(col1, "col1", inputData);
  std::vector<double> col2 = { 0. };
  AddColumn(col2, "col2", inputData);

  bool ret = true;
  {
    vtkLogScopeF(INFO, "axis-unique-value");
    vtkNew<vtkTable> table;
    AddColumn(DATA1, "col1", table);
    std::vector<double> unique(DATA1.size(), 0.);
    AddColumn(unique, "col2", table);
    AddColumn(DATA3, FIELD_NAME, table);

    vtkNew<vtkTableProbe> tableProbe;
    EnableAllArrays(tableProbe->GetCoordinatesArray(), inputData->GetRowData());
    // first input data is copied to output and will receive the new data array
    tableProbe->SetInputData(inputData);
    // second input is the table where to look for values
    tableProbe->SetInputData(1, table);
    tableProbe->SetAttributeToProcess(vtkDataObject::ROW);
    tableProbe->SetInputArrayToProcess(FIELD_NAME.c_str(), vtkDataObject::ROW);
    tableProbe->Update();

    auto out = tableProbe->GetOutput();
    auto outField = out->GetAttributes(vtkDataObject::ROW)->GetArray(FIELD_NAME.c_str());

    vtkNew<vtkDoubleArray> expectedField;
    expectedField->SetName(FIELD_NAME.c_str());
    expectedField->InsertNextValue(DATA3[3]);
    bool lastValueUsed = CompareAndPrint(outField, expectedField);
    vtkLogIf(ERROR, !lastValueUsed, "axis unique value failed");
    ret &= lastValueUsed;
  }

  {
    vtkLogScopeF(INFO, "duplicated-coordinates");
    vtkNew<vtkTable> table;

    std::vector<double> axis1 = DATA1;
    std::vector<double> axis2 = DATA2;

    // indices 0 and 1 contain the same coordinates
    const int firstOccurence = 0;
    const int duplicatedId = 3;
    axis1[duplicatedId] = axis1[firstOccurence];
    axis2[duplicatedId] = axis2[firstOccurence];

    AddColumn(axis1, "col1", table);
    AddColumn(axis2, "col2", table);
    AddColumn(DATA3, FIELD_NAME, table);

    // probe at the duplicated point
    col1[0] = axis1[duplicatedId];
    col2[0] = axis2[duplicatedId];
    inputData->Modified();

    vtkNew<vtkTableProbe> tableProbe;
    EnableAllArrays(tableProbe->GetCoordinatesArray(), inputData->GetRowData());
    // first input data is copied to output and will receive the new data array
    tableProbe->SetInputData(inputData);
    // second input is the table where to look for values
    tableProbe->SetInputData(1, table);
    tableProbe->SetAttributeToProcess(vtkDataObject::ROW);
    tableProbe->SetInputArrayToProcess(FIELD_NAME.c_str(), vtkDataObject::ROW);
    tableProbe->Update();
    auto out = tableProbe->GetOutput();
    auto outField = out->GetAttributes(vtkDataObject::ROW)->GetArray(FIELD_NAME.c_str());

    vtkNew<vtkDoubleArray> expectedField;
    expectedField->SetName(FIELD_NAME.c_str());
    expectedField->InsertNextValue(DATA3[duplicatedId]);
    bool lastValueUsed = CompareAndPrint(outField, expectedField);
    vtkLogIf(ERROR, !lastValueUsed, "duplicated coordinates failed.");

    ret &= lastValueUsed;
  }

  return ret;
}

//------------------------------------------------------------------------------
bool Test2DCase()
{
  bool ret = TestExactMatch();
  ret &= TestEdgeInterpolation();
  ret &= TestCellInterpolation();
  ret &= TestOutOfBounds();

  return ret;
}

//------------------------------------------------------------------------------
bool Test4DCase(int argc, char* argv[])
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkHDFReader> gridReader;

  std::string gridFileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/harmonics.vtkhdf");
  gridReader->SetFileName(gridFileName.c_str());
  gridReader->Update();

  auto grid = gridReader->GetOutput();
  auto gridAttribute = grid->GetAttributes(vtkDataObject::POINT);

  vtkNew<vtkDelimitedTextReader> tableReader;
  std::string tableFileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/harmonics.csv");
  tableReader->SetFileName(tableFileName.c_str());
  tableReader->SetHaveHeaders(true);
  tableReader->DetectNumericColumnsOn();
  tableReader->Update();
  auto tableSource = tableReader->GetOutput();

  vtkNew<vtkTableProbe> tableProbe;
  tableProbe->SetSourceConnection(tableReader->GetOutputPort());

  const std::vector<std::string> arrays_for_coords = { "X", "Y", "Z", "Time" };
  for (const std::string& array : arrays_for_coords)
  {
    vtkLogIf(ERROR, !tableSource->GetRowData()->HasArray(array.c_str()),
      "missing required input array in table " << array);
    vtkLogIf(ERROR, !gridAttribute->HasArray(array.c_str()),
      << "missing required array in grid " << array);
    tableProbe->GetCoordinatesArray()->AddArray(array.c_str(), true);
  }

  const char* fieldName = "SpatioTemporalHarmonics";
  vtkLogIf(ERROR, !tableSource->GetRowData()->HasArray(fieldName),
    "missing required field array to probe");
  // get SpatioTemporalHarmonics column and add it as PointData on the grid
  tableProbe->SetInputArrayToProcess(fieldName, vtkDataObject::ROW);
  tableProbe->SetAttributeToProcess(vtkDataObject::POINT);

  bool ret = true;
  {
    vtkLogScopeF(INFO, "test NaN");
    tableProbe->SetInputConnection(gridReader->GetOutputPort());
    tableProbe->Update();

    auto out = tableProbe->GetOutput();
    auto outField = out->GetAttributes(vtkDataObject::POINT)->GetArray(fieldName);
    auto fieldRange = vtk::DataArrayValueRange(outField);
    const int nbOfNan = std::count_if(
      fieldRange.begin(), fieldRange.end(), [](double val) { return std::isnan(val); });
    constexpr int expectedNan = 24;
    bool hasNan = nbOfNan == expectedNan;
    vtkLogIf(ERROR, !hasNan, << "Incorrect number of NaN in output: has " << nbOfNan
                             << " but expects " << expectedNan);
    ret &= hasNan;
  }

  {
    vtkLogScopeF(INFO, "test without NaN");
    vtkNew<vtkExtractVOI> voi;
    // remove one row on Y to avoid "NaN" in tableProbe output, so we can use the
    // vtkTestUtilities comparison tools
    voi->SetVOI(0, 5, 0, 4, 0, 3);
    voi->SetInputConnection(gridReader->GetOutputPort());
    voi->Update();
    tableProbe->SetInputConnection(voi->GetOutputPort());
    tableProbe->Update();

    ret &= vtkTestUtilities::RegressionTest(
      argc, argv, tableProbe->GetOutput(), "/Data/BaselineData/TestTableProbe.vtkhdf");
  }

  return ret;
}
};

//------------------------------------------------------------------------------
int TestTableProbe(int argc, char* argv[])
{
  bool ret = ::Test2DCase();
  ret &= ::Test4DCase(argc, argv);
  ret &= ::TestInputTable();

  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
