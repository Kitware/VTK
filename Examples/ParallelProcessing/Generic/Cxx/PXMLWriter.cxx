// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// Minimal example: write a parallel XML unstructured-grid file (.pvtu) from
// an MPI program. Each rank contributes one piece; rank 0 also writes the
// summary file. Run with e.g.
//
//   mpiexec -n 4 ./PXMLWriter output.pvtu
//
// The two settings that bite first-time users are:
//   1. SetNumberOfPieces / SetStartPiece / SetEndPiece are not derived from
//      the controller automatically; they must be set on every rank.
//   2. The writer needs a vtkMultiProcessController, either via
//      SetGlobalController() or SetController(); otherwise it silently
//      falls back to single-process behavior.

#include "vtkCellArray.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLPUnstructuredGridWriter.h"

#include <vtk_mpi.h>

#include <iostream>
#include <string>

namespace
{

// Build a tiny per-rank grid: one tetrahedron whose vertex positions encode
// the rank, so the assembled .pvtu visibly contains contributions from every
// process.
vtkSmartPointer<vtkUnstructuredGrid> MakePieceForRank(int rank)
{
  vtkNew<vtkPoints> points;
  const double z = static_cast<double>(rank);
  points->InsertNextPoint(0.0, 0.0, z);
  points->InsertNextPoint(1.0, 0.0, z);
  points->InsertNextPoint(0.0, 1.0, z);
  points->InsertNextPoint(0.0, 0.0, z + 1.0);

  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);
  ug->Allocate(1);
  const vtkIdType tet[4] = { 0, 1, 2, 3 };
  ug->InsertNextCell(VTK_TETRA, 4, tet);

  return ug;
}

} // namespace

int main(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  const int rank = controller->GetLocalProcessId();
  const int nproc = controller->GetNumberOfProcesses();

  const std::string fileName = (argc > 1) ? argv[1] : "PXMLWriterOutput.pvtu";

  auto piece = MakePieceForRank(rank);

  vtkNew<vtkXMLPUnstructuredGridWriter> writer;
  writer->SetFileName(fileName.c_str());
  writer->SetInputData(piece);
  writer->SetNumberOfPieces(nproc);
  writer->SetStartPiece(rank);
  writer->SetEndPiece(rank);
  // SetController is not strictly required because the global controller is
  // installed above, but doing it explicitly makes the dependency obvious.
  writer->SetController(controller);
  writer->Write();

  if (rank == 0)
  {
    std::cout << "Wrote " << fileName << " with " << nproc << " piece(s)." << '\n';
  }

  controller->Finalize();
  return 0;
}
