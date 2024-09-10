// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellType.h"
#include "vtkCellTypeSource.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include "vtkPoints.h"

#include "vtkTestUtilities.h"
#include <string>

vtkSmartPointer<vtkUnstructuredGrid> generateIcosidodecaheron()
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0., 0.999999777, 0.);
  points->InsertNextPoint(0.587785257, 0.809016978, 0.);
  points->InsertNextPoint(0.262865551, 0.809016978, 0.525731102);
  points->InsertNextPoint(-0.587785257, 0.809016978, 0.);
  points->InsertNextPoint(-0.262865551, 0.809016978, -0.525731102);
  points->InsertNextPoint(0.688190955, 0.499999983, -0.525731102);
  points->InsertNextPoint(0.951056507, 0.309016995, 0);
  points->InsertNextPoint(0.425325404, 0.309016995, 0.850650808);
  points->InsertNextPoint(-0.162459853, 0.499999983, 0.850650808);
  points->InsertNextPoint(-0.688190955, 0.499999983, 0.525731102);
  points->InsertNextPoint(-0.951056507, 0.309016995, 0.);
  points->InsertNextPoint(-0.425325404, 0.309016995, -0.850650808);
  points->InsertNextPoint(0.162459853, 0.499999983, -0.850650808);
  points->InsertNextPoint(0.951056507, -0.309016995, 0.);
  points->InsertNextPoint(0.850650808, 0., 0.525731102);
  points->InsertNextPoint(0.525731102, 0., -0.850650808);
  points->InsertNextPoint(-0.525731102, 0., 0.850650808);
  points->InsertNextPoint(0.425325404, -0.309016995, 0.850650808);
  points->InsertNextPoint(-0.951056507, -0.309016995, 0.);
  points->InsertNextPoint(-0.850650808, 0., -0.525731102);
  points->InsertNextPoint(-0.425325404, -0.309016995, -0.850650808);
  points->InsertNextPoint(0.688190955, -0.499999983, -0.525731102);
  points->InsertNextPoint(0.587785257, -0.809016978, 0.);
  points->InsertNextPoint(0.162459853, -0.499999983, -0.850650808);
  points->InsertNextPoint(-0.162459853, -0.499999983, 0.850650808);
  points->InsertNextPoint(-0.688190955, -0.499999983, 0.525731102);
  points->InsertNextPoint(0.262865551, -0.809016978, 0.525731102);
  points->InsertNextPoint(-0.587785257, -0.809016978, 0.);
  points->InsertNextPoint(-0.262865551, -0.809016978, -0.525731102);
  points->InsertNextPoint(0., -0.999999777, 0.);

  vtkSmartPointer<vtkCellArray> connectivity = vtkSmartPointer<vtkCellArray>::New();
  connectivity->InsertNextCell(30);
  for (vtkIdType ii = 0; ii < 30; ++ii)
  {
    connectivity->InsertCellPoint(ii);
  }

  vtkSmartPointer<vtkCellArray> faces = vtkSmartPointer<vtkCellArray>::New();
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(2);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(4);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(6);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(8);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(10);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(12);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(13);
  faces->InsertCellPoint(14);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(12);
  faces->InsertCellPoint(15);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(16);
  faces->InsertCellPoint(9);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(14);
  faces->InsertCellPoint(17);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(18);
  faces->InsertCellPoint(19);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(19);
  faces->InsertCellPoint(20);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(13);
  faces->InsertCellPoint(21);
  faces->InsertCellPoint(22);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(15);
  faces->InsertCellPoint(23);
  faces->InsertCellPoint(21);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(16);
  faces->InsertCellPoint(24);
  faces->InsertCellPoint(25);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(17);
  faces->InsertCellPoint(26);
  faces->InsertCellPoint(24);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(18);
  faces->InsertCellPoint(25);
  faces->InsertCellPoint(27);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(20);
  faces->InsertCellPoint(28);
  faces->InsertCellPoint(23);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(22);
  faces->InsertCellPoint(29);
  faces->InsertCellPoint(26);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(27);
  faces->InsertCellPoint(29);
  faces->InsertCellPoint(28);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(3);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(12);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(1);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(14);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(2);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(19);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(4);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(15);
  faces->InsertCellPoint(21);
  faces->InsertCellPoint(13);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(17);
  faces->InsertCellPoint(24);
  faces->InsertCellPoint(16);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(16);
  faces->InsertCellPoint(25);
  faces->InsertCellPoint(18);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(12);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(20);
  faces->InsertCellPoint(23);
  faces->InsertCellPoint(15);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(13);
  faces->InsertCellPoint(22);
  faces->InsertCellPoint(26);
  faces->InsertCellPoint(17);
  faces->InsertCellPoint(14);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(18);
  faces->InsertCellPoint(27);
  faces->InsertCellPoint(28);
  faces->InsertCellPoint(20);
  faces->InsertCellPoint(19);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(22);
  faces->InsertCellPoint(21);
  faces->InsertCellPoint(23);
  faces->InsertCellPoint(28);
  faces->InsertCellPoint(29);
  faces->InsertNextCell(5);
  faces->InsertCellPoint(25);
  faces->InsertCellPoint(24);
  faces->InsertCellPoint(26);
  faces->InsertCellPoint(29);
  faces->InsertCellPoint(27);

  vtkSmartPointer<vtkCellArray> poly_to_faces = vtkSmartPointer<vtkCellArray>::New();
  poly_to_faces->InsertNextCell(32);
  for (vtkIdType ii = 0; ii < 32; ++ii)
  {
    poly_to_faces->InsertCellPoint(ii);
  }

  vtkSmartPointer<vtkUnsignedCharArray> ctypes = vtkSmartPointer<vtkUnsignedCharArray>::New();
  ctypes->SetNumberOfValues(1);
  ctypes->SetValue(0, VTK_POLYHEDRON);

  vtkSmartPointer<vtkUnstructuredGrid> uns = vtkSmartPointer<vtkUnstructuredGrid>::New();
  uns->SetPoints(points);
  uns->SetPolyhedralCells(ctypes, connectivity, poly_to_faces, faces);

  return uns;
}

int TestXMLPolyhedronUnstructuredGrid(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string fileName(tempDir);
  delete[] tempDir;

  fileName += "/XMLIcosidodecaheronUnstructuredGrid.vtu";
  //
  // Create a UnstructuredGrid with a polyhedron, write the UnstructuredGrid, read the
  // UnstructuredGrid and compare the cell count and face count
  vtkSmartPointer<vtkUnstructuredGrid> IcosidodecaheronGrid = generateIcosidodecaheron();

  std::cout << "Write to " << fileName << std::endl;

  // write the UnstructuredGrid
  vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
    vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  writer->SetInputData(IcosidodecaheronGrid);
  writer->SetFileName(fileName.c_str());
  //
  writer->SetDataModeToBinary();
  writer->SetCompressorTypeToNone();
  writer->Write();

  // read back the UnstructuredGrid
  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
    vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
  reader->SetFileName(fileName.c_str());

  if (!reader->CanReadFile(fileName.c_str()))
  {
    std::cerr << "CanReadFile failed" << std::endl;
    return EXIT_FAILURE;
  }
  reader->Update();

  if (IcosidodecaheronGrid->GetNumberOfCells() != reader->GetOutput()->GetNumberOfCells())
  {
    return EXIT_FAILURE;
  }
  if (IcosidodecaheronGrid->GetCellType(0) != reader->GetOutput()->GetCellType(0))
  {
    return EXIT_FAILURE;
  }
  if (IcosidodecaheronGrid->GetPolyhedronFaces()->GetNumberOfCells() !=
    reader->GetOutput()->GetPolyhedronFaces()->GetNumberOfCells())
  {
    return EXIT_FAILURE;
  }
  // Comparison of faces connectivities
  const vtkIdType* nodesRef;
  vtkNew<vtkIdList> ptsIdsRef;
  const vtkIdType* nodesRead;
  vtkNew<vtkIdList> ptsIdsRead;
  for (vtkIdType idx = 0; idx < IcosidodecaheronGrid->GetPolyhedronFaces()->GetNumberOfCells();
       ++idx)
  {
    vtkIdType numPtsRef, numPtsRead;
    IcosidodecaheronGrid->GetPolyhedronFaces()->GetCellAtId(idx, numPtsRef, nodesRef, ptsIdsRef);
    reader->GetOutput()->GetPolyhedronFaces()->GetCellAtId(idx, numPtsRead, nodesRead, ptsIdsRead);
    if (numPtsRef != numPtsRead)
    {
      return EXIT_FAILURE;
    }
    for (vtkIdType pt = 0; pt < numPtsRef; ++pt)
    {
      if (nodesRead[pt] != nodesRef[pt])
      {
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
