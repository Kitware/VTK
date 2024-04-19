// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtksys/FStream.hxx"
#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkIdList.h>
#include <vtkLogger.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkStringArray.h>
#include <vtkTestUtilities.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLPUnstructuredGridReader.h>
#include <vtkXMLPUnstructuredGridWriter.h>
#include <vtkXMLUnstructuredGridReader.h>

#include <string>

bool CompareGrids(vtkUnstructuredGrid* s, vtkUnstructuredGrid* t)
{
  if (s->GetNumberOfCells() != t->GetNumberOfCells())
  {
    cerr << "The number of cells does not match: " << s->GetNumberOfCells()
         << " != " << t->GetNumberOfCells() << endl;
    return false;
  }
  vtkStringArray* helloArrayS =
    vtkArrayDownCast<vtkStringArray>(s->GetCellData()->GetAbstractArray("helloArray"));
  vtkStringArray* helloArrayT =
    vtkArrayDownCast<vtkStringArray>(t->GetCellData()->GetAbstractArray("helloArray"));
  for (vtkIdType i = 0; i < s->GetNumberOfCells(); ++i)
  {
    if (helloArrayS->GetValue(i) != helloArrayT->GetValue(i))
    {
      std::cerr << "String array does not match:" << helloArrayS->GetValue(i)
                << " != " << helloArrayT->GetValue(i) << std::endl;
      return false;
    }
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

  vtkStringArray* alphaArrayS =
    vtkArrayDownCast<vtkStringArray>(s->GetPointData()->GetAbstractArray("alphaArray"));
  vtkStringArray* alphaArrayT =
    vtkArrayDownCast<vtkStringArray>(t->GetPointData()->GetAbstractArray("alphaArray"));
  for (vtkIdType i = 0; i < s->GetNumberOfPoints(); ++i)
  {
    if (alphaArrayS->GetValue(i) != alphaArrayT->GetValue(i))
    {
      std::cerr << "String array does not match:" << alphaArrayS->GetValue(i)
                << " != " << alphaArrayT->GetValue(i) << std::endl;
      return false;
    }
  }

  vtkFloatArray* numArrayS =
    vtkArrayDownCast<vtkFloatArray>(s->GetPointData()->GetAbstractArray("my_point_data"));
  vtkFloatArray* numArrayT =
    vtkArrayDownCast<vtkFloatArray>(t->GetPointData()->GetAbstractArray("my_point_data"));

  if (numArrayS->GetNumberOfComponents() != numArrayT->GetNumberOfComponents())
  {
    std::cerr << "The number of components is different:" << numArrayS->GetNumberOfComponents()
              << " != " << numArrayT->GetNumberOfComponents() << std::endl;
    return false;
  }

  for (vtkIdType component_i = 0; component_i < 2; ++component_i)
  {
    if (std::string(numArrayS->GetComponentName(component_i)) !=
      std::string(numArrayT->GetComponentName(component_i)))
    {
      std::cerr << "The component names are different:" << numArrayS->GetComponentName(component_i)
                << " != " << numArrayT->GetComponentName(component_i) << std::endl;
      return false;
    }

    for (vtkIdType i = 0; i < s->GetNumberOfPoints(); ++i)
    {
      if (numArrayS->GetTuple(i)[component_i] != numArrayT->GetTuple(i)[component_i])
      {
        std::cerr << "0 Num array does not match:" << numArrayS->GetTuple(i)[component_i]
                  << " != " << numArrayT->GetTuple(i)[component_i] << std::endl;
        return false;
      }
    }
  }
  return true;
}

int TestParallelUnstructuredGridIO(int argc, char* argv[])
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

  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);

  vtkSmartPointer<vtkFloatArray> point_data = vtkSmartPointer<vtkFloatArray>::New();
  point_data->SetName("my_point_data");
  point_data->SetNumberOfComponents(2);
  point_data->SetComponentName(0, "point_data_0");
  point_data->SetComponentName(1, "point_data_1");
  point_data->InsertNextTuple2(0.0, 10.0);
  point_data->InsertNextTuple2(1.0, 11.0);
  point_data->InsertNextTuple2(2.0, 12.0);
  point_data->InsertNextTuple2(3.0, 13.0);
  point_data->InsertNextTuple2(4.0, 14.0);
  point_data->InsertNextTuple2(5.0, 15.0);
  point_data->InsertNextTuple2(6.0, 16.0);
  point_data->InsertNextTuple2(7.0, 17.0);
  point_data->InsertNextTuple2(8.0, 18.0);
  point_data->InsertNextTuple2(9.0, 19.0);
  ug->GetPointData()->AddArray(point_data);

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

  // String array for cells
  vtkNew<vtkStringArray> helloArray;
  helloArray->SetNumberOfTuples(3);
  helloArray->SetValue(0, "hello.");
  helloArray->SetValue(1, "Hello..");
  helloArray->SetValue(2, "HELLO...");
  helloArray->SetName("helloArray");

  ug->GetCellData()->AddArray(helloArray);

  // String array for points
  vtkNew<vtkStringArray> alphaArray;
  alphaArray->SetNumberOfTuples(10);
  alphaArray->SetValue(0, "alpha");
  alphaArray->SetValue(1, "beta");
  alphaArray->SetValue(2, "gamma");
  alphaArray->SetValue(3, "delta");
  alphaArray->SetValue(4, "epsilon");
  alphaArray->SetValue(5, "zeta");
  alphaArray->SetValue(6, "eta");
  alphaArray->SetValue(7, "theta");
  alphaArray->SetValue(8, "iota");
  alphaArray->SetValue(9, "kappa");
  alphaArray->SetName("alphaArray");

  ug->GetPointData()->AddArray(alphaArray);

  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
  vtkNew<vtkXMLPUnstructuredGridWriter> w;
  w->SetController(ctrl);
  w->SetInputData(ug);
  w->SetUseSubdirectory(true);
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string dir(tempDir);
  std::string fn = dir + "/ug.pvtu";
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

  vtkNew<vtkXMLUnstructuredGridReader> r;
  std::string piece = dir + "/ug/ug_0.vtu";
  r->SetFileName(piece.c_str());
  r->Update();

  // first try reading the piece with a non-parallel reader
  vtkUnstructuredGrid* read = r->GetOutput();
  vtkLogF(INFO, "Comparing original with .vtu");
  if (!CompareGrids(ug.GetPointer(), read))
    return EXIT_FAILURE;

  // now read the .pvtu file with the parallel reader
  vtkNew<vtkXMLPUnstructuredGridReader> pr;
  pr->SetFileName(fn.c_str());
  // this will give a SIGSEGV on vtkXMLPUnstructuredGridReader::ReadPieceData()
  pr->Update();

  read = pr->GetOutput();
  vtkLogF(INFO, "Comparing original with .pvtu");
  if (!CompareGrids(ug.GetPointer(), read))
    return EXIT_FAILURE;

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return EXIT_SUCCESS;
}
