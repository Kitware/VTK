// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that vtkMultiVariateToRGBA does not hang when only some processes
 * have data, as with a serial reader: the other processes get an empty
 * table or dataset without arrays. Also check that all pieces can be empty,
 * and that a process missing the array, while not empty, makes all processes
 * fail instead of hanging.
 */

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
  // The errors are expected. Each observer catches the errors reported by
  // its object: the output reports the missing array, the executive the
  // failed request, and the filter its own errors (none in this case, but
  // a change in RequestData should not fail the test on its output).
  filter->UpdateDataObject();
  filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  filter->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  filter->GetOutputDataObject(0)->AddObserver(vtkCommand::ErrorEvent, errorObserver);
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
  success = TestMissingArray(controller) && success;
  int localSuccess = success;
  int globalSuccess = 0;
  controller->AllReduce(&localSuccess, &globalSuccess, 1, vtkCommunicator::MIN_OP);
  controller->Finalize();
  return globalSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}
