// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkPResolveHangingNodes on a 2D vtkPolyData mesh of quads with 3 levels distributed over
// 4 processes, with 2 partitions each of a vtkPartitionedDataSetCollection: the object factory
// override, polygon and decomposition modes, points received from the other partitions of the same
// process and of other processes with their point data, the conformity of the gathered output, and
// a process without cells.

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "PResolveHangingNodesTestUtilities.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPResolveHangingNodes.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{
// The domain [0,8] x [0,4].
constexpr double DOMAIN_AREA = 32.0;
constexpr double DOMAIN_PERIMETER = 24.0;

// 3-level mesh over the domain [0,8] x [0,4], with a quad in each of the 8 partitions, the
// partitions 2 rank and 2 rank + 1 being on the process rank:
// - Partition 0: level 0 quad [0,4] x [0,4] (area 16). Its edge x = 4 has the hanging point (4,2)
//   of the partitions 1, on the same process, and 3.
// - Partitions 1, 2 and 3: level 1 quads [4,6] x [0,2], [6,8] x [0,2] and [4,6] x [2,4] (area 4).
//   The edge y = 2 of partition 2 has the hanging point (7,2) of the partitions 4 and 5, and the
//   edge x = 6 of partition 3 has the hanging point (6,3) of the partitions 4 and 6.
// - Partitions 4 to 7: level 2 quads [6,8] x [2,4] subdivided 2 x 2 (area 1), by rows.
// Each quad [x0,x1] x [y0,y1] is { x0, y0, x1, y1 }.
const std::array<std::array<double, 4>, 8> QUADS = { {
  { 0, 0, 4, 4 },
  { 4, 0, 6, 2 },
  { 6, 0, 8, 2 },
  { 4, 2, 6, 4 },
  { 6, 2, 7, 3 },
  { 7, 2, 8, 3 },
  { 6, 3, 7, 4 },
  { 7, 3, 8, 4 },
} };

// The quad of a partition. Point data "Values" holds x + 10 y + 100 partition, to check where a
// point comes from: of the copies of a point received from several partitions, the one of the
// first partition is used.
vtkSmartPointer<vtkPolyData> CreatePartition(int partition)
{
  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  vtkNew<vtkDoubleArray> values;
  values->SetName("Values");
  auto add = [&](double x, double y)
  {
    values->InsertNextValue(x + 10.0 * y + 100.0 * partition);
    return points->InsertNextPoint(x, y, 0.0);
  };
  const auto& [x0, y0, x1, y1] = QUADS[partition];
  vtkNew<vtkCellArray> polys;
  const vtkIdType quad[4] = { add(x0, y0), add(x1, y0), add(x1, y1), add(x0, y1) };
  polys->InsertNextCell(4, quad);

  auto pd = vtkSmartPointer<vtkPolyData>::New();
  pd->SetPoints(points);
  pd->SetPolys(polys);
  pd->GetPointData()->AddArray(values);
  return pd;
}

// The partitions of the process `rank`, in the single vtkPartitionedDataSet of a
// vtkPartitionedDataSetCollection.
vtkSmartPointer<vtkPartitionedDataSetCollection> CreatePartitions(int rank)
{
  auto partitions = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
  for (unsigned int i = 0; i < 2; ++i)
  {
    partitions->SetPartition(0, i, CreatePartition(2 * rank + static_cast<int>(i)));
  }
  return partitions;
}

// Expected output of a partition: the numbers of points and cells, and the point received from
// another partition, if any.
struct Expected
{
  vtkIdType NumberOfPoints;
  vtkIdType NumberOfPolygonModeCells;
  vtkIdType NumberOfDecompositionModeCells;
  vtkIdType RemotePointId; // -1 if none
  std::array<double, 3> RemotePoint;
  double RemoteValue;
};

const Expected EXPECTED[] = {
  { 5, 1, 3, 4, { 4, 2, 0 }, 4 + 20 + 100 }, // (4,2) from partition 1
  { 4, 1, 1, -1, { 0, 0, 0 }, 0 },           // unchanged
  { 5, 1, 3, 4, { 7, 2, 0 }, 7 + 20 + 400 }, // (7,2) from partition 4
  { 5, 1, 3, 4, { 6, 3, 0 }, 6 + 30 + 400 }, // (6,3) from partition 4
  { 4, 1, 1, -1, { 0, 0, 0 }, 0 },           // unchanged
  { 4, 1, 1, -1, { 0, 0, 0 }, 0 },           // unchanged
  { 4, 1, 1, -1, { 0, 0, 0 }, 0 },           // unchanged
  { 4, 1, 1, -1, { 0, 0, 0 }, 0 },           // unchanged
};

// Checks of the output of a partition of this process.
bool CheckLocalOutput(const std::string& name, int partition, vtkResolveHangingNodes* filter,
  vtkPolyData* output, bool decompose)
{
  const Expected& expected = EXPECTED[partition];
  const vtkIdType expectedCells =
    decompose ? expected.NumberOfDecompositionModeCells : expected.NumberOfPolygonModeCells;
  if (filter->GetNumberOfLevels() != 3)
  {
    std::cerr << name << ": partition " << partition << " expected 3 levels, got "
              << filter->GetNumberOfLevels() << "\n";
    return false;
  }
  if (!output)
  {
    std::cerr << name << ": partition " << partition << " has no vtkPolyData output\n";
    return false;
  }
  if (output->GetNumberOfPoints() != expected.NumberOfPoints ||
    output->GetNumberOfCells() != expectedCells)
  {
    std::cerr << name << ": partition " << partition << " expected " << expected.NumberOfPoints
              << " points and " << expectedCells << " cells, got " << output->GetNumberOfPoints()
              << " and " << output->GetNumberOfCells() << "\n";
    return false;
  }
  for (vtkIdType c = 0; c < output->GetNumberOfCells(); ++c)
  {
    const int type = output->GetCellType(c);
    if (type != VTK_QUAD && (decompose ? type != VTK_TRIANGLE : type != VTK_POLYGON))
    {
      std::cerr << name << ": partition " << partition << " has a cell of type " << type << "\n";
      return false;
    }
  }
  if (expected.RemotePointId >= 0)
  {
    // The point received from another partition is copied with its point data.
    double p[3];
    output->GetPoint(expected.RemotePointId, p);
    vtkDataArray* values = output->GetPointData()->GetArray("Values");
    if (vtkMath::Distance2BetweenPoints(p, expected.RemotePoint.data()) > EPSILON || !values ||
      std::abs(values->GetTuple1(expected.RemotePointId) - expected.RemoteValue) > EPSILON)
    {
      std::cerr << name << ": partition " << partition << " received a wrong point or point data\n";
      return false;
    }
  }
  if (!output->GetCellData()->GetArray("InverseArea"))
  {
    std::cerr << name << ": partition " << partition << " has no InverseArea cell data array\n";
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
// vtkResolveHangingNodes::New() gives the parallel filter, through the object factory, so that
// the applications creating a vtkResolveHangingNodes, e.g., ParaView, resolve the hanging nodes of
// distributed data.
bool TestObjectFactory()
{
  vtkNew<vtkResolveHangingNodes> filter;
  if (!vtkPResolveHangingNodes::SafeDownCast(filter))
  {
    std::cerr << "ObjectFactory: vtkResolveHangingNodes::New() gives a " << filter->GetClassName()
              << " instead of a vtkPResolveHangingNodes\n";
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
// Polygon and decomposition modes, every partition having cells.
bool TestMode(vtkMultiProcessController* controller, bool decompose)
{
  const std::string name = decompose ? "DecompositionMode" : "PolygonMode";
  const int rank = controller->GetLocalProcessId();
  vtkNew<vtkPResolveHangingNodes> filter;
  filter->SetController(controller);
  filter->SetInputData(CreatePartitions(rank));
  filter->SetDecompose(decompose);
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));

  bool success = true;
  for (unsigned int i = 0; i < 2; ++i)
  {
    const int partition = 2 * rank + static_cast<int>(i);
    vtkPolyData* partitionOutput = vtkPolyData::SafeDownCast(output->GetPartition(0, i));
    success = CheckLocalOutput(name, partition, filter, partitionOutput, decompose) && success;
  }
  if (success && !decompose && rank == 0)
  {
    // The point (4,2) of partition 1 is inserted between the points (4,0) and (4,4).
    const vtkIdType expected[5] = { 0, 1, 4, 2, 3 };
    vtkNew<vtkIdList> ids;
    output->GetPartition(0, 0)->GetCellPoints(0, ids);
    for (vtkIdType i = 0; success && i < 5; ++i)
    {
      success = ids->GetId(i) == expected[i];
    }
    if (!success)
    {
      std::cerr << name << ": partition 0 has a wrong point order\n";
    }
  }
  // All the processes take part in the gather, even if a local check failed.
  success =
    CheckGlobalOutput<2>(name, controller, output, DOMAIN_AREA, DOMAIN_PERIMETER) && success;
  return success;
}

//------------------------------------------------------------------------------
// A process without cells must take part in the exchange of points: process 3 has
// `numEmptyPartitions` partitions without cells (with none, it has a diy block without partition,
// see vtkPResolveHangingNodes::GetRemotePoints), so the domain loses [6,8] x [3,4], and the top
// edges of the partitions 4 and 5 are on its boundary.
bool TestEmptyProcess(vtkMultiProcessController* controller, unsigned int numEmptyPartitions)
{
  const std::string name = "EmptyProcess(" + vtk::to_string(numEmptyPartitions) + " partitions)";
  const int rank = controller->GetLocalProcessId();
  vtkSmartPointer<vtkPartitionedDataSetCollection> input = CreatePartitions(rank);
  if (rank == 3)
  {
    input->SetNumberOfPartitions(0, numEmptyPartitions);
    for (unsigned int i = 0; i < numEmptyPartitions; ++i)
    {
      input->SetPartition(0, i, vtkSmartPointer<vtkPolyData>::New());
    }
  }
  vtkNew<vtkPResolveHangingNodes> filter;
  filter->SetController(controller);
  filter->SetInputData(input);
  filter->SetComputeInverseMeasure(true);
  filter->Update();
  auto output = vtkPartitionedDataSetCollection::SafeDownCast(filter->GetOutputDataObject(0));

  bool success = output->GetNumberOfPartitions(0) == input->GetNumberOfPartitions(0);
  if (!success)
  {
    std::cerr << name << ": process " << rank << " has " << output->GetNumberOfPartitions(0)
              << " output partitions instead of " << input->GetNumberOfPartitions(0) << "\n";
  }
  for (unsigned int i = 0; success && i < output->GetNumberOfPartitions(0); ++i)
  {
    const int partition = 2 * rank + static_cast<int>(i);
    vtkDataSet* partitionOutput = output->GetPartition(0, i);
    if (!partitionOutput ||
      (rank == 3 ? partitionOutput->GetNumberOfCells() != 0
                 : partitionOutput->GetNumberOfPoints() != EXPECTED[partition].NumberOfPoints))
    {
      std::cerr << name << ": partition " << partition << " has a wrong output\n";
      success = false;
    }
  }
  success =
    CheckGlobalOutput<2>(name, controller, output, DOMAIN_AREA - 2.0, DOMAIN_PERIMETER) && success;
  return success;
}
} // namespace

int TestPResolveHangingNodes2DUG(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  int success = 1;
  if (controller->GetNumberOfProcesses() != 4)
  {
    std::cerr << "This test needs 4 processes, running with " << controller->GetNumberOfProcesses()
              << "\n";
    success = 0;
  }
  else
  {
    success = TestObjectFactory() ? 1 : 0;
    success = TestMode(controller, false) && success;
    success = TestMode(controller, true) && success;
    success = TestEmptyProcess(controller, 2) && success;
    success = TestEmptyProcess(controller, 0) && success;
    // All the processes report the same result.
    int globalSuccess = 0;
    controller->AllReduce(&success, &globalSuccess, 1, vtkCommunicator::MIN_OP);
    success = globalSuccess;
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  controller->Finalize();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
