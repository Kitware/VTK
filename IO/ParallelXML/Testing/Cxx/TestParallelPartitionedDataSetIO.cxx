// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkCell.h"
#include "vtkIdList.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPoints.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLPartitionedDataSetReader.h"
#include "vtkXMLPartitionedDataSetWriter.h"
#include "vtksys/FStream.hxx"

#include <string>

namespace
{
bool CompareGrids(vtkUnstructuredGrid* s, vtkUnstructuredGrid* t)
{
  if (s->GetNumberOfCells() != t->GetNumberOfCells())
  {
    cerr << "The number of cells does not match: " << s->GetNumberOfCells()
         << " != " << t->GetNumberOfCells() << endl;
    return false;
  }
  for (vtkIdType i = 0; i < s->GetNumberOfCells(); ++i)
  {
    if (s->GetCellType(i) != t->GetCellType(i))
    {
      cerr << "The cell type does not match: " << s->GetCellType(i) << " != " << t->GetCellType(i)
           << endl;
      return false;
    }
    vtkNew<vtkIdList> sIds, tIds;
    if (s->GetCellType(i) == VTK_POLYHEDRON)
    {
      s->GetFaceStream(i, sIds.GetPointer());
      t->GetFaceStream(i, tIds.GetPointer());
    }
    else
    {
      s->GetCellPoints(i, sIds.GetPointer());
      t->GetCellPoints(i, tIds.GetPointer());
    }
    if (sIds->GetNumberOfIds() != tIds->GetNumberOfIds())
    {
      cerr << "Cell type : " << s->GetCellType(i) << endl;
      cerr << "The number of ids does not match: " << sIds->GetNumberOfIds()
           << " != " << tIds->GetNumberOfIds() << endl;
      return false;
    }

    for (vtkIdType j = 0; j < sIds->GetNumberOfIds(); ++j)
    {
      vtkIdType sId = sIds->GetId(j);
      vtkIdType tId = tIds->GetId(j);

      if (sId != tId)
      {
        cerr << "Cell type : " << s->GetCellType(i) << endl;
        cerr << "The id at position " << j << " does not match: " << sId << " != " << tId << endl;
        return false;
      }
    }
  }

  return true;
}
} // anonymous namespace

int TestParallelPartitionedDataSetIO(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif
  contr->Initialize(&argc, &argv);
  vtkLogger::SetThreadName("rank=" + std::to_string(contr->GetLocalProcessId()));
  vtkMultiProcessController::SetGlobalController(contr);

  vtkNew<vtkPoints> points;

  if (contr->GetLocalProcessId() == 0)
  {
    points->InsertNextPoint(0, 0, 0);
    points->InsertNextPoint(1, 0, 0);
    points->InsertNextPoint(1, 1, 0);
    points->InsertNextPoint(0, 1, 0);

    points->InsertNextPoint(0, 0, 1);
    points->InsertNextPoint(1, 0, 1);
    points->InsertNextPoint(1, 1, 1);
    points->InsertNextPoint(0, 1, 1);

    points->InsertNextPoint(.5, .5, 2);
    points->InsertNextPoint(.5, .5, -1);
  }
  else if (contr->GetLocalProcessId() == 1)
  {
    points->InsertNextPoint(0, 0, 4);
    points->InsertNextPoint(1, 0, 4);
    points->InsertNextPoint(1, 1, 4);
    points->InsertNextPoint(0, 1, 4);

    points->InsertNextPoint(0, 0, 5);
    points->InsertNextPoint(1, 0, 5);
    points->InsertNextPoint(1, 1, 5);
    points->InsertNextPoint(0, 1, 5);

    points->InsertNextPoint(.5, .5, 6);
    points->InsertNextPoint(.5, .5, 3);
  }

  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);

  ug->Allocate(3); // allocate for 3 cells

  vtkNew<vtkIdList> ids;

  // add a hexahedron of the first 8 points (i.e. a cube)
  ids->InsertNextId(0);
  ids->InsertNextId(1);
  ids->InsertNextId(2);
  ids->InsertNextId(3);
  ids->InsertNextId(4);
  ids->InsertNextId(5);
  ids->InsertNextId(6);
  ids->InsertNextId(7);
  ug->InsertNextCell(VTK_HEXAHEDRON, ids.GetPointer());
  ids->Reset();

  // add a polyhedron comprise of the top hexahedron face
  // and four triangles to the 9th point
  ids->InsertNextId(4);
  ids->InsertNextId(5);
  ids->InsertNextId(6);
  ids->InsertNextId(7);
  ids->InsertNextId(8);

  vtkNew<vtkCellArray> faces;
  // top face of four points
  faces->InsertNextCell(4);

  faces->InsertCellPoint(4);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(7);

  // four triangle side faces, each of three points
  faces->InsertNextCell(3);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(8);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(8);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(8);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(8);

  // insert the polyhedron cell
  ug->InsertNextCell(VTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), faces);

  // put another pyramid on the bottom towards the 10th point
  faces->Reset();
  ids->Reset();

  // the list of points that the pyramid references
  ids->InsertNextId(0);
  ids->InsertNextId(1);
  ids->InsertNextId(2);
  ids->InsertNextId(3);
  ids->InsertNextId(9);

  // bottom face of four points
  faces->InsertNextCell(4);

  faces->InsertCellPoint(0);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(3);

  // four side faces, each of three points
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(9);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(9);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(9);

  faces->InsertNextCell(3);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(9);

  // insert the cell. We now have two pyramids with a cube in between
  ug->InsertNextCell(VTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), faces);

  vtkNew<vtkPartitionedDataSet> pds;
  pds->SetNumberOfPartitions(1);
  pds->SetPartition(0, ug);

  vtkNew<vtkXMLPartitionedDataSetWriter> w;
  w->SetController(contr);
  w->SetInputData(pds);
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string dir(tempDir);
  std::string fn = dir + "/pds.vtpd";
  w->SetFileName(fn.c_str());
  w->SetDataModeToAscii();
  w->Update();
  delete[] tempDir;

  // Barrier needed to ensure we don't start checking for files before the
  // writer has written them out on all ranks.
  contr->Barrier();

  vtksys::ifstream f(fn.c_str());
  if (!f.good())
  {
    vtkLogF(ERROR, "File '%s' does not exist!", fn.c_str());
    return EXIT_FAILURE;
  }

  vtkNew<vtkXMLPartitionedDataSetReader> r;
  std::string piece = dir + "/pds.vtpd";
  r->SetFileName(piece.c_str());
  r->Update();

  // first try reading the piece with a non-parallel reader
  vtkPartitionedDataSet* read = vtkPartitionedDataSet::SafeDownCast(r->GetOutput());
  vtkLogF(INFO, "Comparing original with .vtu");
  bool retVal =
    CompareGrids(ug.GetPointer(), vtkUnstructuredGrid::SafeDownCast(read->GetPartition(0)));

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return !retVal;
}
