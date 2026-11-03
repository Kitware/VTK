// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that vtkMultiVariateToRGBA on distributed data gives the same 'rgba'
 * and 'gbc' as on all the data in one process, as the ranges used to
 * normalize the components are reduced across the processes. Each process has
 * a slab of an image, with component ranges that differ between the slabs.
 * The same holds for table pieces, with a CustomRange, and after a lens-only
 * change that follows a change of the values on one process only. Processes
 * with different numbers of components all fail, without waiting on each
 * other.
 */

#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkExecutive.h>
#include <vtkImageData.h>
#include <vtkMPIController.h>
#include <vtkMultiVariateToRGBA.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkStringFormatter.h>
#include <vtkTable.h>
#include <vtkTestErrorObserver.h>

#include <cmath>
#include <functional>
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

constexpr int Dimensions[3] = { 21, 5, 3 };
constexpr double Tolerance = 1e-6;

// Returns the image with the x extent [x0, x1] and a 4-component 'mv' array
// that depends on x, so each slab has different component ranges. With 5
// components, the last one is y.
vtkSmartPointer<vtkImageData> MakeImage(int x0, int x1, int numComponents = 4)
{
  auto image = vtkSmartPointer<vtkImageData>::New();
  image->SetExtent(x0, x1, 0, Dimensions[1] - 1, 0, Dimensions[2] - 1);
  vtkNew<vtkDoubleArray> values;
  values->SetName("mv");
  values->SetNumberOfComponents(numComponents);
  values->SetNumberOfTuples(image->GetNumberOfPoints());
  for (vtkIdType i = 0; i < image->GetNumberOfPoints(); ++i)
  {
    double p[3];
    image->GetPoint(i, p);
    const double tuple[] = { p[0] + 0.1 * p[1], 20 - p[0] + 2 * p[2],
      3 * std::sin(0.7 * p[0]) + p[1], 0.05 * p[0] * p[0] + p[2], p[1] };
    values->SetTuple(i, tuple);
  }
  image->GetPointData()->AddArray(values);
  return image;
}

// Triples component 0 of the points of 'image' with x < 'xLimit'.
void ScaleBelow(vtkImageData* image, double xLimit)
{
  vtkDataArray* values = image->GetPointData()->GetArray("mv");
  for (vtkIdType i = 0; i < image->GetNumberOfPoints(); ++i)
  {
    double p[3];
    image->GetPoint(i, p);
    if (p[0] < xLimit)
    {
      values->SetComponent(i, 0, 3 * values->GetComponent(i, 0));
    }
  }
  values->Modified();
}

// Returns a table with a column per component of 'mv' and a row per point of
// 'image', in the same order.
vtkSmartPointer<vtkTable> ToTable(vtkImageData* image)
{
  vtkDataArray* values = image->GetPointData()->GetArray("mv");
  auto table = vtkSmartPointer<vtkTable>::New();
  for (int c = 0; c < values->GetNumberOfComponents(); ++c)
  {
    vtkNew<vtkDoubleArray> column;
    column->SetName(("c" + vtk::to_string(c)).c_str());
    column->SetNumberOfTuples(values->GetNumberOfTuples());
    column->CopyComponent(0, values, c);
    table->AddColumn(column);
  }
  return table;
}

using Setup = std::function<void(vtkMultiVariateToRGBA*)>;

vtkSmartPointer<vtkMultiVariateToRGBA> Run(
  vtkDataObject* input, vtkMultiProcessController* controller, const Setup& setup)
{
  auto filter = vtkSmartPointer<vtkMultiVariateToRGBA>::New();
  filter->SetController(controller);
  filter->SetInputData(input);
  filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "mv");
  filter->SetStoreGBC(true);
  setup(filter);
  filter->Update();
  return filter;
}

// Returns the array 'name' of the rows or points of the output of 'filter'.
vtkDataArray* GetArray(vtkMultiVariateToRGBA* filter, const char* name)
{
  vtkDataObject* output = filter->GetOutputDataObject(0);
  vtkDataSetAttributes* attributes = output->GetAttributes(
    vtkTable::SafeDownCast(output) ? vtkDataObject::ROW : vtkDataObject::POINT);
  return attributes ? attributes->GetArray(name) : nullptr;
}

// Checks that 'rgba' and 'gbc' of 'distributed', for this process's slab
// 'piece', match the ones of 'serial', for all the data 'whole', and that
// both used the same ranges. The outputs are images or tables with a row per
// point of the images.
bool Matches(vtkMultiProcessController* controller, vtkMultiVariateToRGBA* distributed,
  vtkMultiVariateToRGBA* serial, vtkImageData* piece, vtkImageData* whole)
{
  int rank = controller->GetLocalProcessId();
  CHECK(distributed->GetRange() == serial->GetRange());
  for (const char* name : { "rgba", "gbc" })
  {
    vtkDataArray* actualArray = GetArray(distributed, name);
    vtkDataArray* expectedArray = GetArray(serial, name);
    CHECK(actualArray && expectedArray);
    CHECK(actualArray->GetNumberOfTuples() == piece->GetNumberOfPoints());
    for (vtkIdType i = 0; i < piece->GetNumberOfPoints(); ++i)
    {
      double p[3];
      piece->GetPoint(i, p);
      vtkIdType j = whole->FindPoint(p);
      CHECK(j >= 0);
      for (int c = 0; c < actualArray->GetNumberOfComponents(); ++c)
      {
        if (std::fabs(actualArray->GetComponent(i, c) - expectedArray->GetComponent(j, c)) >
          Tolerance)
        {
          std::cerr << "Rank " << rank << ": " << name << " at (" << p[0] << ", " << p[1] << ", "
                    << p[2] << "), component " << c << ": " << actualArray->GetComponent(i, c)
                    << ", expected " << expectedArray->GetComponent(j, c) << std::endl;
          return false;
        }
      }
    }
  }
  return true;
}

// Checks that the filter on this process's slab 'piece', or on its table
// with 'asTable', matches the filter on all the data 'whole', after 'setup'.
bool MatchesSerial(vtkMultiProcessController* controller, vtkImageData* piece, vtkImageData* whole,
  const Setup& setup, bool asTable = false)
{
  vtkSmartPointer<vtkDataObject> pieceInput = piece;
  vtkSmartPointer<vtkDataObject> wholeInput = whole;
  if (asTable)
  {
    pieceInput = ToTable(piece);
    wholeInput = ToTable(whole);
  }
  auto distributed = Run(pieceInput, controller, setup);
  auto serial = Run(wholeInput, nullptr, setup);
  return Matches(controller, distributed, serial, piece, whole);
}

void SetLens(vtkMultiVariateToRGBA* filter, double radius)
{
  filter->SetLensCenter(0.2, 0.1);
  filter->SetLensRadius(radius);
}

// Checks lens-only changes on distributed data: they match the serial
// results, also after the values changed on rank 0 only, which makes every
// process execute fully. Rank 0 changes the points with x < 'rank0X1', its
// slab without the boundary shared with rank 1.
bool TestLensCache(vtkMultiProcessController* controller, int x0, int x1, int rank0X1, int cells)
{
  int rank = controller->GetLocalProcessId();
  vtkSmartPointer<vtkImageData> piece = MakeImage(x0, x1);
  vtkSmartPointer<vtkImageData> whole = MakeImage(0, cells);
  auto distributed = Run(piece, controller, [](vtkMultiVariateToRGBA*) {});
  // no early return before an Update: the others would wait for this process
  SetLens(distributed, 0.5);
  distributed->Update();
  auto serial = Run(whole, nullptr, [](vtkMultiVariateToRGBA* f) { SetLens(f, 0.5); });
  bool success = Matches(controller, distributed, serial, piece, whole);

  if (rank == 0)
  {
    ScaleBelow(piece, rank0X1);
  }
  ScaleBelow(whole, rank0X1);
  SetLens(distributed, 0.4);
  distributed->Update();
  serial = Run(whole, nullptr, [](vtkMultiVariateToRGBA* f) { SetLens(f, 0.4); });
  success = Matches(controller, distributed, serial, piece, whole) && success;
  CHECK(success);
  return true;
}

// Checks that every process fails when the last one has 5 components and the
// others 4.
bool TestComponentMismatch(vtkMultiProcessController* controller, int x0, int x1)
{
  int rank = controller->GetLocalProcessId();
  int size = controller->GetNumberOfProcesses();
  vtkSmartPointer<vtkImageData> piece = MakeImage(x0, x1, rank == size - 1 ? 5 : 4);
  vtkNew<vtkTest::ErrorObserver> errorObserver;
  auto filter = Run(piece, controller,
    [&errorObserver](vtkMultiVariateToRGBA* f)
    {
      f->AddObserver(vtkCommand::ErrorEvent, errorObserver);
      f->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver);
    });
  CHECK(errorObserver->HasErrorMessage("different numbers of components"));
  CHECK(!GetArray(filter, "rgba") && !GetArray(filter, "gbc"));
  return true;
}
}

int TestMultiVariateToRGBAParallel(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  int rank = controller->GetLocalProcessId();
  int size = controller->GetNumberOfProcesses();
  // slabs along x, sharing their boundary points
  int cells = Dimensions[0] - 1;
  vtkSmartPointer<vtkImageData> piece = MakeImage(rank * cells / size, (rank + 1) * cells / size);
  vtkSmartPointer<vtkImageData> whole = MakeImage(0, cells);

  // every test runs on all processes, so a failure does not leave the others waiting
  // default settings: one group, every variable visible, ranges reduced from the data
  bool success = MatchesSerial(controller, piece, whole, [](vtkMultiVariateToRGBA*) {});
  // two groups, { 0, 1 } and { 2, 3 }, each with its own range reduced from the data
  success = MatchesSerial(controller, piece, whole,
              [](vtkMultiVariateToRGBA* filter)
              {
                for (int i : { 0, 1, 2, 3 })
                {
                  filter->AddPartitionIndex(i);
                }
                filter->AddPartitionOffset(2);
                filter->AddPartitionOffset(4);
              }) &&
    success;
  // variable 1 hidden: the range is reduced from the visible variables 0, 2 and 3 only
  success = MatchesSerial(controller, piece, whole,
              [](vtkMultiVariateToRGBA* filter)
              {
                for (int i : { 0, 2, 3 })
                {
                  filter->AddVisibleVariableIndex(i);
                }
              }) &&
    success;
  // a custom range narrower than the data, so values are clamped
  success = MatchesSerial(controller, piece, whole,
              [](vtkMultiVariateToRGBA* filter)
              {
                filter->SetPartitionIndices({ 0, 1, 2, 3 });
                filter->SetPartitionOffsets({ 2, 4 });
                filter->SetCustomRange({ 2, 15, -1, 10 });
              }) &&
    success;
  // table pieces: each slab as a table with a column per component, default settings
  success = MatchesSerial(controller, piece, whole, [](vtkMultiVariateToRGBA*) {}, true) && success;
  // table pieces with column 1 hidden: only columns 0, 2 and 3 are processed
  success = MatchesSerial(
              controller, piece, whole, [](vtkMultiVariateToRGBA* filter)
              { filter->SetVariableVisibility(1, false); }, true) &&
    success;
  // lens-only changes reuse the cache, unless the values changed on any process
  success = TestLensCache(
              controller, rank * cells / size, (rank + 1) * cells / size, cells / size, cells) &&
    success;
  // processes with different numbers of components all fail, without waiting on each other
  if (size > 1)
  {
    success =
      TestComponentMismatch(controller, rank * cells / size, (rank + 1) * cells / size) && success;
  }
  int localSuccess = success;
  int globalSuccess = 0;
  controller->AllReduce(&localSuccess, &globalSuccess, 1, vtkCommunicator::MIN_OP);
  controller->Finalize();
  return globalSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}
