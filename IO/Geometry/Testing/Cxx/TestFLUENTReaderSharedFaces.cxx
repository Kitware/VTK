// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Reads a polyhedral mesh and checks that every polyhedron is wound outward, and that the faces
// are shared: a copy of the faces per cell gives every polyhedron the same faces and stores more.

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkFLUENTReader.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPolyhedronCellArray.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace
{
// Every polyhedron of the grid is closed and consistently wound, each edge traversed once in each
// direction, and encloses a positive volume, i.e., its faces are wound outward.
bool CheckOrientedPolyhedra(vtkUnstructuredGrid* grid, const std::string& name)
{
  vtkPolyhedronCellArray* polyhedra = grid->GetPolyhedronCellArray();
  if (!polyhedra || !polyhedra->Validate())
  {
    std::cerr << name << ": invalid polyhedron arrays" << std::endl;
    return false;
  }
  vtkNew<vtkIdList> facePoints;
  for (vtkIdType cellId = 0; cellId < grid->GetNumberOfCells(); ++cellId)
  {
    if (grid->GetCellType(cellId) != VTK_POLYHEDRON)
    {
      continue;
    }
    std::map<std::pair<vtkIdType, vtkIdType>, int> edges;
    double volume = 0.0;
    for (int face = 0; face < polyhedra->GetCellNumberOfFaces(cellId); ++face)
    {
      polyhedra->GetFacePoints(cellId, face, facePoints);
      const vtkIdType numPoints = facePoints->GetNumberOfIds();
      double p0[3], p1[3], p2[3];
      grid->GetPoint(facePoints->GetId(0), p0);
      for (vtkIdType i = 0; i < numPoints; ++i)
      {
        ++edges[{ facePoints->GetId(i), facePoints->GetId((i + 1) % numPoints) }];
      }
      for (vtkIdType i = 1; i + 1 < numPoints; ++i)
      {
        grid->GetPoint(facePoints->GetId(i), p1);
        grid->GetPoint(facePoints->GetId(i + 1), p2);
        volume += vtkMath::Determinant3x3(p0, p1, p2) / 6.0;
      }
    }
    for (const auto& edge : edges)
    {
      const auto reversed = edges.find({ edge.first.second, edge.first.first });
      if (edge.second != 1 || reversed == edges.end() || reversed->second != 1)
      {
        std::cerr << name << ": polyhedron " << cellId << " is not consistently wound" << std::endl;
        return false;
      }
    }
    if (volume <= 0.0)
    {
      std::cerr << name << ": polyhedron " << cellId << " has its faces wound inward" << std::endl;
      return false;
    }
  }
  return true;
}

// The grid read with shared faces has the cells of the one read with a copy of the faces per cell,
// each polyhedron with the same faces in the same orientation, and stores fewer faces.
bool CheckSharedFaces(vtkUnstructuredGrid* shared, vtkUnstructuredGrid* copied,
  const std::string& name, bool expectSharing = true)
{
  if (!CheckOrientedPolyhedra(shared, name + " (shared)") ||
    !CheckOrientedPolyhedra(copied, name + " (copied)"))
  {
    return false;
  }
  if (shared->GetNumberOfCells() != copied->GetNumberOfCells())
  {
    std::cerr << name << ": different numbers of cells" << std::endl;
    return false;
  }
  vtkNew<vtkIdList> sharedFaces, copiedFaces;
  for (vtkIdType cellId = 0; cellId < shared->GetNumberOfCells(); ++cellId)
  {
    if (shared->GetCellType(cellId) != copied->GetCellType(cellId))
    {
      std::cerr << name << ": cell " << cellId << " has a different type" << std::endl;
      return false;
    }
    if (shared->GetCellType(cellId) != VTK_POLYHEDRON)
    {
      continue;
    }
    shared->GetFaceStream(cellId, sharedFaces);
    copied->GetFaceStream(cellId, copiedFaces);
    if (sharedFaces->GetNumberOfIds() != copiedFaces->GetNumberOfIds() ||
      !std::equal(sharedFaces->begin(), sharedFaces->end(), copiedFaces->begin()))
    {
      std::cerr << name << ": polyhedron " << cellId << " has different faces" << std::endl;
      return false;
    }
  }
  vtkPolyhedronCellArray* sharedPolyhedra = shared->GetPolyhedronCellArray();
  vtkPolyhedronCellArray* copiedPolyhedra = copied->GetPolyhedronCellArray();
  if (copiedPolyhedra->HasSharedFaces())
  {
    std::cerr << name << ": faces are shared although sharing is off" << std::endl;
    return false;
  }
  if (expectSharing &&
    (!sharedPolyhedra->HasSharedFaces() ||
      sharedPolyhedra->GetNumberOfFaces() >= copiedPolyhedra->GetNumberOfFaces()))
  {
    std::cerr << name << ": faces are not shared (" << sharedPolyhedra->GetNumberOfFaces()
              << " stored, " << copiedPolyhedra->GetNumberOfFaces() << " without sharing)"
              << std::endl;
    return false;
  }
  return true;
}

// The unstructured grids of a reader's output, in order.
std::vector<vtkUnstructuredGrid*> Grids(vtkDataObject* output)
{
  std::vector<vtkUnstructuredGrid*> grids;
  if (auto grid = vtkUnstructuredGrid::SafeDownCast(output))
  {
    grids.push_back(grid);
  }
  else if (auto composite = vtkCompositeDataSet::SafeDownCast(output))
  {
    vtkSmartPointer<vtkCompositeDataIterator> it;
    it.TakeReference(composite->NewIterator());
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
      if (auto leaf = vtkUnstructuredGrid::SafeDownCast(it->GetCurrentDataObject()))
      {
        grids.push_back(leaf);
      }
    }
  }
  return grids;
}

// Compares the polyhedral grids of the two outputs.
bool CompareOutputs(vtkDataObject* shared, vtkDataObject* copied, const std::string& name)
{
  const auto sharedGrids = Grids(shared);
  const auto copiedGrids = Grids(copied);
  if (sharedGrids.size() != copiedGrids.size())
  {
    std::cerr << name << ": different numbers of grids" << std::endl;
    return false;
  }
  bool anySharing = false;
  bool ok = true;
  for (std::size_t i = 0; i < sharedGrids.size(); ++i)
  {
    if (!sharedGrids[i]->GetPolyhedronCellArray())
    {
      continue;
    }
    ok &= CheckSharedFaces(sharedGrids[i], copiedGrids[i], name, false);
    anySharing |= sharedGrids[i]->GetPolyhedronCellArray()->HasSharedFaces();
  }
  if (!anySharing)
  {
    std::cerr << name << ": no grid shares faces" << std::endl;
    ok = false;
  }
  return ok;
}
}

namespace
{
// Two unit boxes side by side along x. Splitting their tops into two triangles gives them seven
// faces, so they are polyhedra; the face at x = 1 is between them. Points are i + 3 j + 6 k at
// (i, j, k).
const double BoxPoints[12][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 2, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 },
  { 2, 1, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, { 2, 0, 1 }, { 0, 1, 1 }, { 1, 1, 1 }, { 2, 1, 1 } };

// The faces wound out of the first box (x < 1) for the face between the boxes and out of their
// box for the others. The face between the boxes comes first.
const std::vector<std::vector<int>> BoxFaces = { { 1, 4, 10, 7 },
  // first box: x = 0, y = 0, y = 1, bottom, top
  { 0, 6, 9, 3 }, { 0, 1, 7, 6 }, { 3, 9, 10, 4 }, { 0, 3, 4, 1 }, { 6, 7, 10 }, { 6, 10, 9 },
  // second box: x = 2, y = 0, y = 1, bottom, top
  { 2, 5, 11, 8 }, { 1, 2, 8, 7 }, { 4, 10, 11, 5 }, { 1, 4, 5, 2 }, { 7, 8, 11 }, { 7, 11, 10 } };

// The box each face belongs to, and the other box for the face between them.
const int BoxFaceCell[13] = { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 };

// Writes the two boxes as an ASCII Fluent mesh. A Fluent face points into its cell c0: the face
// between the boxes, wound into the second box, has it as c0, and the boundary faces, wound out of
// their box, are written reversed.
std::string WriteMesh(const std::string& dir)
{
  vtksys::SystemTools::MakeDirectory(dir);
  const std::string fileName = dir + "/boxes.msh";
  std::ofstream out(fileName);
  out << std::hex;
  out << "(0 \"two polyhedral boxes\")\n(2 3)\n";
  out << "(10 (0 1 c 0 3))\n(10 (1 1 c 1 3)(\n";
  out << std::dec;
  for (const auto& p : BoxPoints)
  {
    out << p[0] << " " << p[1] << " " << p[2] << "\n";
  }
  out << std::hex;
  out << "))\n";
  out << "(12 (0 1 2 0))\n(12 (2 1 2 1 7))\n";
  out << "(13 (0 1 d 0))\n";
  auto writeFace = [&out](const std::vector<int>& face, bool reversed, int c0, int c1)
  {
    out << face.size();
    for (std::size_t i = 0; i < face.size(); ++i)
    {
      out << " " << (face[reversed ? face.size() - 1 - i : i] + 1);
    }
    out << " " << c0 << " " << c1 << "\n";
  };
  out << "(13 (3 1 1 2 0)(\n";
  writeFace(BoxFaces[0], false, 2, 1);
  out << "))\n";
  out << "(13 (4 2 d 3 0)(\n";
  for (std::size_t f = 1; f < BoxFaces.size(); ++f)
  {
    writeFace(BoxFaces[f], true, BoxFaceCell[f] + 1, 0);
  }
  out << "))\n";
  out << "(45 (2 fluid fluid)())\n(45 (3 interior default-interior)())\n(45 (4 wall walls)())\n";
  return fileName;
}

vtkSmartPointer<vtkDataObject> Read(const std::string& fileName, bool share)
{
  vtkNew<vtkFLUENTReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkDataObject* output = reader->GetOutputDataObject(0);
  if (!share)
  {
    for (vtkUnstructuredGrid* grid : Grids(output))
    {
      if (vtkPolyhedronCellArray* polyhedra = grid->GetPolyhedronCellArray())
      {
        polyhedra->DuplicateSharedFaces();
      }
    }
  }
  return output;
}
}

int TestFLUENTReaderSharedFaces(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  const std::string dir = std::string(tempDir) + "/TestFLUENTReaderSharedFaces";
  delete[] tempDir;
  const std::string mesh = WriteMesh(dir);

  auto shared = Read(mesh, true);
  auto copied = Read(mesh, false);
  bool ok = CompareOutputs(shared, copied, mesh);

  // The face between the two polyhedra is stored once.
  const auto grids = Grids(shared);
  vtkPolyhedronCellArray* polyhedra = grids.empty() ? nullptr : grids[0]->GetPolyhedronCellArray();
  if (!polyhedra || grids[0]->GetNumberOfCells() != 2 || polyhedra->GetNumberOfFaces() != 13)
  {
    std::cerr << "expected the fluid zone to have 2 polyhedra and 13 stored faces" << std::endl;
    ok = false;
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
