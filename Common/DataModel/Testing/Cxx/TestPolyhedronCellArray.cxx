// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkPolyhedronCellArray in its 5 storage types: Int32, Int64, FixedSizeInt32,
// FixedSizeInt64 and Generic. What vtkCellArray does with Faces and FaceLocations is tested by the
// vtkCellArray tests.

#include "MockDataArray.h"
#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkPolyhedronCellArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
using StorageTypes = vtkPolyhedronCellArray::StorageTypes;
// The faces of every cell, every face being its point ids.
using Cells = std::vector<std::vector<std::vector<vtkIdType>>>;

void Check(bool condition, const std::string& message)
{
  if (!condition)
  {
    throw std::runtime_error(message);
  }
}

//------------------------------------------------------------------------------
// An empty vtkPolyhedronCellArray of the storage type, with the cell sizes of TestCells().
template <StorageTypes StorageType>
vtkSmartPointer<vtkPolyhedronCellArray> NewPolyhedra()
{
  vtkNew<vtkCellArray> faces;
  vtkNew<vtkCellArray> faceLocations;
  if constexpr (StorageType == StorageTypes::Int32)
  {
    faces->Use32BitStorage();
    faceLocations->Use32BitStorage();
  }
  else if constexpr (StorageType == StorageTypes::Int64)
  {
    faces->Use64BitStorage();
    faceLocations->Use64BitStorage();
  }
  else if constexpr (StorageType == StorageTypes::FixedSizeInt32)
  {
    faces->UseFixedSize32BitStorage(4);
    faceLocations->UseFixedSize32BitStorage(6);
  }
  else if constexpr (StorageType == StorageTypes::FixedSizeInt64)
  {
    faces->UseFixedSize64BitStorage(4);
    faceLocations->UseFixedSize64BitStorage(6);
  }
  else
  {
    // Arrays that vtkCellArray does not dispatch.
    for (vtkCellArray* cells : { faces.Get(), faceLocations.Get() })
    {
      vtkNew<MockDataArray<vtkTypeInt32>> offsets;
      vtkNew<MockDataArray<vtkTypeInt32>> connectivity;
      offsets->InsertNextValue(0);
      cells->SetData(offsets, connectivity);
    }
  }
  auto polyhedra = vtkSmartPointer<vtkPolyhedronCellArray>::New();
  polyhedra->SetData(faces, faceLocations);
  Check(polyhedra->GetStorageType() == StorageType, "wrong storage type of an empty array");
  return polyhedra;
}

// The face stream of a cell, without the number of faces.
std::vector<vtkIdType> FaceStream(const std::vector<std::vector<vtkIdType>>& faces)
{
  std::vector<vtkIdType> stream;
  for (const auto& face : faces)
  {
    stream.push_back(static_cast<vtkIdType>(face.size()));
    stream.insert(stream.end(), face.begin(), face.end());
  }
  return stream;
}

void InsertCells(vtkPolyhedronCellArray* polyhedra, const Cells& cells)
{
  for (const auto& faces : cells)
  {
    if (faces.empty())
    {
      polyhedra->InsertEmptyCells(1);
      continue;
    }
    const std::vector<vtkIdType> stream = FaceStream(faces);
    polyhedra->InsertNextCell(static_cast<vtkIdType>(faces.size()), stream.data());
  }
}

constexpr bool IsFixedSize(StorageTypes storageType)
{
  return storageType == StorageTypes::FixedSizeInt32 || storageType == StorageTypes::FixedSizeInt64;
}

// The cells of the explicit storage types: a tetrahedron, a cell without face, and a pyramid.
Cells ExplicitCells()
{
  return { { { 0, 1, 2 }, { 0, 3, 1 }, { 1, 3, 2 }, { 2, 3, 0 } }, {},
    { { 4, 7, 6, 5 }, { 4, 5, 8 }, { 5, 6, 8 }, { 6, 7, 8 }, { 7, 4, 8 } } };
}

// The cells of the fixed size storage types: two hexahedra, of 6 faces of 4 points.
Cells FixedSizeCells()
{
  Cells cells;
  for (const vtkIdType p : { 0, 8 })
  {
    cells.push_back({ { p + 0, p + 3, p + 2, p + 1 }, { p + 4, p + 5, p + 6, p + 7 },
      { p + 0, p + 1, p + 5, p + 4 }, { p + 1, p + 2, p + 6, p + 5 },
      { p + 2, p + 3, p + 7, p + 6 }, { p + 3, p + 0, p + 4, p + 7 } });
  }
  return cells;
}

template <StorageTypes StorageType>
Cells TestCells()
{
  return IsFixedSize(StorageType) ? FixedSizeCells() : ExplicitCells();
}

template <StorageTypes StorageType>
vtkSmartPointer<vtkPolyhedronCellArray> NewFilledPolyhedra()
{
  auto polyhedra = NewPolyhedra<StorageType>();
  InsertCells(polyhedra, TestCells<StorageType>());
  return polyhedra;
}

// Check every query of the cells of polyhedra against cells.
void CheckCells(const std::string& name, vtkPolyhedronCellArray* polyhedra, const Cells& cells)
{
  const StorageTypes facesStorage = polyhedra->GetFaces()->GetStorageType();
  Check(polyhedra->GetStorageType() ==
      (facesStorage == polyhedra->GetFaceLocations()->GetStorageType() ? facesStorage
                                                                       : StorageTypes::Generic),
    name + ": the storage type is not the one of Faces and FaceLocations");
  Check(polyhedra->GetNumberOfCells() == static_cast<vtkIdType>(cells.size()),
    name + ": wrong number of cells");
  vtkIdType numFaces = 0;
  for (const auto& faces : cells)
  {
    numFaces += static_cast<vtkIdType>(faces.size());
  }
  Check(polyhedra->GetNumberOfFaces() == numFaces, name + ": wrong number of faces");
  int maxCellNumberOfFaces = 0;
  int maxFaceSize = 0;
  for (const auto& faces : cells)
  {
    maxCellNumberOfFaces = std::max(maxCellNumberOfFaces, static_cast<int>(faces.size()));
    for (const auto& face : faces)
    {
      maxFaceSize = std::max(maxFaceSize, static_cast<int>(face.size()));
    }
  }
  Check(polyhedra->GetMaxCellNumberOfFaces() == maxCellNumberOfFaces,
    name + ": wrong GetMaxCellNumberOfFaces");
  Check(polyhedra->GetMaxFaceSize() == maxFaceSize, name + ": wrong GetMaxFaceSize");

  vtkNew<vtkIdList> tempIds;
  vtkNew<vtkIdList> pts;
  vtkNew<vtkIdList> stream;
  vtkNew<vtkCellArray> cellFaces;
  vtkNew<vtkIdList> cellFacePts;
  vtkIdType faceId = 0;
  for (vtkIdType cellId = 0; cellId < polyhedra->GetNumberOfCells(); ++cellId)
  {
    const std::string cell = name + ": cell " + vtk::to_string(cellId);
    const auto& faces = cells[cellId];
    const auto cellNumFaces = static_cast<int>(faces.size());
    Check(polyhedra->GetCellNumberOfFaces(cellId) == cellNumFaces,
      cell + ": wrong GetCellNumberOfFaces");
    polyhedra->GetCellFaces(cellId, cellFaces);
    Check(cellFaces->GetNumberOfCells() == cellNumFaces, cell + ": wrong GetCellFaces");
    vtkIdType numFacePoints = 0;
    for (int face = 0; face < cellNumFaces; ++face)
    {
      const std::string where = cell + ", face " + vtk::to_string(face);
      const auto& expected = faces[face];
      numFacePoints += static_cast<vtkIdType>(expected.size());
      Check(polyhedra->GetFaceId(cellId, face) == faceId++, where + ": wrong GetFaceId");
      Check(polyhedra->GetFaceSize(cellId, face) == static_cast<vtkIdType>(expected.size()),
        where + ": wrong GetFaceSize");
      vtkIdType npts = 0;
      const vtkIdType* ids = nullptr;
      polyhedra->GetFacePoints(cellId, face, npts, ids, tempIds);
      Check(std::vector<vtkIdType>(ids, ids + npts) == expected, where + ": wrong GetFacePoints");
      // The points are copied into tempIds only if the storage is not shareable.
      Check((ids == tempIds->GetPointer(0)) == !polyhedra->IsStorageShareable(),
        where + ": wrong sharing of the points");
      polyhedra->GetFacePoints(cellId, face, pts);
      Check(std::vector<vtkIdType>(pts->begin(), pts->end()) == expected,
        where + ": wrong GetFacePoints into a vtkIdList");
      cellFaces->GetCellAtId(face, cellFacePts);
      Check(std::vector<vtkIdType>(cellFacePts->begin(), cellFacePts->end()) == expected,
        where + ": wrong GetCellFaces");
    }
    Check(polyhedra->GetCellNumberOfFacePoints(cellId) == numFacePoints,
      cell + ": wrong GetCellNumberOfFacePoints");
    std::vector<vtkIdType> expectedStream;
    if (cellNumFaces > 0)
    {
      expectedStream = FaceStream(faces);
      expectedStream.insert(expectedStream.begin(), cellNumFaces);
    }
    polyhedra->GetFaceStream(cellId, stream);
    Check(std::vector<vtkIdType>(stream->begin(), stream->end()) == expectedStream,
      cell + ": wrong GetFaceStream");
  }
}

//------------------------------------------------------------------------------
template <StorageTypes StorageType>
void TestQueries()
{
  const auto polyhedra = NewFilledPolyhedra<StorageType>();
  Check(polyhedra->GetStorageType() == StorageType, "wrong storage type after insertion");
  Check(polyhedra->IsStorageShareable() == polyhedra->GetFaces()->IsStorageShareable(),
    "wrong IsStorageShareable");
  CheckCells("Queries", polyhedra, TestCells<StorageType>());
}

// The arrays that Dispatch() gives for each storage type.
template <StorageTypes StorageType>
struct DispatchedArrays
{
  using Offsets = vtkDataArray;
  using Connectivity = vtkDataArray;
};
template <>
struct DispatchedArrays<StorageTypes::Int32>
{
  using Offsets = vtkPolyhedronCellArray::AOSArray32;
  using Connectivity = vtkPolyhedronCellArray::AOSArray32;
};
template <>
struct DispatchedArrays<StorageTypes::Int64>
{
  using Offsets = vtkPolyhedronCellArray::AOSArray64;
  using Connectivity = vtkPolyhedronCellArray::AOSArray64;
};
template <>
struct DispatchedArrays<StorageTypes::FixedSizeInt32>
{
  using Offsets = vtkPolyhedronCellArray::AffineArray32;
  using Connectivity = vtkPolyhedronCellArray::AOSArray32;
};
template <>
struct DispatchedArrays<StorageTypes::FixedSizeInt64>
{
  using Offsets = vtkPolyhedronCellArray::AffineArray64;
  using Connectivity = vtkPolyhedronCellArray::AOSArray64;
};

template <StorageTypes StorageType>
struct CheckDispatchedArrays : public vtkPolyhedronCellArray::DispatchUtilities
{
  // Returns the number of points of the faces of the cell cellId, and whether the arrays are the
  // ones of the storage type.
  template <class FaceOffsetsT, class FaceConnectivityT, class LocationOffsetsT,
    class LocationConnectivityT>
  vtkIdType operator()(FaceOffsetsT* faceOffsets, FaceConnectivityT*,
    LocationOffsetsT* locationOffsets, LocationConnectivityT* locationConnectivity,
    vtkIdType cellId, bool& expectedArrays) const
  {
    using Expected = DispatchedArrays<StorageType>;
    expectedArrays = std::is_same_v<FaceOffsetsT, typename Expected::Offsets> &&
      std::is_same_v<FaceConnectivityT, typename Expected::Connectivity> &&
      std::is_same_v<LocationOffsetsT, typename Expected::Offsets> &&
      std::is_same_v<LocationConnectivityT, typename Expected::Connectivity>;
    vtkIdType numPoints = 0;
    for (const vtkIdType faceId : GetCellRange(locationOffsets, locationConnectivity, cellId))
    {
      numPoints += GetCellSize(faceOffsets, faceId);
    }
    return numPoints;
  }
};

template <StorageTypes StorageType>
void TestDispatch()
{
  const auto polyhedra = NewFilledPolyhedra<StorageType>();
  using Worker = CheckDispatchedArrays<StorageType>;
  for (vtkIdType cellId = 0; cellId < polyhedra->GetNumberOfCells(); ++cellId)
  {
    bool expectedArrays = false;
    const auto numPoints = polyhedra->Dispatch(Worker{}, cellId, expectedArrays);
    static_assert(std::is_same_v<decltype(numPoints), const vtkIdType>);
    Check(expectedArrays, "Dispatch: wrong arrays");
    Check(numPoints == polyhedra->GetCellNumberOfFacePoints(cellId), "Dispatch: wrong result");

    // With the storage type given.
    expectedArrays = false;
    const auto givenStorage =
      polyhedra->Dispatch(Worker{}, polyhedra->GetStorageType(), cellId, expectedArrays);
    Check(expectedArrays && givenStorage == numPoints, "Dispatch(storageType): wrong result");
  }
}

vtkIdType GetNumberOfPoints(const Cells& cells)
{
  vtkIdType numPoints = 0;
  for (const auto& faces : cells)
  {
    for (const auto& face : faces)
    {
      numPoints = std::max(numPoints, *std::max_element(face.begin(), face.end()) + 1);
    }
  }
  return numPoints;
}

// The cells with their point ids offset by numPoints.
Cells Offset(Cells cells, vtkIdType numPoints)
{
  for (auto& faces : cells)
  {
    for (auto& face : faces)
    {
      for (auto& id : face)
      {
        id += numPoints;
      }
    }
  }
  return cells;
}

template <StorageTypes StorageType>
void TestInsertion()
{
  const Cells cells = TestCells<StorageType>();

  // The cells given by a vtkCellArray of their faces keep the storage type.
  auto polyhedra = NewPolyhedra<StorageType>();
  for (const auto& faces : cells)
  {
    vtkNew<vtkCellArray> cellFaces;
    for (const auto& face : faces)
    {
      cellFaces->InsertNextCell(static_cast<vtkIdType>(face.size()), face.data());
    }
    polyhedra->InsertNextCell(cellFaces);
  }
  Check(polyhedra->GetStorageType() == StorageType, "InsertNextCell: wrong storage type");
  CheckCells("InsertNextCell(vtkCellArray*)", polyhedra, cells);

  // Cells of other numbers of faces and of points, and a cell without face, convert the fixed
  // size storage.
  const Cells other = Offset(ExplicitCells(), GetNumberOfPoints(cells));
  Cells all = cells;
  all.insert(all.end(), other.begin(), other.end());
  InsertCells(polyhedra, other);
  StorageTypes expected = StorageType;
  if constexpr (StorageType == StorageTypes::FixedSizeInt32)
  {
    expected = StorageTypes::Int32;
  }
  else if constexpr (StorageType == StorageTypes::FixedSizeInt64)
  {
    expected = StorageTypes::Int64;
  }
  Check(polyhedra->GetStorageType() == expected, "InsertNextCell: wrong converted storage type");
  CheckCells("InsertNextCell(other sizes)", polyhedra, all);
}

// The cells, followed by the cells with their point ids offset by their number of points.
Cells Twice(const Cells& cells)
{
  Cells twice = cells;
  const Cells offset = Offset(cells, GetNumberOfPoints(cells));
  twice.insert(twice.end(), offset.begin(), offset.end());
  return twice;
}

template <StorageTypes StorageType>
void TestCopies()
{
  const Cells cells = TestCells<StorageType>();
  const auto polyhedra = NewFilledPolyhedra<StorageType>();

  vtkNew<vtkPolyhedronCellArray> deep;
  deep->DeepCopy(polyhedra);
  Check(deep->GetFaces() != polyhedra->GetFaces() &&
      deep->GetFaceLocations() != polyhedra->GetFaceLocations(),
    "DeepCopy: the arrays are shared");
  Check(deep->GetStorageType() == StorageType, "DeepCopy: wrong storage type");
  CheckCells("DeepCopy", deep, cells);

  vtkNew<vtkPolyhedronCellArray> shallow;
  shallow->ShallowCopy(polyhedra);
  Check(shallow->GetFaces() == polyhedra->GetFaces() &&
      shallow->GetFaceLocations() == polyhedra->GetFaceLocations(),
    "ShallowCopy: the arrays are not shared");
  Check(shallow->GetStorageType() == StorageType, "ShallowCopy: wrong storage type");

  vtkNew<vtkPolyhedronCellArray> shared;
  shared->SetData(polyhedra->GetFaces(), polyhedra->GetFaceLocations());
  Check(shared->GetFaces() == polyhedra->GetFaces() &&
      shared->GetFaceLocations() == polyhedra->GetFaceLocations(),
    "SetData: the arrays are not shared");
  Check(shared->GetStorageType() == StorageType, "SetData: wrong storage type");
  CheckCells("SetData", shared, cells);

  // The cells appended after themselves, their point ids offset by their number of points, and
  // the ids of their faces by their number of faces.
  auto appended = NewPolyhedra<StorageType>();
  appended->Append(polyhedra);
  appended->Append(polyhedra, GetNumberOfPoints(cells));
  Check(appended->GetStorageType() == StorageType, "Append: wrong storage type");
  CheckCells("Append", appended, Twice(cells));
}

template <StorageTypes StorageType>
void TestConversions()
{
  const Cells cells = TestCells<StorageType>();

  auto polyhedra = NewFilledPolyhedra<StorageType>();
  Check(polyhedra->ConvertTo32BitStorage() && polyhedra->GetStorageType() == StorageTypes::Int32,
    "ConvertTo32BitStorage: wrong storage type");
  CheckCells("ConvertTo32BitStorage", polyhedra, cells);

  polyhedra = NewFilledPolyhedra<StorageType>();
  Check(polyhedra->ConvertTo64BitStorage() && polyhedra->GetStorageType() == StorageTypes::Int64,
    "ConvertTo64BitStorage: wrong storage type");
  CheckCells("ConvertTo64BitStorage", polyhedra, cells);

  polyhedra = NewFilledPolyhedra<StorageType>();
  const StorageTypes defaultStorage =
    vtkCellArray::GetDefaultStorageIs64Bit() ? StorageTypes::Int64 : StorageTypes::Int32;
  Check(polyhedra->ConvertToDefaultStorage() && polyhedra->GetStorageType() == defaultStorage,
    "ConvertToDefaultStorage: wrong storage type");
  CheckCells("ConvertToDefaultStorage", polyhedra, cells);

  // Only the cells of the fixed size storage types have a fixed number of faces and of points.
  polyhedra = NewFilledPolyhedra<StorageType>();
  const StorageTypes smallestStorage =
    IsFixedSize(StorageType) ? StorageTypes::FixedSizeInt32 : StorageTypes::Int32;
  Check(polyhedra->ConvertToSmallestStorage() && polyhedra->GetStorageType() == smallestStorage,
    "ConvertToSmallestStorage: wrong storage type");
  CheckCells("ConvertToSmallestStorage", polyhedra, cells);
}

template <StorageTypes StorageType>
void TestMemory()
{
  const Cells cells = TestCells<StorageType>();
  auto polyhedra = NewPolyhedra<StorageType>();
  Check(polyhedra->AllocateEstimate(2, 6, 4), "AllocateEstimate failed");
  InsertCells(polyhedra, cells);
  CheckCells("AllocateEstimate", polyhedra, cells);
  Check(polyhedra->GetActualMemorySize() > 0, "wrong GetActualMemorySize");

  polyhedra->Squeeze();
  CheckCells("Squeeze", polyhedra, cells);

  polyhedra->Reset();
  CheckCells("Reset", polyhedra, {});
  InsertCells(polyhedra, cells);
  CheckCells("Reset, then insertion", polyhedra, cells);

  const vtkMTimeType mtime = polyhedra->GetMTime();
  polyhedra->Initialize();
  Check(polyhedra->GetMTime() > mtime, "Initialize: the array should be modified");
  CheckCells("Initialize", polyhedra, {});
  InsertCells(polyhedra, cells);
  CheckCells("Initialize, then insertion", polyhedra, cells);
}

// The legacy layout of the cells, see vtkPolyhedronCellArray::ExportLegacyFormat().
void LegacyFormat(
  const Cells& cells, std::vector<vtkIdType>& faces, std::vector<vtkIdType>& faceLocations)
{
  for (const auto& cellFaces : cells)
  {
    if (cellFaces.empty())
    {
      faceLocations.push_back(-1);
      continue;
    }
    faceLocations.push_back(static_cast<vtkIdType>(faces.size()));
    faces.push_back(static_cast<vtkIdType>(cellFaces.size()));
    const std::vector<vtkIdType> stream = FaceStream(cellFaces);
    faces.insert(faces.end(), stream.begin(), stream.end());
  }
}

template <StorageTypes StorageType>
void TestLegacyFormat()
{
  const Cells cells = TestCells<StorageType>();
  std::vector<vtkIdType> expectedFaces;
  std::vector<vtkIdType> expectedLocations;
  LegacyFormat(cells, expectedFaces, expectedLocations);

  const auto polyhedra = NewFilledPolyhedra<StorageType>();
  vtkNew<vtkIdTypeArray> faces;
  vtkNew<vtkIdTypeArray> faceLocations;
  polyhedra->ExportLegacyFormat(faces, faceLocations);
  Check(std::vector<vtkIdType>(faces->Begin(), faces->End()) == expectedFaces &&
      std::vector<vtkIdType>(faceLocations->Begin(), faceLocations->End()) == expectedLocations,
    "ExportLegacyFormat: wrong layout");

  // ImportLegacyFormat replaces the cells.
  auto imported = NewFilledPolyhedra<StorageType>();
  Check(imported->ImportLegacyFormat(faces, faceLocations), "ImportLegacyFormat failed");
  CheckCells("ImportLegacyFormat", imported, cells);

  // AppendLegacyFormat keeps the cells, and offsets the point ids of the appended ones.
  auto appended = NewFilledPolyhedra<StorageType>();
  Check(appended->AppendLegacyFormat(faces, faceLocations, GetNumberOfPoints(cells)),
    "AppendLegacyFormat failed");
  CheckCells("AppendLegacyFormat", appended, Twice(cells));

  // A corrupt layout, whose last face has too many points, or whose last face location is past
  // the faces, leaves the cells unchanged.
  std::vector<vtkIdType> corruptFaces = expectedFaces;
  corruptFaces[corruptFaces.size() - 1 - cells.back().back().size()] += 1;
  std::vector<vtkIdType> corruptLocations = expectedLocations;
  corruptLocations.back() = static_cast<vtkIdType>(expectedFaces.size());
  const auto facesLength = static_cast<vtkIdType>(expectedFaces.size());
  const auto numCells = static_cast<vtkIdType>(expectedLocations.size());
  Check(!imported->ImportLegacyFormat(
          corruptFaces.data(), facesLength, expectedLocations.data(), numCells) &&
      !appended->AppendLegacyFormat(
        corruptFaces.data(), facesLength, expectedLocations.data(), numCells) &&
      !appended->AppendLegacyFormat(
        expectedFaces.data(), facesLength, corruptLocations.data(), numCells),
    "LegacyFormat: a corrupt layout should be rejected");
  CheckCells("ImportLegacyFormat(corrupt)", imported, cells);
  CheckCells("AppendLegacyFormat(corrupt)", appended, Twice(cells));
}

template <StorageTypes StorageType>
void RunTests(const char* name)
{
  std::cout << "Testing " << name << " storage\n";
  TestQueries<StorageType>();
  TestDispatch<StorageType>();
  TestInsertion<StorageType>();
  TestCopies<StorageType>();
  TestConversions<StorageType>();
  TestMemory<StorageType>();
  TestLegacyFormat<StorageType>();
}

// Faces and FaceLocations of different storage types give the Generic storage type, which
// ConvertToSmallestStorage() avoids.
void TestMixedStorage()
{
  // A tetrahedron, whose faces have a fixed size, and a cell without face.
  const Cells cells = { { { 0, 1, 2 }, { 0, 3, 1 }, { 1, 3, 2 }, { 2, 3, 0 } }, {} };
  vtkNew<vtkPolyhedronCellArray> source;
  InsertCells(source, cells);
  vtkNew<vtkCellArray> faces;
  faces->DeepCopy(source->GetFaces());
  faces->ConvertTo32BitStorage();
  vtkNew<vtkCellArray> faceLocations;
  faceLocations->DeepCopy(source->GetFaceLocations());
  faceLocations->ConvertTo64BitStorage();
  vtkNew<vtkPolyhedronCellArray> mixed;
  mixed->SetData(faces, faceLocations);
  Check(mixed->GetStorageType() == StorageTypes::Generic, "Mixed: wrong storage type");
  bool expectedArrays = false;
  mixed->Dispatch(CheckDispatchedArrays<StorageTypes::Generic>{}, 0, expectedArrays);
  Check(expectedArrays, "Mixed: wrong dispatched arrays");
  CheckCells("Mixed", mixed, cells);

  Check(mixed->ConvertToSmallestStorage() && mixed->GetStorageType() == StorageTypes::Int32,
    "Mixed: ConvertToSmallestStorage should give both arrays the same storage type");
  CheckCells("Mixed, ConvertToSmallestStorage", mixed, cells);
}
} // namespace

int TestPolyhedronCellArray(int, char*[])
{
  try
  {
    RunTests<StorageTypes::Int32>("Int32");
    RunTests<StorageTypes::Int64>("Int64");
    RunTests<StorageTypes::FixedSizeInt32>("FixedSizeInt32");
    RunTests<StorageTypes::FixedSizeInt64>("FixedSizeInt64");
    RunTests<StorageTypes::Generic>("Generic");
    TestMixedStorage();
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
