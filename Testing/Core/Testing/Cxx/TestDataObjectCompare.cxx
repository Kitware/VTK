// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTestUtilities.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDataAssembly.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenerateGlobalIds.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridDepthLimiter.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMatrix3x3.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkTesting.h"
#include "vtkTriangleFilter.h"
#include "vtkTypeName.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVariantArray.h"
#include "vtkXMLHyperTreeGridReader.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLPartitionedDataSetCollectionReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkXMLStructuredGridReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace
{
//------------------------------------------------------------------------------
template <class ObjectT>
void CheckErrorMessage(bool success, std::ostringstream& logStream, std::string&& message,
  std::vector<std::string>& retLog, std::string&& query)
{
  if (success)
  {
    retLog.emplace_back(
      "Could not catch a wrong " + query + " in " + vtk::TypeName<ObjectT>() + ".");
  }

  std::string logString = logStream.str();
  std::size_t found = logString.find(message);
  logStream.str(""); // clearing
  if (found == std::string::npos)
  {
    retLog.emplace_back("Missing error message for wrong " + query + " in " +
      vtk::TypeName<ObjectT>() + ": \"" + message + "\"");
  }
}

//------------------------------------------------------------------------------
void ShrinkFieldData(vtkFieldData* fd, vtkIdType newSize)
{
  for (int arrayId = 0; arrayId < fd->GetNumberOfArrays(); ++arrayId)
  {
    fd->GetAbstractArray(arrayId)->SetNumberOfTuples(newSize);
  }
}

//------------------------------------------------------------------------------
void ShrinkFieldData(vtkDataSet* ds)
{
  ShrinkFieldData(ds->GetPointData(), ds->GetNumberOfPoints());
  ShrinkFieldData(ds->GetCellData(), ds->GetNumberOfCells());
}

//------------------------------------------------------------------------------
void ShrinkFieldData(vtkHyperTreeGrid* htg)
{
  ShrinkFieldData(htg->GetCellData(), htg->GetNumberOfCells());
}

//------------------------------------------------------------------------------
void TurnOffLogging(std::ostringstream& logStream)
{
  auto stream_sink = [](void* userData, const vtkLogger::Message& message)
  {
    std::ostream& s = *reinterpret_cast<std::ostream*>(userData);
    s << message.preamble << message.message << std::endl;
  };
  vtkLogger::AddCallback("logStream", stream_sink, &logStream, vtkLogger::VERBOSITY_ERROR);
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_OFF);
}

//------------------------------------------------------------------------------
void TurnOnLogging()
{
  vtkLogger::RemoveCallback("logStream");
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_INFO);
}

//------------------------------------------------------------------------------
vtkNew<vtkDoubleArray> GenerateArray(vtkFieldData* fd, vtkIdType size, double firstValue)
{
  vtkNew<vtkDoubleArray> array;
  array->SetName("Array");
  array->SetNumberOfValues(size);
  array->SetValue(0, firstValue);
  for (vtkIdType id = 1; id < size; ++id)
  {
    array->SetValue(id, id);
  }

  fd->AddArray(array);

  return array;
}

//------------------------------------------------------------------------------
bool ComparePoints(vtkDataSet* ds1, vtkDataSet* ds2)
{
  if (!vtkTestUtilities::ComparePoints(ds1, ds2))
  {
    vtkLog(ERROR, "Points should be similar, but they are not.");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool ComparePoints(vtkHyperTreeGrid*, vtkHyperTreeGrid*)
{
  return true;
}

//------------------------------------------------------------------------------
template <class DataSetT>
void TestExtent(DataSetT* ds, std::ostringstream& logStream, std::vector<std::string>& retLog)
{
  auto other = vtkSmartPointer<DataSetT>::Take(ds->NewInstance());
  other->DeepCopy(ds);
  int extent[6];
  other->GetExtent(extent);
  ++extent[0];
  ++extent[2];
  ++extent[4];
  --extent[1];
  --extent[3];
  --extent[5];
  other->SetExtent(extent);
  ShrinkFieldData(other);

  CheckErrorMessage<DataSetT>(vtkTestUtilities::CompareDataObjects(ds, other), logStream,
    "Extent doesn't match between the 2 input " + vtk::TypeName<DataSetT>(), retLog, "Extent");

  if (!std::is_same<DataSetT, vtkHyperTreeGrid>::value)
  {
    CheckErrorMessage<DataSetT>(ComparePoints(ds, other), logStream,
      "Extent doesn't match between the 2 input " + vtk::TypeName<DataSetT>(), retLog, "Extent");
  }
}

//------------------------------------------------------------------------------
template <class RectilinearGridT>
void TestCoords(
  RectilinearGridT* rg, std::ostringstream& logStream, std::vector<std::string>& retLog)
{
  auto other = vtkSmartPointer<RectilinearGridT>::Take(rg->NewInstance());
  other->DeepCopy(rg);
  auto xCoords = vtkArrayDownCast<vtkDoubleArray>(other->GetXCoordinates());
  auto yCoords = vtkArrayDownCast<vtkDoubleArray>(other->GetYCoordinates());
  auto zCoords = vtkArrayDownCast<vtkDoubleArray>(other->GetZCoordinates());
  xCoords->SetValue(0, xCoords->GetValue(0) - 1);
  yCoords->SetValue(0, yCoords->GetValue(0) - 1);
  zCoords->SetValue(0, zCoords->GetValue(0) - 1);

  CheckErrorMessage<RectilinearGridT>(vtkTestUtilities::CompareDataObjects(rg, other), logStream,
    "Structure doesn't match between the 2 input " + vtk::TypeName<RectilinearGridT>(), retLog,
    "Coordinate");

  if (!std::is_same<RectilinearGridT, vtkHyperTreeGrid>::value)
  {
    CheckErrorMessage<RectilinearGridT>(ComparePoints(rg, other), logStream,
      "Structure doesn't match between the 2 input " + vtk::TypeName<RectilinearGridT>(), retLog,
      "Coordinate");
  }

  // Testing messing on the coordinate sizes
  xCoords->SetNumberOfValues(xCoords->GetNumberOfValues() - 1);
  CheckErrorMessage<RectilinearGridT>(vtkTestUtilities::CompareDataObjects(rg, other), logStream,
    "Not right number of coordinates in dimension 0", retLog, "Coordinates size");
  xCoords->ShallowCopy(rg->GetXCoordinates());

  if (!std::is_same<RectilinearGridT, vtkHyperTreeGrid>::value)
  {
    xCoords->SetNumberOfValues(xCoords->GetNumberOfValues() - 1);
    CheckErrorMessage<RectilinearGridT>(ComparePoints(rg, other), logStream,
      "Not right number of coordinates in dimension 0", retLog, "Coordinates size");
    xCoords->ShallowCopy(rg->GetXCoordinates());
  }

  yCoords->SetNumberOfValues(yCoords->GetNumberOfValues() - 1);
  CheckErrorMessage<RectilinearGridT>(vtkTestUtilities::CompareDataObjects(rg, other), logStream,
    "Not right number of coordinates in dimension 1", retLog, "Coordinates size");
  yCoords->ShallowCopy(rg->GetYCoordinates());

  if (!std::is_same<RectilinearGridT, vtkHyperTreeGrid>::value)
  {
    yCoords->SetNumberOfValues(yCoords->GetNumberOfValues() - 1);
    CheckErrorMessage<RectilinearGridT>(ComparePoints(rg, other), logStream,
      "Not right number of coordinates in dimension 1", retLog, "Coordinates size");
    yCoords->ShallowCopy(rg->GetYCoordinates());
  }

  zCoords->SetNumberOfValues(zCoords->GetNumberOfValues() - 1);
  CheckErrorMessage<RectilinearGridT>(vtkTestUtilities::CompareDataObjects(rg, other), logStream,
    "Not right number of coordinates in dimension 2", retLog, "Coordinates size");

  if (!std::is_same<RectilinearGridT, vtkHyperTreeGrid>::value)
  {
    zCoords->SetNumberOfValues(zCoords->GetNumberOfValues() - 1);
    CheckErrorMessage<RectilinearGridT>(ComparePoints(rg, other), logStream,
      "Not right number of coordinates in dimension 2", retLog, "Coordinates size");
  }
}

//------------------------------------------------------------------------------
template <class DataSetT, class FieldDataT>
void TestFieldDataFailures(DataSetT* ds1, DataSetT* ds2, FieldDataT* fd1, FieldDataT* fd2,
  vtkIdType n, std::ostringstream& logStream, std::vector<std::string>& retLog)
{
  auto array1 = GenerateArray(fd1, n, 1.0);
  auto array2 = GenerateArray(fd2, n, 2.0);

  CheckErrorMessage<DataSetT>(vtkTestUtilities::CompareDataObjects(ds1, ds2), logStream,
    "Array mismatch for Array in input", retLog, "Tuples in " + std::string(fd1->GetClassName()));

  CheckErrorMessage<FieldDataT>(vtkTestUtilities::CompareFieldData(fd1, fd2), logStream,
    "Array mismatch for Array in input", retLog, "Tuples");

  if (std::is_same<FieldDataT, vtkPointData>::value)
  {
    CheckErrorMessage<FieldDataT>(
      ComparePoints(ds1, ds2), logStream, "Array mismatch for Array in input", retLog, "Tuples");
  }
  else if (std::is_same<FieldDataT, vtkCellData>::value)
  {
    CheckErrorMessage<FieldDataT>(vtkTestUtilities::CompareCells(ds1, ds2), logStream,
      "Array mismatch for Array in input", retLog, "Tuples");
  }

  if (vtkUnsignedCharArray* ghosts1 = fd1->GetGhostArray())
  {
    vtkUnsignedCharArray* ghosts2 = fd2->GetGhostArray();

    if (!ghosts2)
    {
      retLog.emplace_back(
        "One ghost array is nullptr while the other is not in " + std::string(fd1->GetClassName()));
      return;
    }

    unsigned char ghostsToSkip1 = fd1->GetGhostsToSkip(), ghostsToSkip2 = fd2->GetGhostsToSkip();

    if (ghostsToSkip1 != ghostsToSkip2)
    {
      retLog.emplace_back("Ghosts to skip do not match in " + std::string(fd1->GetClassName()));
    }

    // There is nothing to test here
    if (!ghostsToSkip1)
    {
      return;
    }

    ghosts1->SetValue(0, ghostsToSkip1);
    ghosts2->SetValue(0, ghostsToSkip2);

    array1->SetValue(0, 10);
    array2->SetValue(0, 10);

    if (!vtkTestUtilities::CompareDataObjects(ds1, ds2))
    {
      retLog.emplace_back("Mismatch on ghost to skip should have been ignored in " +
        std::string(fd1->GetClassName()) + " in " + ds1->GetClassName());
    }

    if (!vtkTestUtilities::CompareFieldData(fd1, fd2))
    {
      retLog.emplace_back("Mismatch on ghost to skip should have been ignored in " +
        std::string(fd1->GetClassName()));
    }

    ghosts1->SetValue(0, 0);
    ghosts2->SetValue(0, 1);

    CheckErrorMessage<DataSetT>(vtkTestUtilities::CompareDataObjects(ds1, ds2), logStream,
      "Ghost arrays in " + std::string(fd1->GetClassName()) + " do not match.", retLog,
      "Ghost Values");

    CheckErrorMessage<FieldDataT>(vtkTestUtilities::CompareFieldData(fd1, fd2), logStream,
      "Ghost arrays in " + std::string(fd1->GetClassName()) + " do not match.", retLog,
      "Ghost Values");

    fd1->SetGhostsToSkip(1);
    fd2->SetGhostsToSkip(2);

    CheckErrorMessage<DataSetT>(vtkTestUtilities::CompareDataObjects(ds1, ds2), logStream,
      "Ghost element status of the 2 input " + std::string(fd1->GetClassName()) + " do not match.",
      retLog, "Ghosts To Skip");

    CheckErrorMessage<FieldDataT>(vtkTestUtilities::CompareFieldData(fd1, fd2), logStream,
      "Ghost element status of the 2 input " + std::string(fd1->GetClassName()) + " do not match.",
      retLog, "Ghosts To Skip");
  }
}

//------------------------------------------------------------------------------
template <class DataSetT>
void TestDataFailures(DataSetT* ds, std::ostringstream& logStream, std::vector<std::string>& retLog)
{
  auto other1 = vtkSmartPointer<DataSetT>::Take(ds->NewInstance());
  auto other2 = vtkSmartPointer<DataSetT>::Take(ds->NewInstance());

  other1->DeepCopy(ds);
  other2->DeepCopy(ds);
  TestFieldDataFailures(other1.GetPointer(), other2.GetPointer(), other1->GetPointData(),
    other2->GetPointData(), other2->GetNumberOfPoints(), logStream, retLog);

  other1->DeepCopy(ds);
  other2->DeepCopy(ds);
  TestFieldDataFailures(other1.GetPointer(), other2.GetPointer(), other1->GetCellData(),
    other2->GetCellData(), ds->GetNumberOfCells(), logStream, retLog);

  other1->DeepCopy(ds);
  other2->DeepCopy(ds);
  TestFieldDataFailures(other1.GetPointer(), other2.GetPointer(), other1->GetFieldData(),
    other2->GetFieldData(), 10, logStream, retLog);
}

//------------------------------------------------------------------------------
void TestDataFailures(
  vtkHyperTreeGrid* htg, std::ostringstream& logStream, std::vector<std::string>& retLog)
{
  vtkNew<vtkHyperTreeGrid> other1, other2;

  other1->DeepCopy(htg);
  other2->DeepCopy(htg);
  TestFieldDataFailures(other1.GetPointer(), other2.GetPointer(), other1->GetCellData(),
    other2->GetCellData(), htg->GetNumberOfCells(), logStream, retLog);

  other1->DeepCopy(htg);
  other2->DeepCopy(htg);
  TestFieldDataFailures(other1.GetPointer(), other2.GetPointer(), other1->GetFieldData(),
    other2->GetFieldData(), 10, logStream, retLog);
}

//------------------------------------------------------------------------------
template <class DataSetT>
void TestDataFailuresForDuplicatePointInput(
  DataSetT* ds, std::ostringstream& logStream, std::vector<std::string>& retLog)
{
  auto other = vtkSmartPointer<DataSetT>::Take(ds->NewInstance());

  {
    other->ShallowCopy(ds);
    vtkIdType nPoints = ds->GetNumberOfPoints();

    GenerateArray(ds->GetPointData(), nPoints, 1.0);
    GenerateArray(other->GetPointData(), nPoints, 2.0);

    CheckErrorMessage<DataSetT>(vtkTestUtilities::CompareDataObjects(ds, other), logStream,
      "Found point candidates when watching point position, but their tuples don't match.", retLog,
      "Unmatching Point Data");

    CheckErrorMessage<DataSetT>(vtkTestUtilities::ComparePoints(ds, other), logStream,
      "Found point candidates when watching point position, but their tuples don't match.", retLog,
      "Unmatching Point Data");
  }

  {
    other->ShallowCopy(ds);
    vtkIdType nCells = ds->GetNumberOfCells();

    GenerateArray(ds->GetCellData(), nCells, 1.0);
    GenerateArray(other->GetCellData(), nCells, 2.0);

    CheckErrorMessage<DataSetT>(vtkTestUtilities::CompareDataObjects(ds, other), logStream,
      "Found point candidates when watching point position, but their tuples don't match.", retLog,
      "Unmatching Cell Data");

    CheckErrorMessage<DataSetT>(vtkTestUtilities::CompareCells(ds, other), logStream,
      "Found point candidates when watching point position, but their tuples don't match.", retLog,
      "Unmatching Cell Data");
  }

  // Unmangling input first point
  auto newDS = vtkSmartPointer<DataSetT>::Take(ds->NewInstance());
  newDS->DeepCopy(ds);
  vtkPoints* points = newDS->GetPoints();
  auto data = vtkArrayDownCast<vtkFloatArray>(points->GetData());
  float p[3];
  data->GetTypedTuple(0, p);
  --p[0];
  --p[1];
  --p[2];
  data->SetTypedTuple(0, p);

  TestDataFailures(newDS.GetPointer(), logStream, retLog);
}

//------------------------------------------------------------------------------
template <class PointSetT>
void TestPoints(PointSetT* ps, std::ostringstream& logStream, std::vector<std::string>& retLog)
{
  auto other = vtkSmartPointer<PointSetT>::Take(ps->NewInstance());
  other->DeepCopy(ps);
  vtkPoints* points = other->GetPoints();
  auto data = vtkArrayDownCast<vtkFloatArray>(points->GetData());
  float p[3];
  data->GetTypedTuple(0, p);
  --p[0];
  --p[1];
  --p[2];
  data->SetTypedTuple(0, p);

  CheckErrorMessage<PointSetT>(vtkTestUtilities::CompareDataObjects(ps, other), logStream,
    "Point positions don't match between the 2 input " + vtk::TypeName<PointSetT>(), retLog,
    "Point");

  CheckErrorMessage<PointSetT>(vtkTestUtilities::ComparePoints(ps, other), logStream,
    "Point positions don't match between the 2 input " + vtk::TypeName<PointSetT>(), retLog,
    "Point");
}

//------------------------------------------------------------------------------
std::vector<std::string> TestDataSetFailures(vtkImageData* im, std::ostringstream& logStream)
{
  std::vector<std::string> retLog;
  vtkNew<vtkImageData> other;

  // Testing Origin
  other->ShallowCopy(im);
  double origin[3];
  other->GetOrigin(origin);
  ++origin[0];
  ++origin[1];
  ++origin[2];
  other->SetOrigin(origin);

  CheckErrorMessage<vtkImageData>(vtkTestUtilities::CompareDataObjects(im, other), logStream,
    "Structure doesn't match between the 2 input vtkImageData", retLog, "Origin");

  CheckErrorMessage<vtkImageData>(vtkTestUtilities::ComparePoints(im, other), logStream,
    "Structure doesn't match between the 2 input vtkImageData", retLog, "Origin");

  // Testing spacing
  other->ShallowCopy(im);
  double spacing[3];
  other->GetSpacing(spacing);
  ++spacing[0];
  ++spacing[1];
  ++spacing[2];
  other->SetSpacing(spacing);

  CheckErrorMessage<vtkImageData>(vtkTestUtilities::CompareDataObjects(im, other), logStream,
    "Structure doesn't match between the 2 input vtkImageData", retLog, "Spacing");

  CheckErrorMessage<vtkImageData>(vtkTestUtilities::ComparePoints(im, other), logStream,
    "Structure doesn't match between the 2 input vtkImageData", retLog, "Spacing");

  // Testing orientation
  other->ShallowCopy(im);
  vtkNew<vtkMatrix3x3> rot;
  double* rotData = rot->GetData();
  // Rotation on 3 axis by pi / 4
  rotData[0] = 0.5;
  rotData[1] = -0.5;
  rotData[2] = std::sqrt(2) * 0.5;
  rotData[3] = -0.5;
  rotData[4] = 0.5;
  rotData[5] = 0;
  rotData[6] = -0.5;
  rotData[7] = -0.5;
  rotData[8] = 0;
  other->SetDirectionMatrix(rot);

  CheckErrorMessage<vtkImageData>(vtkTestUtilities::CompareDataObjects(im, other), logStream,
    "Structure doesn't match between the 2 input vtkImageData", retLog, "Direction Matrix");

  CheckErrorMessage<vtkImageData>(vtkTestUtilities::ComparePoints(im, other), logStream,
    "Structure doesn't match between the 2 input vtkImageData", retLog, "Direction Matrix");

  TestExtent(im, logStream, retLog);
  TestDataFailures(im, logStream, retLog);

  return retLog;
}

//------------------------------------------------------------------------------
std::vector<std::string> TestDataSetFailures(vtkRectilinearGrid* rg, std::ostringstream& logStream)
{
  std::vector<std::string> retLog;

  TestCoords(rg, logStream, retLog);
  TestExtent(rg, logStream, retLog);
  TestDataFailures(rg, logStream, retLog);

  return retLog;
}

//------------------------------------------------------------------------------
std::vector<std::string> TestDataSetFailures(vtkStructuredGrid* sg, std::ostringstream& logStream)
{
  std::vector<std::string> retLog;

  TestPoints(sg, logStream, retLog);
  TestExtent(sg, logStream, retLog);
  TestDataFailures(sg, logStream, retLog);

  return retLog;
}

//------------------------------------------------------------------------------
std::vector<std::string> TestDataSetFailures(vtkUnstructuredGrid* ug, std::ostringstream& logStream)
{
  std::vector<std::string> retLog;

  TestPoints(ug, logStream, retLog);

  // We can't rely on TestDataFailures because we have duplicate points everywhere, and
  // in this case the discrepancy between point data / cell data is caught whil mapping points.
  // We need to use another procedure here.
  TestDataFailuresForDuplicatePointInput(ug, logStream, retLog);

  vtkNew<vtkUnstructuredGrid> other;
  other->DeepCopy(ug);
  other->GetCellTypesArray()->SetValue(0, VTK_HEXAHEDRON);

  CheckErrorMessage<vtkUnstructuredGrid>(vtkTestUtilities::CompareDataObjects(ug, other), logStream,
    "Cells of input of type vtkUnstructuredGrid do not match.", retLog, "Cell Types");

  CheckErrorMessage<vtkUnstructuredGrid>(vtkTestUtilities::CompareDataObjects(ug, other), logStream,
    "Cell connectivity is wrong between the 2 datasets.", retLog, "Connectivity");

  CheckErrorMessage<vtkUnstructuredGrid>(vtkTestUtilities::CompareCells(ug, other), logStream,
    "Cells of input of type vtkUnstructuredGrid do not match.", retLog, "Cell Types");

  CheckErrorMessage<vtkUnstructuredGrid>(vtkTestUtilities::CompareCells(ug, other), logStream,
    "Cell connectivity is wrong between the 2 datasets.", retLog, "Connectivity");

  return retLog;
}

//------------------------------------------------------------------------------
std::vector<std::string> TestDataSetFailures(vtkPolyData* pd, std::ostringstream& logStream)
{
  std::vector<std::string> retLog;

  TestPoints(pd, logStream, retLog);

  // We can't rely on TestDataFailures because we have duplicate points everywhere, and
  // in this case the discrepancy between point data / cell data is caught whil mapping points.
  // We need to use another procedure here.
  TestDataFailuresForDuplicatePointInput(pd, logStream, retLog);

  vtkNew<vtkTriangleFilter> triangulator;
  triangulator->SetInputData(pd);
  triangulator->Update();
  auto other = vtkPolyData::SafeDownCast(triangulator->GetOutputDataObject(0));

  CheckErrorMessage<vtkPolyData>(vtkTestUtilities::CompareDataObjects(pd, other), logStream,
    "Cells of input of type vtkPolyData do not match.", retLog, "Cell Types");

  CheckErrorMessage<vtkPolyData>(vtkTestUtilities::CompareDataObjects(pd, other), logStream,
    "Cell connectivity is wrong between the 2 datasets.", retLog, "Connectivity");

  CheckErrorMessage<vtkPolyData>(vtkTestUtilities::CompareCells(pd, other), logStream,
    "Cells of input of type vtkPolyData do not match.", retLog, "Cell Types");

  CheckErrorMessage<vtkPolyData>(vtkTestUtilities::CompareCells(pd, other), logStream,
    "Cell connectivity is wrong between the 2 datasets.", retLog, "Connectivity");

  return retLog;
}

//------------------------------------------------------------------------------
std::vector<std::string> TestDataSetFailures(vtkHyperTreeGrid* htg, std::ostringstream& logStream)
{
  std::vector<std::string> retLog;

  TestExtent(htg, logStream, retLog);
  TestDataFailures(htg, logStream, retLog);

  vtkNew<vtkHyperTreeGridDepthLimiter> limiter;
  limiter->SetInputData(htg);
  limiter->SetDepth(3);
  limiter->Update();

  auto other = vtkHyperTreeGrid::SafeDownCast(limiter->GetOutputDataObject(0));

  CheckErrorMessage<vtkHyperTreeGrid>(vtkTestUtilities::CompareDataObjects(htg, other), logStream,
    "Depth Limiter value doesn't match", retLog, "Topology");

  return retLog;
}

//------------------------------------------------------------------------------
std::vector<std::string> TestDataSetFailures(
  vtkPartitionedDataSetCollection* pdc, std::ostringstream& logStream)
{
  std::vector<std::string> retLog;

  if (!pdc)
  {
    return retLog;
  }

  vtkNew<vtkPartitionedDataSetCollection> other;
  other->DeepCopy(pdc);
  other->SetDataAssembly(nullptr);

  CheckErrorMessage<vtkPartitionedDataSetCollection>(
    vtkTestUtilities::CompareDataObjects(pdc, other), logStream,
    "vtkPartitionedDataSetCollection Assembly tree structures do not match", retLog, "Assembly");

  for (unsigned int index = 0; index < pdc->GetNumberOfPartitionedDataSets(); ++index)
  {
    vtkPartitionedDataSet* pd = pdc->GetPartitionedDataSet(index);

    if (!pd)
    {
      continue;
    }

    for (unsigned int partition = 0; partition < pd->GetNumberOfPartitions(); ++partition)
    {
      auto block = pd->GetPartition(partition);
      if (!block)
      {
        continue;
      }

      vtkLog(ERROR, << "datatype: " << block->GetClassName());

      std::vector<std::string> childLog;
      if (auto image = vtkImageData::SafeDownCast(block))
      {
        childLog = TestDataSetFailures(image, logStream);
      }
      else if (auto sg = vtkStructuredGrid::SafeDownCast(block))
      {
        childLog = TestDataSetFailures(sg, logStream);
      }
      else if (auto rg = vtkRectilinearGrid::SafeDownCast(block))
      {
        childLog = TestDataSetFailures(rg, logStream);
      }
      else if (auto ug = vtkUnstructuredGrid::SafeDownCast(block))
      {
        childLog = TestDataSetFailures(ug, logStream);
      }
      else if (auto htg = vtkHyperTreeGrid::SafeDownCast(block))
      {
        childLog = TestDataSetFailures(htg, logStream);
      }
      else if (auto polydata = vtkPolyData::SafeDownCast(block))
      {
        childLog = TestDataSetFailures(polydata, logStream);
      }

      retLog.insert(retLog.end(), childLog.begin(), childLog.end());
    }
  }

  return retLog;
}

//------------------------------------------------------------------------------
std::vector<std::string> TestDataSetFailures(
  vtkMultiBlockDataSet* mb, std::ostringstream& logStream)
{
  std::vector<std::string> retLog;

  if (!mb)
  {
    return retLog;
  }

  for (unsigned int index = 0; index < mb->GetNumberOfBlocks(); ++index)
  {
    auto block = mb->GetBlock(index);

    if (!block)
    {
      continue;
    }

    vtkLog(ERROR, << "datatype: " << block->GetClassName());

    std::vector<std::string> childLog;

    if (auto image = vtkImageData::SafeDownCast(block))
    {
      childLog = TestDataSetFailures(image, logStream);
    }
    else if (auto sg = vtkStructuredGrid::SafeDownCast(block))
    {
      childLog = TestDataSetFailures(sg, logStream);
    }
    else if (auto rg = vtkRectilinearGrid::SafeDownCast(block))
    {
      childLog = TestDataSetFailures(rg, logStream);
    }
    else if (auto ug = vtkUnstructuredGrid::SafeDownCast(block))
    {
      childLog = TestDataSetFailures(ug, logStream);
    }
    else if (auto htg = vtkHyperTreeGrid::SafeDownCast(block))
    {
      childLog = TestDataSetFailures(htg, logStream);
    }
    else if (auto polydata = vtkPolyData::SafeDownCast(block))
    {
      childLog = TestDataSetFailures(polydata, logStream);
    }

    retLog.insert(retLog.end(), childLog.begin(), childLog.end());
  }

  return retLog;
}

//------------------------------------------------------------------------------
vtkNew<vtkTable> MakeTable()
{
  vtkIdType N = 5;
  vtkNew<vtkTable> table;

  vtkNew<vtkStringArray> stringArray;
  stringArray->SetName("String");
  stringArray->SetNumberOfTuples(N);

  vtkNew<vtkBitArray> bitArray;
  bitArray->SetName("Bit");
  bitArray->SetNumberOfComponents(8); // Testing multi-dimensional tuples
  bitArray->SetNumberOfTuples(N);

  vtkNew<vtkVariantArray> variantArray;
  variantArray->SetName("Variant");
  variantArray->SetNumberOfTuples(N);

  vtkNew<vtkDoubleArray> doubleArray;
  doubleArray->SetName("Double");
  doubleArray->SetNumberOfComponents(8); // Testing run-time vector manipulation
  doubleArray->SetNumberOfTuples(N);

  vtkNew<vtkUnsignedCharArray> ghosts;
  ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
  ghosts->SetNumberOfTuples(N);

  for (vtkIdType i = 0; i < N; ++i)
  {
    stringArray->SetValue(i, std::to_string(i));
    for (int j = 0; j < 8; ++j)
    {
      bitArray->SetValue(8 * i + j, j % 2);
      doubleArray->SetValue(8 * i + j, 8 * i + j);
    }
    variantArray->SetValue(i, i);
    ghosts->SetValue(i, i % 2 ? 0 : 1);
  }

  vtkNew<vtkDataSetAttributes> dsa;
  dsa->AddArray(stringArray);
  dsa->AddArray(bitArray);
  dsa->AddArray(variantArray);
  dsa->AddArray(doubleArray);
  dsa->AddArray(ghosts);
  dsa->SetGhostsToSkip(1);

  table->GetFieldData()->AddArray(doubleArray);
  table->SetRowData(dsa);

  return table;
}

//------------------------------------------------------------------------------
std::vector<std::string> TestTableFailures(vtkTable* table, std::ostringstream& logStream)
{
  std::vector<std::string> retLog;

  vtkNew<vtkTable> other;

  {
    other->DeepCopy(table);
    vtkFieldData* fd = other->GetRowData();
    auto array = vtkArrayDownCast<vtkStringArray>(fd->GetAbstractArray("String"));
    array->SetValue(1, "1000");
    CheckErrorMessage<vtkTable>(vtkTestUtilities::CompareDataObjects(table, other), logStream,
      "Failed to match the 2 input data objects of type vtkTable", retLog, "String Array");
  }

  {
    other->DeepCopy(table);
    vtkFieldData* fd = other->GetRowData();
    auto array = vtkArrayDownCast<vtkBitArray>(fd->GetAbstractArray("Bit"));
    array->SetValue(10, !array->GetValue(8));
    CheckErrorMessage<vtkTable>(vtkTestUtilities::CompareDataObjects(table, other), logStream,
      "Failed to match the 2 input data objects of type vtkTable", retLog, "Bit Array");
  }

  {
    other->DeepCopy(table);
    vtkFieldData* fd = other->GetRowData();
    auto array = vtkArrayDownCast<vtkVariantArray>(fd->GetAbstractArray("Variant"));
    array->SetValue(1, 1000);
    CheckErrorMessage<vtkTable>(vtkTestUtilities::CompareDataObjects(table, other), logStream,
      "Failed to match the 2 input data objects of type vtkTable", retLog, "Variant Array");
  }

  {
    other->DeepCopy(table);
    vtkFieldData* fd = other->GetRowData();
    auto array = vtkArrayDownCast<vtkDoubleArray>(fd->GetAbstractArray("Double"));
    array->SetValue(10, 500);
    CheckErrorMessage<vtkTable>(vtkTestUtilities::CompareDataObjects(table, other), logStream,
      "Failed to match the 2 input data objects of type vtkTable", retLog, "Double Array");
  }

  {
    other->DeepCopy(table);
    vtkFieldData* fd = other->GetFieldData();
    auto array = vtkArrayDownCast<vtkDoubleArray>(fd->GetAbstractArray("Double"));
    array->SetValue(10, 500);
    CheckErrorMessage<vtkTable>(vtkTestUtilities::CompareDataObjects(table, other), logStream,
      "Failed to match the 2 input data objects of type vtkTable", retLog, "Double Array");
  }

  return retLog;
}

//------------------------------------------------------------------------------
template <class DataSetT, class ReaderT>
bool TestDataSet(const std::string& root, std::string&& name)
{
  vtkLog(INFO, "### Testing " << vtk::TypeName<DataSetT>());

  vtkNew<ReaderT> reader;
  reader->SetFileName((root + name).c_str());
  reader->Update();
  auto ds = DataSetT::SafeDownCast(reader->GetOutputDataObject(0));

  if (!vtkTestUtilities::CompareDataObjects(ds, ds))
  {
    vtkLog(ERROR, "Datasets should be similar, but they are not.");
    return false;
  }

  if (!ComparePoints(ds, ds))
  {
    return false;
  }

  if (!vtkTestUtilities::CompareCells(ds, ds))
  {
    vtkLog(ERROR, "Cells should be similar, but they are not.");
    return false;
  }

  std::ostringstream logStream;

  // Turning off ERROR logging so we can test that the utility correctly catches failures
  TurnOffLogging(logStream);

  auto retLog = TestDataSetFailures(ds, logStream);

  TurnOnLogging();

  for (const std::string& log : retLog)
  {
    vtkLog(ERROR, << log);
  }

  return retLog.empty();
}

//------------------------------------------------------------------------------
template <>
bool TestDataSet<vtkMultiBlockDataSet, vtkXMLMultiBlockDataReader>(
  const std::string& root, std::string&& name)
{
  vtkLog(INFO, "### Testing " << vtk::TypeName<vtkMultiBlockDataSet>());

  vtkNew<vtkXMLMultiBlockDataReader> reader;
  reader->SetFileName((root + name).c_str());
  reader->Update();
  auto ds = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));

  if (!vtkTestUtilities::CompareDataObjects(ds, ds))
  {
    vtkLog(ERROR, "Datasets should be similar, but they are not.");
    return false;
  }

  std::ostringstream logStream;

  // Turning off ERROR logging so we can test that the utility correctly catches failures
  TurnOffLogging(logStream);

  auto retLog = TestDataSetFailures(ds, logStream);

  TurnOnLogging();

  for (const std::string& log : retLog)
  {
    vtkLog(ERROR, << log);
  }

  return retLog.empty();
}

//------------------------------------------------------------------------------
template <>
bool TestDataSet<vtkPartitionedDataSetCollection, vtkXMLPartitionedDataSetCollectionReader>(
  const std::string& root, std::string&& name)
{
  vtkLog(INFO, "### Testing " << vtk::TypeName<vtkPartitionedDataSetCollection>());

  vtkNew<vtkXMLPartitionedDataSetCollectionReader> reader;
  reader->SetFileName((root + name).c_str());
  reader->Update();
  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));

  if (!vtkTestUtilities::CompareDataObjects(pdc, pdc))
  {
    vtkLog(ERROR, "PartitionedDataSetCollections should be similar, but they are not.");
    return false;
  }

  vtkNew<vtkPartitionedDataSetCollection> other;
  other->DeepCopy(pdc);
  other->SetDataAssembly(nullptr);
  if (!vtkTestUtilities::CompareDataObjects(other, other))
  {
    vtkLog(ERROR, "PartitionedDataSetCollections should be similar, but they are not.");
    return false;
  }

  std::ostringstream logStream;

  // Turning off ERROR logging so we can test that the utility correctly catches failures
  TurnOffLogging(logStream);

  auto retLog = TestDataSetFailures(pdc, logStream);

  TurnOnLogging();

  for (const std::string& log : retLog)
  {
    vtkLog(ERROR, << log);
  }

  return retLog.empty();
}

//------------------------------------------------------------------------------
bool TestTableAndArrays()
{
  vtkLog(INFO, "### Testing vtkTable");

  std::ostringstream logStream;

  auto table = MakeTable();

  if (!vtkTestUtilities::CompareDataObjects(table, table))
  {
    vtkLog(ERROR, "Tables should be similar, but they are not.");
    return false;
  }

  // Turning off ERROR logging so we can test that the utility correctly catches failures
  TurnOffLogging(logStream);

  auto retLog = TestTableFailures(table, logStream);

  TurnOnLogging();

  for (const std::string& log : retLog)
  {
    vtkLog(ERROR, << log);
  }

  return retLog.empty();
}

/**
 * Recursively deep copy the input tree pointed by the cursor
 * to the output, ignoring masked branches. This will create a new HTG
 * with a totally different internal structure,
 * that should still be identical to the original one.
 */
void CopyInputTreeToOutput(vtkHyperTreeGridNonOrientedCursor* inCursor,
  vtkHyperTreeGridNonOrientedCursor* outCursor, vtkCellData* inCellData, vtkCellData* outCellData,
  vtkBitArray* inMask, vtkBitArray* outMask)
{
  vtkIdType outIdx = outCursor->GetGlobalNodeIndex();
  vtkIdType inIdx = inCursor->GetGlobalNodeIndex();
  if (inMask)
  {
    outMask->InsertTuple1(outIdx, inMask->GetValue(inIdx));
  }
  outCellData->InsertTuple(outIdx, inIdx, inCellData);
  if (!inCursor->IsMasked())
  {
    if (!inCursor->IsLeaf())
    {
      outCursor->SubdivideLeaf();
      for (int ichild = 0; ichild < inCursor->GetNumberOfChildren(); ++ichild)
      {
        outCursor->ToChild(ichild);
        inCursor->ToChild(ichild);
        ::CopyInputTreeToOutput(inCursor, outCursor, inCellData, outCellData, inMask, outMask);
        outCursor->ToParent();
        inCursor->ToParent();
      }
    }
  }
}

/**
 * Compare HyperTreeGrid with a different memory layout.
 */
bool TestRandomHyperTreeGridCompare()
{
  // Create a random HTG source using masking
  vtkNew<vtkRandomHyperTreeGridSource> randomSource;
  randomSource->SetDimensions(3, 3, 3);
  randomSource->SetMaxDepth(5);
  randomSource->SetMaskedFraction(0.3);
  randomSource->SetSplitFraction(0.8);

  // Generate global ids field
  vtkNew<vtkGenerateGlobalIds> globalIds;
  globalIds->SetInputConnection(randomSource->GetOutputPort());

  // Limit HTG depth
  vtkNew<vtkHyperTreeGridDepthLimiter> limiter;
  limiter->SetInputConnection(globalIds->GetOutputPort());
  limiter->SetDepth(4);
  limiter->Update();
  vtkHyperTreeGrid* sourceHTG = limiter->GetHyperTreeGridOutput();

  // Create copy its structure, not its content
  vtkNew<vtkHyperTreeGrid> copyHTG;
  copyHTG->CopyEmptyStructure(sourceHTG);
  copyHTG->GetCellData()->CopyStructure(sourceHTG->GetCellData());

  vtkBitArray* inputMask = sourceHTG->GetMask();
  vtkNew<vtkBitArray> outputMask;

  // Copy recursively each tree
  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor, inCursor;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator inputIterator;
  vtkIdType inTreeIndex = 0, totalVertices = 0;
  sourceHTG->InitializeTreeIterator(inputIterator);
  while (inputIterator.GetNextTree(inTreeIndex))
  {
    sourceHTG->InitializeNonOrientedCursor(inCursor, inTreeIndex);
    copyHTG->InitializeNonOrientedCursor(outCursor, inTreeIndex, true);
    outCursor->SetGlobalIndexStart(totalVertices);
    ::CopyInputTreeToOutput(
      inCursor, outCursor, sourceHTG->GetCellData(), copyHTG->GetCellData(), inputMask, outputMask);
    totalVertices += outCursor->GetTree()->GetNumberOfVertices();
  }
  copyHTG->SetMask(outputMask);

  if (!vtkTestUtilities::CompareDataObjects(sourceHTG, copyHTG) ||
    !vtkTestUtilities::CompareDataObjects(copyHTG, sourceHTG))
  {
    vtkLog(ERROR, "HyperTreeGrids should be similar, but they are not.");
    return false;
  }

  return true;
}
} // anonymous namespace

//------------------------------------------------------------------------------
int TestDataObjectCompare(int argc, char* argv[])
{
  bool retVal = true;

  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  std::string root = testing->GetDataRoot() + std::string("/Data/DataObjects/");

  retVal &= ::TestDataSet<vtkImageData, vtkXMLImageDataReader>(root, "image_data_template.vti");
  retVal &= ::TestDataSet<vtkRectilinearGrid, vtkXMLRectilinearGridReader>(
    root, "rectilinear_grid_template.vtr");
  retVal &= ::TestDataSet<vtkStructuredGrid, vtkXMLStructuredGridReader>(
    root, "structured_grid_template.vts");
  retVal &= ::TestDataSet<vtkUnstructuredGrid, vtkXMLUnstructuredGridReader>(
    root, "unstructured_grid_wavelet_duplicate.vtu");
  retVal &= ::TestDataSet<vtkPolyData, vtkXMLPolyDataReader>(root, "poly_data_template.vtp");
  retVal &= ::TestDataSet<vtkHyperTreeGrid, vtkXMLHyperTreeGridReader>(
    root, "hyper_tree_grid_template.htg");
  retVal &= ::TestRandomHyperTreeGridCompare();
  retVal &=
    ::TestDataSet<vtkPartitionedDataSetCollection, vtkXMLPartitionedDataSetCollectionReader>(
      root, "partitioned_dataset_collection_template.vtpc");
  retVal &= ::TestDataSet<vtkMultiBlockDataSet, vtkXMLMultiBlockDataReader>(
    root, "multiblock_dataset_template.vtm");

  retVal &= ::TestTableAndArrays();

  return retVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
