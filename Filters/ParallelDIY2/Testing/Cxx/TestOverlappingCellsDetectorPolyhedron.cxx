// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Verify that vtkOverlappingCellsDetector preserves the face topology of
// VTK_POLYHEDRON cells when it exchanges candidate cells between ranks.
//
// The candidate cells are gathered with vtkCell::GetPointIds(), which for a
// polyhedron is its point list and does not describe its faces. Handing that
// point list to the two argument vtkUnstructuredGrid::SetCells() makes it be
// reinterpreted as a legacy face stream.
//
// Each rank owns one hexahedron expressed as a polyhedron. The two overlap, so
// each rank must ship its cell to the other and report exactly one overlap.

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkGenerateGlobalIds.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkOverlappingCellsDetector.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <cstdlib>

namespace
{
constexpr int FACES_PER_CELL = 6;
constexpr int POINTS_PER_FACE = 4;
constexpr int POINTS_PER_CELL = 8;

// Faces of a hexahedron against its eight corners in VTK ordering.
const int HexahedronFaces[FACES_PER_CELL][POINTS_PER_FACE] = { { 0, 3, 2, 1 }, { 4, 5, 6, 7 },
  { 0, 1, 5, 4 }, { 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 3, 0, 4, 7 } };

//------------------------------------------------------------------------------
// One unit hexahedron with its low x corner at xOffset, as a VTK_POLYHEDRON.
vtkSmartPointer<vtkUnstructuredGrid> BuildOnePolyhedron(double xOffset)
{
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(POINTS_PER_CELL);
  const double corners[POINTS_PER_CELL][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 },
    { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } };
  for (int p = 0; p < POINTS_PER_CELL; ++p)
  {
    points->SetPoint(p, corners[p][0] + xOffset, corners[p][1], corners[p][2]);
  }

  vtkNew<vtkCellArray> cells;
  vtkNew<vtkCellArray> faces;
  vtkNew<vtkCellArray> faceLocations;
  vtkNew<vtkUnsignedCharArray> types;

  std::array<vtkIdType, POINTS_PER_CELL> pointIds;
  for (int p = 0; p < POINTS_PER_CELL; ++p)
  {
    pointIds[p] = p;
  }
  cells->InsertNextCell(POINTS_PER_CELL, pointIds.data());
  types->InsertNextValue(VTK_POLYHEDRON);

  std::array<vtkIdType, FACES_PER_CELL> globalFaceIds;
  for (int f = 0; f < FACES_PER_CELL; ++f)
  {
    std::array<vtkIdType, POINTS_PER_FACE> facePoints;
    for (int p = 0; p < POINTS_PER_FACE; ++p)
    {
      facePoints[p] = HexahedronFaces[f][p];
    }
    globalFaceIds[f] = faces->InsertNextCell(POINTS_PER_FACE, facePoints.data());
  }
  faceLocations->InsertNextCell(FACES_PER_CELL, globalFaceIds.data());

  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);
  grid->SetPolyhedralCells(types, cells, faceLocations, faces);
  return grid;
}
} // anonymous namespace

//------------------------------------------------------------------------------
int TestOverlappingCellsDetectorPolyhedron(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  int retVal = EXIT_SUCCESS;
  const int myrank = contr->GetLocalProcessId();

  // Rank 0 spans x in [0, 1], rank 1 spans x in [0.5, 1.5]. They overlap.
  auto grid = BuildOnePolyhedron(myrank == 0 ? 0.0 : 0.5);

  vtkNew<vtkGenerateGlobalIds> globalIds;
  globalIds->SetInputDataObject(grid);

  vtkNew<vtkOverlappingCellsDetector> detector;
  detector->SetInputConnection(globalIds->GetOutputPort());
  detector->Update();

  auto* output = vtkUnstructuredGrid::SafeDownCast(detector->GetOutput(0));
  if (!output)
  {
    vtkLog(ERROR, "No output grid.");
    contr->Finalize();
    vtkMultiProcessController::SetGlobalController(nullptr);
    return EXIT_FAILURE;
  }

  // The cell we own must have survived the round trip intact.
  for (vtkIdType cellId = 0; cellId < output->GetNumberOfCells(); ++cellId)
  {
    if (output->GetCellType(cellId) != VTK_POLYHEDRON)
    {
      vtkLog(ERROR, "Cell " << cellId << " is no longer a polyhedron.");
      retVal = EXIT_FAILURE;
      continue;
    }
    vtkNew<vtkCellArray> cellFaces;
    output->GetPolyhedronFaces(cellId, cellFaces);
    if (cellFaces->GetNumberOfCells() != FACES_PER_CELL)
    {
      vtkLog(ERROR,
        "Cell " << cellId << " has " << cellFaces->GetNumberOfCells() << " faces, expected "
                << FACES_PER_CELL << ".");
      retVal = EXIT_FAILURE;
    }
  }

  // The two polyhedra overlap, so each rank must report exactly one overlap.
  // Without the exchanged faces the candidate cell's topology is garbage and
  // the intersection test cannot come out right.
  vtkDataArray* overlaps =
    output->GetCellData()->GetArray(detector->GetNumberOfOverlapsPerCellArrayName());
  if (!overlaps)
  {
    vtkLog(ERROR, "Missing the overlap count array.");
    retVal = EXIT_FAILURE;
  }
  else
  {
    for (vtkIdType cellId = 0; cellId < overlaps->GetNumberOfTuples(); ++cellId)
    {
      const double count = overlaps->GetTuple1(cellId);
      if (count != 1.0)
      {
        vtkLog(ERROR,
          "Rank " << myrank << " cell " << cellId << " reports " << count
                  << " overlaps, expected 1.");
        retVal = EXIT_FAILURE;
      }
    }
  }

  contr->Finalize();
  vtkMultiProcessController::SetGlobalController(nullptr);
  return retVal;
}
