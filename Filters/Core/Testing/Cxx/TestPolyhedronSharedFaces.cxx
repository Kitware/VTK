// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// The deprecated GetPolyhedronFaces() and GetPolyhedronFaceLocations() are tested too.
#define VTK_DEPRECATION_LEVEL 0

// A grid of polyhedra whose shared faces are stored once, the second cell of a face listing it as
// -(n+1) to traverse it in reverse, gives the filters the results of the same grid with every
// face stored per cell. Stored once without the sign, a shared face is wound inward for one of
// its cells; the filters that rely on the winding then differ, and vtkAppendFilter must not
// overflow on face locations that outnumber the faces.

#include "vtkAppendFilter.h"
#include "vtkCellArray.h"
#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkExtractCells.h"
#include "vtkGenericCell.h"
#include "vtkGeometryFilter.h"
#include "vtkIdList.h"
#include "vtkMassProperties.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyhedron.h"
#include "vtkPolyhedronCellArray.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCleanUnstructuredGrid.h"
#include "vtkStringFormatter.h"
#include "vtkTableBasedClipDataSet.h"
#include "vtkTriangleFilter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace
{
int Failures = 0;

void Check(bool condition, const std::string& message)
{
  if (!condition)
  {
    std::cerr << message << std::endl;
    ++Failures;
  }
}

void CheckClose(double value, double expected, const std::string& message)
{
  Check(vtkMathUtilities::FuzzyCompare(value, expected, 1e-9 * std::max(1.0, std::abs(expected))),
    message + ": " + vtk::to_string(value) + " instead of " + vtk::to_string(expected));
}

// n^3 hexahedra as polyhedra filling the unit cube, each with its own copy of its faces, on
// unevenly spaced planes so that the faces are planar and the cells unlike.
vtkSmartPointer<vtkUnstructuredGrid> MakeDuplicated(int n)
{
  auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  vtkNew<vtkPoints> points;
  auto index = [n](int i, int j, int k) { return i + (n + 1) * (j + (n + 1) * k); };
  auto plane = [n](int i, double phase)
  { return (i + (i > 0 && i < n ? 0.2 * std::sin(phase + 2.0 * i) : 0.0)) / n; };
  for (int k = 0; k <= n; ++k)
  {
    for (int j = 0; j <= n; ++j)
    {
      for (int i = 0; i <= n; ++i)
      {
        points->InsertNextPoint(plane(i, 1.0), plane(j, 2.0), plane(k, 3.0));
      }
    }
  }
  grid->SetPoints(points);
  const int faces[6][4] = { { 0, 4, 7, 3 }, { 1, 2, 6, 5 }, { 0, 1, 5, 4 }, { 3, 7, 6, 2 },
    { 0, 3, 2, 1 }, { 4, 5, 6, 7 } };
  for (int k = 0; k < n; ++k)
  {
    for (int j = 0; j < n; ++j)
    {
      for (int i = 0; i < n; ++i)
      {
        const vtkIdType c[8] = { index(i, j, k), index(i + 1, j, k), index(i + 1, j + 1, k),
          index(i, j + 1, k), index(i, j, k + 1), index(i + 1, j, k + 1),
          index(i + 1, j + 1, k + 1), index(i, j + 1, k + 1) };
        vtkNew<vtkIdList> stream;
        stream->InsertNextId(6);
        for (const auto& face : faces)
        {
          stream->InsertNextId(4);
          for (int v : face)
          {
            stream->InsertNextId(c[v]);
          }
        }
        grid->InsertNextCell(VTK_POLYHEDRON, stream);
      }
    }
  }
  return grid;
}

// The grid with every shared face stored once, as the first cell to list it winds it. A later
// cell lists it as -(n+1) when signed, as n otherwise. Faces match by their points. With
// `partly`, every other shared face keeps a copy per cell, so that both layouts are mixed.
vtkSmartPointer<vtkUnstructuredGrid> MakeShared(
  vtkUnstructuredGrid* duplicated, bool signedIds, bool partly = false)
{
  vtkPolyhedronCellArray* polyhedra = duplicated->GetPolyhedronCellArray();
  std::map<std::vector<vtkIdType>, vtkIdType> storedFaces;
  vtkNew<vtkCellArray> faces;
  vtkNew<vtkCellArray> locations;
  vtkNew<vtkIdList> pts;
  vtkIdType numSharedFaces = 0;
  for (vtkIdType cellId = 0; cellId < duplicated->GetNumberOfCells(); ++cellId)
  {
    const int numFaces = polyhedra->GetCellNumberOfFaces(cellId);
    locations->InsertNextCell(numFaces);
    for (int face = 0; face < numFaces; ++face)
    {
      polyhedra->GetFacePoints(cellId, face, pts);
      std::vector<vtkIdType> key(pts->begin(), pts->end());
      std::sort(key.begin(), key.end());
      auto found = storedFaces.find(key);
      if (found == storedFaces.end())
      {
        const vtkIdType faceId = faces->InsertNextCell(pts);
        storedFaces.emplace(key, faceId);
        locations->InsertCellPoint(faceId);
      }
      else if (partly && numSharedFaces++ % 2 == 1)
      {
        locations->InsertCellPoint(faces->InsertNextCell(pts));
      }
      else
      {
        locations->InsertCellPoint(vtkPolyhedronCellArray::EncodeFaceId(found->second, signedIds));
      }
    }
  }
  auto shared = vtkSmartPointer<vtkUnstructuredGrid>::New();
  shared->SetPoints(duplicated->GetPoints());
  vtkNew<vtkUnsignedCharArray> types;
  types->DeepCopy(duplicated->GetCellTypes());
  shared->SetPolyhedralCells(types, duplicated->GetCells(), locations, faces);
  return shared;
}

double Volume(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkGenericCell> cell;
  double volume = 0.0;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); ++cellId)
  {
    grid->GetCell(cellId, cell);
    volume += vtkPolyhedron::SafeDownCast(cell->GetRepresentativeCell())->ComputeVolume();
  }
  return volume;
}

double Area(vtkPolyData* surface)
{
  vtkNew<vtkTriangleFilter> triangles;
  triangles->SetInputData(surface);
  vtkNew<vtkMassProperties> mass;
  mass->SetInputConnection(triangles->GetOutputPort());
  mass->Update();
  return mass->GetSurfaceArea();
}

double ContourArea(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkUnstructuredGrid> input;
  input->ShallowCopy(grid);
  vtkNew<vtkDoubleArray> scalars;
  scalars->SetName("f");
  for (vtkIdType p = 0; p < input->GetNumberOfPoints(); ++p)
  {
    double x[3];
    input->GetPoint(p, x);
    scalars->InsertNextValue(std::sin(3.0 * x[0]) + x[1] * x[1] - 0.5 * x[2]);
  }
  input->GetPointData()->SetScalars(scalars);
  vtkNew<vtkContourFilter> contour;
  contour->SetInputData(input);
  contour->SetValue(0, 0.6);
  contour->Update();
  return Area(contour->GetOutput());
}

double CutArea(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0.37, 0.41, 0.43);
  plane->SetNormal(1, 2, 3);
  vtkNew<vtkCutter> cutter;
  cutter->SetInputData(grid);
  cutter->SetCutFunction(plane);
  cutter->Update();
  return Area(cutter->GetOutput());
}

double ExtractedVolume(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkIdList> cellIds;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); cellId += 2)
  {
    cellIds->InsertNextId(cellId);
  }
  vtkNew<vtkExtractCells> extract;
  extract->SetInputData(grid);
  extract->SetCellList(cellIds);
  extract->Update();
  return Volume(extract->GetOutput());
}

vtkSmartPointer<vtkUnstructuredGrid> AppendTwice(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkAppendFilter> append;
  append->AddInputData(grid);
  append->AddInputData(grid);
  append->Update();
  return append->GetOutput();
}

double CleanedVolume(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkStaticCleanUnstructuredGrid> clean;
  clean->SetInputData(grid);
  clean->Update();
  return Volume(clean->GetOutput());
}

// The volume the surface encloses, from the winding of its polygons, and its area.
std::pair<double, double> UpperHalfSurface(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkGeometryFilter> geometry;
  geometry->SetInputData(grid);
  geometry->CellClippingOn();
  geometry->SetCellMinimum(grid->GetNumberOfCells() / 2);
  geometry->SetCellMaximum(grid->GetNumberOfCells() - 1);
  vtkNew<vtkTriangleFilter> triangles;
  triangles->SetInputConnection(geometry->GetOutputPort());
  triangles->Update();
  vtkPolyData* surface = triangles->GetOutput();
  double volume = 0.0;
  vtkIdType npts;
  const vtkIdType* pts;
  vtkNew<vtkIdList> tempIds;
  vtkCellArray* polys = surface->GetPolys();
  for (vtkIdType cellId = 0; cellId < polys->GetNumberOfCells(); ++cellId)
  {
    polys->GetCellAtId(cellId, npts, pts, tempIds);
    double x[3][3];
    for (int i = 0; i < 3; ++i)
    {
      surface->GetPoint(pts[i], x[i]);
    }
    volume += (x[0][0] * (x[1][1] * x[2][2] - x[1][2] * x[2][1]) -
                x[0][1] * (x[1][0] * x[2][2] - x[1][2] * x[2][0]) +
                x[0][2] * (x[1][0] * x[2][1] - x[1][1] * x[2][0])) /
      6.0;
  }
  return { volume, Area(surface) };
}

double ClippedVolume(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0.37, 0.41, 0.43);
  plane->SetNormal(1, 2, 3);
  vtkNew<vtkTableBasedClipDataSet> clip;
  clip->SetInputData(grid);
  clip->SetClipFunction(plane);
  clip->Update();
  return Volume(vtkUnstructuredGrid::SafeDownCast(clip->GetOutput()));
}

// The faces of every cell, as point lists, from its face stream.
std::vector<std::vector<vtkIdType>> FaceStreams(vtkUnstructuredGrid* grid)
{
  std::vector<std::vector<vtkIdType>> streams;
  vtkNew<vtkIdList> stream;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); ++cellId)
  {
    grid->GetPolyhedronCellArray()->GetFaceStream(cellId, stream);
    streams.emplace_back(stream->begin(), stream->end());
  }
  return streams;
}

// The deprecated accessors give each cell its own faces, with positive ids, wound as the signed
// grid traverses them.
void CheckPerCellAccessors(vtkUnstructuredGrid* signedShared)
{
  vtkCellArray* locations = signedShared->GetPolyhedronFaceLocations();
  vtkCellArray* faces = signedShared->GetPolyhedronFaces();
  vtkDataArray* ids = locations->GetConnectivityArray();
  bool positive = true;
  for (vtkIdType i = 0; i < ids->GetNumberOfValues(); ++i)
  {
    positive &= ids->GetComponent(i, 0) >= 0;
  }
  Check(positive, "per-cell accessors: every face id should be positive");
  Check(faces->GetNumberOfCells() == locations->GetNumberOfConnectivityIds(),
    "per-cell accessors: every cell should own its faces");
  Check(signedShared->GetPolyhedronFaces() == faces,
    "per-cell accessors: the copy should be kept until the polyhedra change");

  vtkNew<vtkUnstructuredGrid> perCell;
  perCell->SetPoints(signedShared->GetPoints());
  vtkNew<vtkUnsignedCharArray> types;
  types->DeepCopy(signedShared->GetCellTypes());
  perCell->SetPolyhedralCells(types, signedShared->GetCells(), locations, faces);
  Check(FaceStreams(perCell) == FaceStreams(signedShared),
    "per-cell accessors: the cells should have the faces of the signed grid");
}

// The XML format stores the shared faces and their signed ids as they are.
vtkSmartPointer<vtkUnstructuredGrid> XMLRoundTrip(vtkUnstructuredGrid* grid)
{
  vtkNew<vtkXMLUnstructuredGridWriter> writer;
  writer->SetInputData(grid);
  writer->WriteToOutputStringOn();
  writer->Write();
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->ReadFromInputStringOn();
  reader->SetInputString(writer->GetOutputString());
  reader->Update();
  return reader->GetOutput();
}
} // anonymous namespace

int TestPolyhedronSharedFaces(int, char*[])
{
  auto duplicated = MakeDuplicated(4);
  auto shared = MakeShared(duplicated, false);
  auto signedShared = MakeShared(duplicated, true);
  auto partlyShared = MakeShared(duplicated, true, true);

  // 4^3 cells: 3 * 4 * 4 * 5 faces, 3 * 4 * 4 * 3 of them shared.
  const vtkIdType numSides = 240;
  const vtkIdType numShared = 144;
  Check(shared->GetPolyhedronCellArray()->GetNumberOfFaces() == numSides &&
      signedShared->GetPolyhedronCellArray()->GetNumberOfFaces() == numSides,
    "every shared face should be stored once");
  Check(partlyShared->GetPolyhedronCellArray()->GetNumberOfFaces() == numSides + numShared / 2,
    "half of the shared faces should be stored once");

  const double volume = Volume(duplicated);
  const double contourArea = ContourArea(duplicated);
  const double cutArea = CutArea(duplicated);
  const double extracted = ExtractedVolume(duplicated);
  const double clipped = ClippedVolume(duplicated);
  const auto surface = UpperHalfSurface(duplicated);
  CheckClose(volume, 1.0, "the polyhedra should fill the unit cube");
  Check(clipped > 0.1 && clipped < 0.9, "the clip should keep part of the cube");

  CheckClose(Volume(signedShared), volume, "signed: volume");
  CheckClose(ContourArea(signedShared), contourArea, "signed: contour area");
  CheckClose(CutArea(signedShared), cutArea, "signed: cut area");
  CheckClose(ExtractedVolume(signedShared), extracted, "signed: extracted volume");
  CheckClose(CleanedVolume(signedShared), volume, "signed: cleaned volume");
  CheckClose(ClippedVolume(signedShared), clipped, "signed: clipped volume");
  // The upper half of the cells exposes the faces they list reversed, which must face outward.
  const auto signedSurface = UpperHalfSurface(signedShared);
  CheckClose(signedSurface.first, surface.first, "signed: volume enclosed by the surface");
  CheckClose(signedSurface.second, surface.second, "signed: surface area");
  auto signedTwice = AppendTwice(signedShared);
  CheckClose(Volume(signedTwice), 2.0 * volume, "signed: appended volume");
  CheckClose(ContourArea(signedTwice), 2.0 * contourArea, "signed: appended contour area");

  // Without the sign, a shared face is inward for one of its cells, so the divergence theorem
  // volume and the contour both come out wrong.
  Check(std::abs(Volume(shared) - volume) > 1e-3, "unsigned: the volume was expected to differ");
  Check(std::abs(ContourArea(shared) - contourArea) > 1e-3,
    "unsigned: the contour was expected to differ");
  Check(std::abs(UpperHalfSurface(shared).first - surface.first) > 1e-3,
    "unsigned: the surface was expected to face inward where it is exposed");
  auto sharedTwice = AppendTwice(shared);
  Check(sharedTwice->GetNumberOfCells() == 2 * shared->GetNumberOfCells(),
    "unsigned: wrong number of appended cells");
  Check(sharedTwice->GetPolyhedronCellArray()->GetNumberOfFaces() == 2 * numSides,
    "unsigned: the appended faces should stay shared");

  // Faces stored once and faces stored per cell can be mixed in the same grid.
  Check(partlyShared->GetPolyhedronCellArray()->Validate(), "partly: invalid arrays");
  CheckClose(Volume(partlyShared), volume, "partly: volume");
  CheckClose(ContourArea(partlyShared), contourArea, "partly: contour area");
  CheckClose(CutArea(partlyShared), cutArea, "partly: cut area");
  CheckClose(ExtractedVolume(partlyShared), extracted, "partly: extracted volume");
  CheckClose(CleanedVolume(partlyShared), volume, "partly: cleaned volume");
  CheckClose(ClippedVolume(partlyShared), clipped, "partly: clipped volume");
  const auto partlySurface = UpperHalfSurface(partlyShared);
  CheckClose(partlySurface.first, surface.first, "partly: volume enclosed by the surface");
  CheckClose(partlySurface.second, surface.second, "partly: surface area");
  CheckClose(Volume(AppendTwice(partlyShared)), 2.0 * volume, "partly: appended volume");
  auto partlyReread = XMLRoundTrip(partlyShared);
  Check(FaceStreams(partlyReread) == FaceStreams(partlyShared), "partly: XML lost the faces");

  CheckPerCellAccessors(signedShared);
  CheckPerCellAccessors(partlyShared);
  auto reread = XMLRoundTrip(signedShared);
  Check(reread->GetPolyhedronCellArray()->GetNumberOfFaces() ==
      signedShared->GetPolyhedronCellArray()->GetNumberOfFaces(),
    "XML: the shared faces should be written once");
  Check(FaceStreams(reread) == FaceStreams(signedShared), "XML: the cells lost their faces");
  CheckClose(ContourArea(reread), contourArea, "XML: contour area");

  if (Failures > 0)
  {
    std::cerr << Failures << " failure(s)" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
