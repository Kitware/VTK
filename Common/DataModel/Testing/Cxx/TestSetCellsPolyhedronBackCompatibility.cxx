// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#define VTK_DEPRECATION_LEVEL 0
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSetMapper.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkPolygon.h"
#include "vtkPolyhedron.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include "vtkTestUtilities.h"

#define compare_id(x, y) ((x) - (y) == 0)

// Test of vtkPolyhedron directly set with backward compatible SetCells
int TestSetCellsPolyhedronBackCompatibility(int, char*[])
{
  // create the a cube
  vtkNew<vtkPoints> cubeLikePts;
  cubeLikePts->InsertNextPoint(0., 10., 0.);
  cubeLikePts->InsertNextPoint(10., 10., 0.);
  cubeLikePts->InsertNextPoint(10., -10., 0.);
  cubeLikePts->InsertNextPoint(0., -10., 0.);
  cubeLikePts->InsertNextPoint(0., 10., 20.);
  cubeLikePts->InsertNextPoint(10., 10., 20.);
  cubeLikePts->InsertNextPoint(10., -10., 20.);
  cubeLikePts->InsertNextPoint(0., -10., 20.);

  // create a test polyhedron
  vtkIdType pointIds[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->InsertNextValue(VTK_POLYHEDRON);

  vtkNew<vtkCellArray> cells;
  cells->InsertNextCell(8, pointIds);

  vtkNew<vtkIdTypeArray> faces;
  vtkNew<vtkIdTypeArray> faceLocations;
  faceLocations->InsertNextValue(1);
  // First value will be ignored
  faces->InsertNextValue(-1);
  // Number of faces
  faces->InsertNextValue(6);
  // face0
  faces->InsertNextValue(4);
  faces->InsertNextValue(4);
  faces->InsertNextValue(5);
  faces->InsertNextValue(6);
  faces->InsertNextValue(7);
  // face1
  faces->InsertNextValue(4);
  faces->InsertNextValue(1);
  faces->InsertNextValue(2);
  faces->InsertNextValue(6);
  faces->InsertNextValue(5);
  // face2
  faces->InsertNextValue(4);
  faces->InsertNextValue(7);
  faces->InsertNextValue(3);
  faces->InsertNextValue(0);
  faces->InsertNextValue(4);
  // face3
  faces->InsertNextValue(4);
  faces->InsertNextValue(1);
  faces->InsertNextValue(0);
  faces->InsertNextValue(3);
  faces->InsertNextValue(2);
  // face4
  faces->InsertNextValue(4);
  faces->InsertNextValue(0);
  faces->InsertNextValue(1);
  faces->InsertNextValue(5);
  faces->InsertNextValue(4);
  // face5
  faces->InsertNextValue(4);
  faces->InsertNextValue(2);
  faces->InsertNextValue(3);
  faces->InsertNextValue(7);
  faces->InsertNextValue(6);

  // Add garbage that should not be read by SetCells
  faces->InsertNextValue(-1000);
  faces->InsertNextValue(-1000);
  faces->InsertNextValue(-1000);
  faces->InsertNextValue(-1000);

  vtkSmartPointer<vtkUnstructuredGrid> ugrid0 = vtkSmartPointer<vtkUnstructuredGrid>::New();
  ugrid0->SetPoints(cubeLikePts);
  ugrid0->SetCells(cellTypes, cells, faceLocations, faces);

  vtkPolyhedron* polyhedron = static_cast<vtkPolyhedron*>(ugrid0->GetCell(0));

  vtkCellArray* cell = ugrid0->GetCells();
  vtkNew<vtkIdTypeArray> pids;
  cell->ExportLegacyFormat(pids);
  std::cout << "num of cells: " << cell->GetNumberOfCells() << std::endl;
  std::cout << "num of tuples: " << pids->GetNumberOfTuples() << std::endl;
  for (int i = 0; i < pids->GetNumberOfTuples(); i++)
  {
    std::cout << pids->GetValue(i) << " ";
  }
  std::cout << std::endl;

  // Print out basic information
  std::cout << "Testing polyhedron is a cube with bounds "
            << "[0, 10, -10, 10, 0, 20]. It has " << polyhedron->GetNumberOfEdges() << " edges and "
            << polyhedron->GetNumberOfFaces() << " faces." << std::endl;

  // Print face information
  for (int i = 0; i < polyhedron->GetNumberOfFaces(); ++i)
  {
    std::cout << "Face " << i << ":" << std::endl;
    vtkPolygon* pgon = static_cast<vtkPolygon*>(polyhedron->GetFace(i));
    for (int j = 0; j < pgon->PointIds->GetNumberOfIds(); ++j)
    {
      std::cout << pgon->PointIds->GetId(j) << " ";
    }
    std::cout << std::endl;
  }

  // Do Compare the last face that could be corrupted
  {
    vtkIdType reference[4] = { 2, 3, 7, 6 };
    vtkPolygon* pgon =
      static_cast<vtkPolygon*>(polyhedron->GetFace(polyhedron->GetNumberOfFaces() - 1));
    for (int j = 0; j < pgon->PointIds->GetNumberOfIds(); ++j)
    {
      if (!compare_id(pgon->PointIds->GetId(j), reference[j]))
      {
        std::cout << "Error setting the faces on the polyhedron." << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
