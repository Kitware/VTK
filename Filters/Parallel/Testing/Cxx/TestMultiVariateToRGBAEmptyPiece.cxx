// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that vtkMultiVariateToRGBA does not hang when only some processes
 * have data, as with a serial reader: the other processes get an empty
 * table or dataset without arrays, or with unrelated arrays. Also check that
 * the empty pieces use the association of the others, that all pieces can
 * be empty, and that a process missing the array, while not empty, makes all
 * processes fail instead of hanging.
 */

#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDataArrayRange.h>
#include <vtkDoubleArray.h>
#include <vtkExecutive.h>
#include <vtkImageData.h>
#include <vtkMPIController.h>
#include <vtkMultiVariateToRGBA.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkTable.h>
#include <vtkTestErrorObserver.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace
{
#define CHECK(condition)                                                                           \
  if (!(condition))                                                                                \
  {                                                                                                \
    std::cerr << "Rank " << rank << ", line " << __LINE__ << ": check failed: " #condition         \
              << std::endl;                                                                        \
    return false;                                                                                  \
  }

constexpr int NumberOfComponents = 4;
constexpr vtkIdType NumberOfTuples = 50;

/**
 * Returns a 4 component array with values in [0, 1].
 */
vtkSmartPointer<vtkDoubleArray> MakeValues()
{
  auto values = vtkSmartPointer<vtkDoubleArray>::New();
  values->SetName("values");
  values->SetNumberOfComponents(NumberOfComponents);
  values->SetNumberOfTuples(NumberOfTuples);
  for (vtkIdType i = 0; i < values->GetNumberOfValues(); ++i)
  {
    values->SetValue(i, std::fabs(std::sin(0.37 * i)));
  }
  return values;
}

/**
 * Returns a table with one column per component of MakeValues(), or a
 * table without columns if 'empty' is true.
 */
vtkSmartPointer<vtkTable> MakeTable(bool empty)
{
  auto table = vtkSmartPointer<vtkTable>::New();
  if (!empty)
  {
    vtkSmartPointer<vtkDoubleArray> values = MakeValues();
    for (int c = 0; c < NumberOfComponents; ++c)
    {
      vtkNew<vtkDoubleArray> column;
      column->SetName(std::string(1, static_cast<char>('a' + c)).c_str());
      column->SetNumberOfTuples(NumberOfTuples);
      for (vtkIdType i = 0; i < NumberOfTuples; ++i)
      {
        column->SetValue(i, values->GetComponent(i, c));
      }
      table->AddColumn(column);
    }
  }
  return table;
}

/**
 * Returns an image with MakeValues() as point scalars (no arrays if
 * 'withScalars' is false), or an image without points if 'empty' is true.
 */
vtkSmartPointer<vtkImageData> MakeImage(bool empty, bool withScalars = true)
{
  auto image = vtkSmartPointer<vtkImageData>::New();
  if (!empty)
  {
    image->SetDimensions(5, 5, 2);
    if (withScalars)
    {
      image->GetPointData()->SetScalars(MakeValues());
    }
  }
  return image;
}

/**
 * Returns the 'rgba' array of the output of 'filter', a table or an image.
 */
vtkDataArray* GetRGBA(vtkMultiVariateToRGBA* filter)
{
  vtkDataObject* output = filter->GetOutputDataObject(0);
  if (vtkTable* table = vtkTable::SafeDownCast(output))
  {
    return vtkDataArray::SafeDownCast(table->GetColumnByName("rgba"));
  }
  return vtkImageData::SafeDownCast(output)->GetPointData()->GetArray("rgba");
}

/**
 * Runs the filter on 'data' on all processes and checks that the output on
 * rank 0 matches a serial run on the same data.
 */
bool TestEmptyPieces(vtkMultiProcessController* controller, vtkDataObject* data)
{
  int rank = controller->GetLocalProcessId();
  vtkNew<vtkMultiVariateToRGBA> filter;
  filter->SetController(controller);
  filter->SetInputData(data);
  filter->Update();
  vtkDataArray* rgba = GetRGBA(filter);
  CHECK(rgba);
  if (rank != 0)
  {
    CHECK(rgba->GetNumberOfTuples() == 0);
    return true;
  }
  vtkNew<vtkMultiVariateToRGBA> serial;
  serial->SetController(nullptr);
  serial->SetInputData(data);
  serial->Update();
  vtkDataArray* expected = GetRGBA(serial);
  auto values = vtk::DataArrayValueRange(rgba);
  auto expectedValues = vtk::DataArrayValueRange(expected);
  CHECK(values.size() == expectedValues.size());
  CHECK(std::equal(values.begin(), values.end(), expectedValues.begin()));
  return true;
}

/**
 * All pieces are empty but have the array, as after a threshold that removes
 * everything: every process gets an empty output, as in a serial run.
 */
bool TestAllEmpty(vtkMultiProcessController* controller)
{
  int rank = controller->GetLocalProcessId();
  vtkNew<vtkDoubleArray> values;
  values->SetName("values");
  values->SetNumberOfComponents(NumberOfComponents);
  vtkNew<vtkImageData> image;
  image->GetPointData()->SetScalars(values);
  vtkNew<vtkMultiVariateToRGBA> filter;
  filter->SetController(controller);
  filter->SetInputData(image);
  filter->Update();
  vtkDataArray* rgba = GetRGBA(filter);
  CHECK(rgba && rgba->GetNumberOfTuples() == 0);
  return true;
}

/**
 * Empty pieces have an unrelated array but not the scalars: their scalars
 * come from the others, as when they have no arrays.
 */
bool TestEmptyPiecesOtherArray(vtkMultiProcessController* controller)
{
  int rank = controller->GetLocalProcessId();
  vtkSmartPointer<vtkImageData> image = MakeImage(rank != 0);
  if (rank != 0)
  {
    vtkNew<vtkDoubleArray> other;
    other->SetName("other");
    image->GetPointData()->AddArray(other);
  }
  return TestEmptyPieces(controller, image);
}

/**
 * Only rank 0 has data, with cell scalars: the empty pieces put their output
 * in cell data too, although the default association is POINTS_THEN_CELLS.
 */
bool TestEmptyPiecesCellScalars(vtkMultiProcessController* controller)
{
  int rank = controller->GetLocalProcessId();
  vtkNew<vtkImageData> image;
  if (rank == 0)
  {
    image->SetDimensions(5, 5, 2);
    vtkSmartPointer<vtkDoubleArray> values = MakeValues();
    values->SetNumberOfTuples(image->GetNumberOfCells());
    image->GetCellData()->SetScalars(values);
  }
  vtkNew<vtkMultiVariateToRGBA> filter;
  filter->SetController(controller);
  filter->SetInputData(image);
  filter->Update();
  vtkImageData* output = vtkImageData::SafeDownCast(filter->GetOutputDataObject(0));
  CHECK(output->GetCellData()->GetArray("rgba"));
  CHECK(!output->GetPointData()->GetArray("rgba"));
  return true;
}

/**
 * A process that has data but not the array fails, so all processes fail.
 */
bool TestMissingArray(vtkMultiProcessController* controller)
{
  int rank = controller->GetLocalProcessId();
  vtkSmartPointer<vtkImageData> image = MakeImage(false, rank == 0);
  vtkNew<vtkMultiVariateToRGBA> filter;
  vtkNew<vtkTest::ErrorObserver> errorObserver;
  filter->SetController(controller);
  filter->SetInputData(image);
  // The errors are expected: the filter reports the missing array and the
  // executive the failed request.
  filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  filter->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  filter->Update();
  CHECK(!GetRGBA(filter));
  return true;
}
}

int TestMultiVariateToRGBAEmptyPiece(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  bool empty = controller->GetLocalProcessId() != 0;
  // every test runs on all processes, so a failure does not leave the others waiting
  bool success = TestEmptyPieces(controller, MakeTable(empty));
  success = TestEmptyPieces(controller, MakeImage(empty)) && success;
  success = TestAllEmpty(controller) && success;
  success = TestEmptyPiecesOtherArray(controller) && success;
  success = TestEmptyPiecesCellScalars(controller) && success;
  success = TestMissingArray(controller) && success;
  int localSuccess = success;
  int globalSuccess = 0;
  controller->AllReduce(&localSuccess, &globalSuccess, 1, vtkCommunicator::MIN_OP);
  controller->Finalize();
  return globalSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}
