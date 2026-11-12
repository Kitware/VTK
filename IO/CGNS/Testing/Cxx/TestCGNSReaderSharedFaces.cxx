// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Reads a polyhedral mesh and checks that every polyhedron is wound outward, and that the faces
// are shared: a copy of the faces per cell gives every polyhedron the same faces and stores more.

#include "vtkCGNSReader.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPolyhedronCellArray.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <cstdlib>
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
vtkSmartPointer<vtkDataObject> Read(const std::string& fileName, bool share)
{
  vtkNew<vtkCGNSReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->UpdateInformation();
  reader->EnableAllBases();
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

int TestCGNSReaderSharedFaces(int argc, char* argv[])
{
  bool ok = true;
  // NFACE_n given in the file, and NFACE_n rebuilt from NGON_n ParentElements
  for (const char* file : { "Data/Example_nface_n.cgns", "Data/Example_ngon_pe.cgns" })
  {
    char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, file);
    const std::string name = fileName;
    delete[] fileName;
    ok &= CompareOutputs(Read(name, true), Read(name, false), name);
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
