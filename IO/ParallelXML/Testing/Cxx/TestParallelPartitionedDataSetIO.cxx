/*==============================================================================

  Program:   Visualization Toolkit
  Module:    TestParallelPartitionedDataSetIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkCell.h"
#include "vtkIdList.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPoints.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLPPartitionedDataSetWriter.h"
#include "vtkXMLPartitionedDataSetReader.h"
#include "vtksys/FStream.hxx"

#include <string>

using namespace std;

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

  vtkNew<vtkIdList> faces;
  // top face of four points
  faces->InsertNextId(4);

  faces->InsertNextId(4);
  faces->InsertNextId(5);
  faces->InsertNextId(6);
  faces->InsertNextId(7);

  // four triangle side faces, each of three points
  faces->InsertNextId(3);
  faces->InsertNextId(4);
  faces->InsertNextId(5);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(5);
  faces->InsertNextId(6);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(6);
  faces->InsertNextId(7);
  faces->InsertNextId(8);

  faces->InsertNextId(3);
  faces->InsertNextId(7);
  faces->InsertNextId(4);
  faces->InsertNextId(8);

  // insert the polyhedron cell
  ug->InsertNextCell(
    VTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), 5, faces.GetPointer()->GetPointer(0));

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
  faces->InsertNextId(4);

  faces->InsertNextId(0);
  faces->InsertNextId(1);
  faces->InsertNextId(2);
  faces->InsertNextId(3);

  // four side faces, each of three points
  faces->InsertNextId(3);
  faces->InsertNextId(0);
  faces->InsertNextId(1);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(1);
  faces->InsertNextId(2);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(2);
  faces->InsertNextId(3);
  faces->InsertNextId(9);

  faces->InsertNextId(3);
  faces->InsertNextId(3);
  faces->InsertNextId(0);
  faces->InsertNextId(9);

  // insert the cell. We now have two pyramids with a cube in between
  ug->InsertNextCell(
    VTK_POLYHEDRON, 5, ids.GetPointer()->GetPointer(0), 5, faces.GetPointer()->GetPointer(0));

  vtkNew<vtkPartitionedDataSet> pds;
  pds->SetNumberOfPartitions(1);
  pds->SetPartition(0, ug);

  vtkNew<vtkXMLPPartitionedDataSetWriter> w;
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
  vtksys::ifstream f(fn.c_str());
  if (!f.good())
  {
    std::cerr << "File " << fn << " does not exist." << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkXMLPartitionedDataSetReader> r;
  std::string piece = dir + "/pds.vtpd";
  r->SetFileName(piece.c_str());
  r->Update();

  // first try reading the piece with a non-parallel reader
  vtkPartitionedDataSet* read = vtkPartitionedDataSet::SafeDownCast(r->GetOutput());
  cout << "Comparing original with .vtu" << endl;
  bool retVal =
    CompareGrids(ug.GetPointer(), vtkUnstructuredGrid::SafeDownCast(read->GetPartition(0)));

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return !retVal;
}
