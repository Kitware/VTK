// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkCellArray.h>
#include <vtkExtractEdges.h>
#include <vtkIdTypeArray.h>
#include <vtkNew.h>
#include <vtkPolyhedron.h>
#include <vtkSmartPointer.h>

namespace
{
//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyhedron> MakeDodecahedron()
{
  vtkSmartPointer<vtkPolyhedron> aDodecahedron = vtkSmartPointer<vtkPolyhedron>::New();

  for (int i = 0; i < 20; ++i)
  {
    aDodecahedron->GetPointIds()->InsertNextId(i);
  }

  aDodecahedron->GetPoints()->InsertNextPoint(1.21412, 0, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.375185, 1.1547, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.982247, 0.713644, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.982247, -0.713644, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.375185, -1.1547, 1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(1.96449, 0, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.607062, 1.86835, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.58931, 1.1547, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.58931, -1.1547, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.607062, -1.86835, 0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(1.58931, 1.1547, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.607062, 1.86835, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.96449, 0, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.607062, -1.86835, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(1.58931, -1.1547, -0.375185);
  aDodecahedron->GetPoints()->InsertNextPoint(0.982247, 0.713644, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.375185, 1.1547, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-1.21412, 0, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(-0.375185, -1.1547, -1.58931);
  aDodecahedron->GetPoints()->InsertNextPoint(0.982247, -0.713644, -1.58931);

  vtkIdType face_offsets[13] = { 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60 };
  vtkIdType face_conns[60] = {
    0, 1, 2, 3, 4,     // ids
    0, 5, 10, 6, 1,    //
    1, 6, 11, 7, 2,    //
    2, 7, 12, 8, 3,    //
    3, 8, 13, 9, 4,    //
    4, 9, 14, 5, 0,    //
    15, 10, 5, 14, 19, //
    16, 11, 6, 10, 15, //
    17, 12, 7, 11, 16, //
    18, 13, 8, 12, 17, //
    19, 14, 9, 13, 18, //
    19, 18, 17, 16, 15 //
  };
  vtkNew<vtkCellArray> faces;
  vtkNew<vtkIdTypeArray> offsets_arr;
  vtkNew<vtkIdTypeArray> conns_arr;
  offsets_arr->SetArray(face_offsets, 13, 1);
  conns_arr->SetArray(face_conns, 60, 1);
  faces->SetData(offsets_arr, conns_arr);
  aDodecahedron->SetCellFaces(faces);
  aDodecahedron->Initialize();

  return aDodecahedron;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyhedron> MakeConcavePolyhedron()
{
  vtkSmartPointer<vtkPolyhedron> aPolyhedron = vtkSmartPointer<vtkPolyhedron>::New();

  for (int i = 0; i < 20; ++i)
  {
    aPolyhedron->GetPointIds()->InsertNextId(i);
  }

  aPolyhedron->GetPoints()->InsertNextPoint(0.0, 0.0, 0.0);
  aPolyhedron->GetPoints()->InsertNextPoint(2.0, 0.0, 0.0);
  aPolyhedron->GetPoints()->InsertNextPoint(2.0, 2.0, 0.0);
  aPolyhedron->GetPoints()->InsertNextPoint(0.0, 2.0, 0.0);
  aPolyhedron->GetPoints()->InsertNextPoint(1.0, 1.0, 0.0);

  aPolyhedron->GetPoints()->InsertNextPoint(0.0, 0.0, 2.0);
  aPolyhedron->GetPoints()->InsertNextPoint(2.0, 0.0, 2.0);
  aPolyhedron->GetPoints()->InsertNextPoint(2.0, 2.0, 2.0);
  aPolyhedron->GetPoints()->InsertNextPoint(0.0, 2.0, 2.0);
  aPolyhedron->GetPoints()->InsertNextPoint(1.0, 1.0, 2.0);

  vtkIdType face_offsets[9] = { 0, 5, 10, 14, 18, 22, 26, 29, 32 };
  vtkIdType face_conns[32] = { 0, 1, 2, 3, 4, // poly
    5, 6, 7, 8, 9,                            // poly
    0, 4, 9, 5,                               // quad
    4, 3, 8, 9,                               // quad
    3, 2, 7, 8,                               // quad
    5, 6, 1, 0,                               // quad
    6, 7, 1,                                  // tri
    7, 2, 1 };                                // tri

  vtkNew<vtkCellArray> faces;
  vtkNew<vtkIdTypeArray> offsets_arr;
  vtkNew<vtkIdTypeArray> conns_arr;
  offsets_arr->SetArray(face_offsets, 9, 1);
  conns_arr->SetArray(face_conns, 32, 1);
  faces->SetData(offsets_arr, conns_arr);
  aPolyhedron->SetCellFaces(faces);
  aPolyhedron->Initialize();

  return aPolyhedron;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyhedron> MakeConcaveNonPlanarPolyhedron()
{
  vtkSmartPointer<vtkPolyhedron> aPolyhedron = vtkSmartPointer<vtkPolyhedron>::New();

  for (int i = 0; i < 20; ++i)
  {
    aPolyhedron->GetPointIds()->InsertNextId(i);
  }

  aPolyhedron->GetPoints()->InsertNextPoint(0.0, 0.0, 0.0);
  aPolyhedron->GetPoints()->InsertNextPoint(2.0, 0.0, 0.0);
  aPolyhedron->GetPoints()->InsertNextPoint(2.0, 2.0, 0.0);
  aPolyhedron->GetPoints()->InsertNextPoint(0.0, 2.0, 0.0);
  aPolyhedron->GetPoints()->InsertNextPoint(1.0, 1.0, 0.0);

  aPolyhedron->GetPoints()->InsertNextPoint(0.0, 0.0, 2.0);
  aPolyhedron->GetPoints()->InsertNextPoint(2.0, 0.0, 2.0);
  aPolyhedron->GetPoints()->InsertNextPoint(2.0, 2.0, 2.0);
  aPolyhedron->GetPoints()->InsertNextPoint(0.0, 2.0, 2.0);
  aPolyhedron->GetPoints()->InsertNextPoint(1.0, 1.0, 2.0);

  vtkIdType face_offsets[8] = { 0, 5, 10, 16, 20, 24, 27, 30 };
  vtkIdType face_conns[30] = { 0, 1, 2, 3, 4, // planar poly
    5, 6, 7, 8, 9,                            // planar poly
    0, 4, 3, 8, 9, 5,                         // non-planar poly
    3, 2, 7, 8,                               // quad
    5, 6, 1, 0,                               // quad
    6, 7, 1,                                  // tri
    7, 2, 1 };                                // tri

  vtkNew<vtkCellArray> faces;
  vtkNew<vtkIdTypeArray> offsets_arr;
  vtkNew<vtkIdTypeArray> conns_arr;
  offsets_arr->SetArray(face_offsets, 8, 1);
  conns_arr->SetArray(face_conns, 30, 1);
  faces->SetData(offsets_arr, conns_arr);
  aPolyhedron->SetCellFaces(faces);
  aPolyhedron->Initialize();

  return aPolyhedron;
}

//------------------------------------------------------------------------------
bool TestPolyhedron(vtkPolyhedron* poly, int expectedNbOfFaces, int expectedNbOfEdges)
{
  vtkNew<vtkCellArray> newFaces;
  poly->TriangulateFaces(newFaces);
  poly->SetCellFaces(newFaces);
  poly->Initialize();

  if (poly->GetNumberOfFaces() != expectedNbOfFaces)
  {
    vtkGenericWarningMacro(
      "Expected " << expectedNbOfFaces << " faces, got " << poly->GetNumberOfFaces());
    return false;
  }

  vtkNew<vtkExtractEdges> extractEdges;
  extractEdges->SetInputData(poly->GetPolyData());
  extractEdges->Update();
  vtkPolyData* output = extractEdges->GetOutput();

  if (!output || output->GetNumberOfLines() != expectedNbOfEdges)
  {
    vtkGenericWarningMacro(
      "Expected " << expectedNbOfEdges << " edges, got " << poly->GetNumberOfEdges());
    return false;
  }

  return true;
}
}

//------------------------------------------------------------------------------
int TestPolyhedronTriangulateFaces(int, char*[])
{
  // Dodecahedron
  auto dodecahedron = MakeDodecahedron();
  int expectedNbOfFaces = 12 * 3;      // pentagon == 3 triangles
  int expectedNbOfEdges = (12 * 5) / 2 // number of original edges
    + 12 * 2;                          // number of added edges

  if (!TestPolyhedron(dodecahedron, expectedNbOfFaces, expectedNbOfEdges))
  {
    return EXIT_FAILURE;
  }

  // Poly with planar faces
  auto poly1 = MakeConcavePolyhedron();
  expectedNbOfFaces = 2 * 3                       // pentagons (3 triangles)
    + 4 * 2                                       // quads (2 triangles)
    + 2;                                          // triangles
  expectedNbOfEdges = (2 * 5 + 4 * 4 + 2 * 3) / 2 // number of original edges
    + (2 * 2 + 4);                                // number of added edges

  if (!TestPolyhedron(poly1, expectedNbOfFaces, expectedNbOfEdges))
  {
    return EXIT_FAILURE;
  }

  // Poly with one non-planar face
  // Results should be identical to previous ones
  auto poly2 = MakeConcaveNonPlanarPolyhedron();

  if (!TestPolyhedron(poly2, expectedNbOfFaces, expectedNbOfEdges))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
