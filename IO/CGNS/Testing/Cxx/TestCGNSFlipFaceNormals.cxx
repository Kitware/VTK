// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCGNSReader.h"
#include "vtkCell.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPolygon.h"
#include "vtkPolyhedron.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"

#include <iostream>

#define vtk_assert(x)                                                                              \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      std::cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << std::endl;   \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int TestCGNSFlipFaceNormals(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Example_nface_n.cgns");
  std::string nfacen = fname ? fname : "";
  delete[] fname;

  std::cout << "Opening " << nfacen << std::endl;
  vtkNew<vtkCGNSReader> nfacenReader;
  nfacenReader->SetFileName(nfacen.c_str());
  nfacenReader->SetFlipFaceNormals(false);
  nfacenReader->Update();
  vtkMultiBlockDataSet* mb = nfacenReader->GetOutput();

  vtkMultiBlockDataSet* mb2 = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(0));
  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(mb2->GetBlock(0));
  vtkCell* cell = ug->GetCell(0);
  vtk_assert(cell->GetCellType() == VTK_POLYHEDRON);

  vtkPolyhedron* nface = vtkPolyhedron::SafeDownCast(cell);
  vtkPolygon* ngon = vtkPolygon::SafeDownCast(nface->GetFace(0));
  vtkVector3d normFirst;
  // The polygon returned by vtkPolyhedron::GetFace stores its face vertices
  // locally in face-traversal order (0..N-1), while GetPointIds() holds the
  // original GLOBAL point ids from the parent unstructured grid. Use the
  // two-argument overload, which uses the documented natural-order indexing.
  vtkPolygon::ComputeNormal(ngon->GetPoints(), normFirst.GetData());

  nfacenReader->SetFlipFaceNormals(true);
  nfacenReader->Update();
  mb = nfacenReader->GetOutput();

  mb2 = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(0));
  ug = vtkUnstructuredGrid::SafeDownCast(mb2->GetBlock(0));
  cell = ug->GetCell(0);
  vtk_assert(cell->GetCellType() == VTK_POLYHEDRON);

  nface = vtkPolyhedron::SafeDownCast(cell);
  ngon = vtkPolygon::SafeDownCast(nface->GetFace(0));
  vtkVector3d normSecond;
  vtkPolygon::ComputeNormal(ngon->GetPoints(), normSecond.GetData());

  // Test is Normals are oriented in the opposite direction with FlipFaceNormals activated.
  if (normSecond.Dot(normFirst) > 0.0)
  {
    return EXIT_FAILURE;
  }

  std::cout << __FILE__ << " tests passed." << std::endl;
  return EXIT_SUCCESS;
}
