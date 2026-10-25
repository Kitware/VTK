// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyhedronCellArray.h"

#include "vtkDataArrayAccessor.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPolyhedronCellArray);

namespace
{
//----------------------------------------------------------------------------
// The number of points of the faces of a cell, a point of several faces being counted several
// times.
struct GetCellNumberOfFacePointsImpl : public vtkPolyhedronCellArray::DispatchUtilities
{
  template <class FaceOffsetsT, class FaceConnectivityT, class LocationOffsetsT,
    class LocationConnectivityT>
  vtkIdType operator()(FaceOffsetsT* faceOffsets, FaceConnectivityT*,
    LocationOffsetsT* locationOffsets, LocationConnectivityT* locationConnectivity,
    vtkIdType cellId) const
  {
    const auto faceIds = GetCellRange(locationOffsets, locationConnectivity, cellId);
    vtkIdType numPoints = 0;
    for (const vtkIdType faceId : faceIds)
    {
      numPoints += GetCellSize(faceOffsets, faceId);
    }
    return numPoints;
  }
};

//----------------------------------------------------------------------------
struct GetFaceStreamImpl : public vtkPolyhedronCellArray::DispatchUtilities
{
  template <class FaceOffsetsT, class FaceConnectivityT, class LocationOffsetsT,
    class LocationConnectivityT>
  void operator()(FaceOffsetsT* faceOffsets, FaceConnectivityT* faceConnectivity,
    LocationOffsetsT* locationOffsets, LocationConnectivityT* locationConnectivity,
    vtkIdType cellId, vtkIdList* faceStream) const
  {
    const auto faceIds = GetCellRange(locationOffsets, locationConnectivity, cellId);
    const vtkIdType numFaces = faceIds.size();
    if (numFaces == 0)
    {
      return;
    }
    vtkIdType size = 1;
    for (const vtkIdType faceId : faceIds)
    {
      size += 1 + GetCellSize(faceOffsets, faceId);
    }
    faceStream->SetNumberOfIds(size);
    vtkIdType* stream = faceStream->GetPointer(0);
    *stream++ = numFaces;
    for (const vtkIdType faceId : faceIds)
    {
      const auto points = GetCellRange(faceOffsets, faceConnectivity, faceId);
      *stream++ = points.size();
      stream = std::copy(points.begin(), points.end(), stream);
    }
  }
};

//----------------------------------------------------------------------------
// Inserts a cell of npts points into a vtkCellArray, with its typed arrays.
template <typename PointsIterator>
struct InsertNextCellPointsImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdType npts, PointsIterator pts) const
  {
    using ValueType = GetAPIType<OffsetsT>;
    vtkDataArrayAccessor<OffsetsT> offsetsAccessor(offsets);
    vtkDataArrayAccessor<ConnectivityT> connAccessor(conn);
    offsetsAccessor.InsertNext(static_cast<ValueType>(conn->GetNumberOfValues() + npts));
    for (vtkIdType i = 0; i < npts; ++i)
    {
      connAccessor.InsertNext(static_cast<ValueType>(pts[i]));
    }
  }
};

// Copy the faces of a cell into a vtkCellArray, one cell per face.
struct GetCellFacesImpl : public vtkPolyhedronCellArray::DispatchUtilities
{
  template <class FaceOffsetsT, class FaceConnectivityT, class LocationOffsetsT,
    class LocationConnectivityT>
  void operator()(FaceOffsetsT* faceOffsets, FaceConnectivityT* faceConnectivity,
    LocationOffsetsT* locationOffsets, LocationConnectivityT* locationConnectivity,
    vtkIdType cellId, vtkCellArray* faces) const
  {
    const auto faceIds = GetCellRange(locationOffsets, locationConnectivity, cellId);
    faces->AllocateExact(faceIds.size(),
      GetCellNumberOfFacePointsImpl{}(
        faceOffsets, faceConnectivity, locationOffsets, locationConnectivity, cellId));
    const auto connectivity = GetRange(faceConnectivity);
    for (const vtkIdType faceId : faceIds)
    {
      const vtkIdType begin = GetBeginOffset(faceOffsets, faceId);
      const auto points = connectivity.begin() + begin;
      using InsertPoints = InsertNextCellPointsImpl<decltype(points)>;
      faces->Dispatch(InsertPoints{}, GetEndOffset(faceOffsets, faceId) - begin, points);
    }
  }
};

//----------------------------------------------------------------------------
// Write the faces of the cells in the legacy layout, see ExportLegacyFormat(), and return the
// number of values of the face streams.
struct ExportLegacyFormatImpl : public vtkPolyhedronCellArray::DispatchUtilities
{
  template <class FaceOffsetsT, class FaceConnectivityT, class LocationOffsetsT,
    class LocationConnectivityT>
  vtkIdType operator()(FaceOffsetsT* faceOffsets, FaceConnectivityT* faceConnectivity,
    LocationOffsetsT* locationOffsets, LocationConnectivityT* locationConnectivity,
    vtkIdType* faces, vtkIdType* faceLocations) const
  {
    const vtkIdType numCells = locationOffsets->GetNumberOfValues() - 1;
    vtkIdType* stream = faces;
    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
      const auto faceIds = GetCellRange(locationOffsets, locationConnectivity, cellId);
      if (faceIds.size() == 0)
      {
        faceLocations[cellId] = -1;
        continue;
      }
      faceLocations[cellId] = stream - faces;
      *stream++ = faceIds.size();
      for (const vtkIdType faceId : faceIds)
      {
        const auto points = GetCellRange(faceOffsets, faceConnectivity, faceId);
        *stream++ = points.size();
        stream = std::copy(points.begin(), points.end(), stream);
      }
    }
    return stream - faces;
  }
};
} // anonymous namespace

//----------------------------------------------------------------------------
vtkPolyhedronCellArray::vtkPolyhedronCellArray()
  : Faces(vtkSmartPointer<vtkCellArray>::New())
  , FaceLocations(vtkSmartPointer<vtkCellArray>::New())
{
}

//----------------------------------------------------------------------------
vtkPolyhedronCellArray::~vtkPolyhedronCellArray() = default;

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "StorageType: " << this->GetStorageType() << "\n";
  os << indent << "Faces:\n";
  this->Faces->PrintSelf(os, indent.GetNextIndent());
  os << indent << "FaceLocations:\n";
  this->FaceLocations->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
bool vtkPolyhedronCellArray::AllocateEstimate(
  vtkIdType numCells, vtkIdType maxFacesPerCell, vtkIdType maxFacePoints)
{
  return this->Faces->AllocateEstimate(numCells * maxFacesPerCell, maxFacePoints) &&
    this->FaceLocations->AllocateEstimate(numCells, maxFacesPerCell);
}

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::Initialize()
{
  this->Faces->Initialize();
  this->FaceLocations->Initialize();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::Reset()
{
  this->Faces->Reset();
  this->FaceLocations->Reset();
}

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::Squeeze()
{
  this->Faces->Squeeze();
  this->FaceLocations->Squeeze();
}

//----------------------------------------------------------------------------
unsigned long vtkPolyhedronCellArray::GetActualMemorySize() const
{
  return this->Faces->GetActualMemorySize() + this->FaceLocations->GetActualMemorySize();
}

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::DeepCopy(vtkPolyhedronCellArray* other)
{
  if (other == this)
  {
    return;
  }
  this->Faces->DeepCopy(other->Faces);
  this->FaceLocations->DeepCopy(other->FaceLocations);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::ShallowCopy(vtkPolyhedronCellArray* other)
{
  if (other == this)
  {
    return;
  }
  this->SetData(other->Faces, other->FaceLocations);
}

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::SetData(vtkCellArray* faces, vtkCellArray* faceLocations)
{
  if (!faces || !faceLocations)
  {
    vtkErrorMacro("Empty faces or face locations.");
    return;
  }
  if (this->Faces != faces || this->FaceLocations != faceLocations)
  {
    this->Faces = faces;
    this->FaceLocations = faceLocations;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkPolyhedronCellArray::GetMTime()
{
  return std::max(
    { this->Superclass::GetMTime(), this->Faces->GetMTime(), this->FaceLocations->GetMTime() });
}

//----------------------------------------------------------------------------
bool vtkPolyhedronCellArray::ConvertTo64BitStorage()
{
  return this->Faces->ConvertTo64BitStorage() && this->FaceLocations->ConvertTo64BitStorage();
}

//----------------------------------------------------------------------------
bool vtkPolyhedronCellArray::ConvertTo32BitStorage()
{
  return this->Faces->ConvertTo32BitStorage() && this->FaceLocations->ConvertTo32BitStorage();
}

//----------------------------------------------------------------------------
bool vtkPolyhedronCellArray::ConvertToDefaultStorage()
{
  return this->Faces->ConvertToDefaultStorage() && this->FaceLocations->ConvertToDefaultStorage();
}

//----------------------------------------------------------------------------
bool vtkPolyhedronCellArray::ConvertToSmallestStorage()
{
  vtkCellArray* faces = this->Faces;
  vtkCellArray* locations = this->FaceLocations;
  if (!faces->ConvertToSmallestStorage() || !locations->ConvertToSmallestStorage())
  {
    return false;
  }
  // Give both arrays the same storage type, otherwise Dispatch() uses the Generic one.
  const bool is64Bit = faces->IsStorage64Bit() || faces->IsStorageFixedSize64Bit() ||
    locations->IsStorage64Bit() || locations->IsStorageFixedSize64Bit();
  if (faces->IsStorageFixedSize() && locations->IsStorageFixedSize())
  {
    return is64Bit
      ? faces->ConvertToFixedSize64BitStorage() && locations->ConvertToFixedSize64BitStorage()
      : faces->ConvertToFixedSize32BitStorage() && locations->ConvertToFixedSize32BitStorage();
  }
  return is64Bit ? this->ConvertTo64BitStorage() : this->ConvertTo32BitStorage();
}

//----------------------------------------------------------------------------
vtkIdType vtkPolyhedronCellArray::GetCellNumberOfFacePoints(vtkIdType cellId) const
{
  return this->Dispatch(GetCellNumberOfFacePointsImpl{}, cellId);
}

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::GetFacePoints(vtkIdType cellId, int face, vtkIdList* pts) const
{
  vtkIdType npts = 0;
  const vtkIdType* ids = nullptr;
  this->GetFacePoints(cellId, face, npts, ids, pts);
  if (ids != pts->GetPointer(0))
  {
    pts->SetNumberOfIds(npts);
    std::copy_n(ids, npts, pts->GetPointer(0));
  }
}

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::GetFaceStream(vtkIdType cellId, vtkIdList* faceStream) const
{
  faceStream->Reset();
  this->Dispatch(GetFaceStreamImpl{}, cellId, faceStream);
}

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::GetCellFaces(vtkIdType cellId, vtkCellArray* faces) const
{
  faces->Reset();
  this->Dispatch(GetCellFacesImpl{}, cellId, faces);
}

//----------------------------------------------------------------------------
vtkIdType vtkPolyhedronCellArray::InsertNextCell(vtkIdType numFaces, const vtkIdType* faceStream)
{
  vtkIdType faceId = this->Faces->GetNumberOfCells();
  const vtkIdType cellId = this->FaceLocations->InsertNextCell(static_cast<int>(numFaces));
  for (vtkIdType face = 0; face < numFaces; ++face)
  {
    this->FaceLocations->InsertCellPoint(faceId++);
  }
  for (vtkIdType face = 0; face < numFaces; ++face)
  {
    const vtkIdType npts = faceStream[0];
    this->Faces->InsertNextCell(npts, faceStream + 1);
    faceStream += npts + 1;
  }
  return cellId;
}

//----------------------------------------------------------------------------
vtkIdType vtkPolyhedronCellArray::InsertNextCell(vtkCellArray* faces)
{
  const vtkIdType numFaces = faces->GetNumberOfCells();
  vtkIdType faceId = this->Faces->GetNumberOfCells();
  const vtkIdType cellId = this->FaceLocations->InsertNextCell(static_cast<int>(numFaces));
  for (vtkIdType face = 0; face < numFaces; ++face)
  {
    this->FaceLocations->InsertCellPoint(faceId++);
  }
  this->Faces->Append(faces);
  return cellId;
}

//----------------------------------------------------------------------------
vtkIdType vtkPolyhedronCellArray::InsertEmptyCells(vtkIdType numCells)
{
  vtkIdType cellId = -1;
  for (vtkIdType i = 0; i < numCells; ++i)
  {
    cellId = this->FaceLocations->InsertNextCell(0);
  }
  return cellId;
}

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::Append(vtkPolyhedronCellArray* source, vtkIdType pointOffset)
{
  const vtkIdType faceOffset = this->Faces->GetNumberOfCells();
  this->Faces->Append(source->Faces, pointOffset);
  this->FaceLocations->Append(source->FaceLocations, faceOffset);
}

//----------------------------------------------------------------------------
void vtkPolyhedronCellArray::ExportLegacyFormat(
  vtkIdTypeArray* faces, vtkIdTypeArray* faceLocations)
{
  // At most one value per cell, per face, and per point of a face.
  faces->SetNumberOfValues(this->GetNumberOfCells() + this->GetNumberOfFaces() +
    this->Faces->GetNumberOfConnectivityIds());
  faceLocations->SetNumberOfValues(this->GetNumberOfCells());
  faces->SetNumberOfValues(
    this->Dispatch(ExportLegacyFormatImpl{}, faces->GetPointer(0), faceLocations->GetPointer(0)));
}

//----------------------------------------------------------------------------
bool vtkPolyhedronCellArray::ImportLegacyFormat(
  vtkIdTypeArray* faces, vtkIdTypeArray* faceLocations)
{
  return this->ImportLegacyFormat(faces->GetPointer(0), faces->GetNumberOfValues(),
    faceLocations->GetPointer(0), faceLocations->GetNumberOfValues());
}

//----------------------------------------------------------------------------
bool vtkPolyhedronCellArray::ImportLegacyFormat(const vtkIdType* faces, vtkIdType facesLength,
  const vtkIdType* faceLocations, vtkIdType numberOfCells)
{
  vtkNew<vtkPolyhedronCellArray> imported;
  if (!imported->AppendLegacyFormat(faces, facesLength, faceLocations, numberOfCells))
  {
    return false;
  }
  this->Faces->DeepCopy(imported->Faces);
  this->FaceLocations->DeepCopy(imported->FaceLocations);
  this->Modified();
  return true;
}

//----------------------------------------------------------------------------
bool vtkPolyhedronCellArray::AppendLegacyFormat(
  vtkIdTypeArray* faces, vtkIdTypeArray* faceLocations, vtkIdType ptOffset)
{
  return this->AppendLegacyFormat(faces->GetPointer(0), faces->GetNumberOfValues(),
    faceLocations->GetPointer(0), faceLocations->GetNumberOfValues(), ptOffset);
}

//----------------------------------------------------------------------------
bool vtkPolyhedronCellArray::AppendLegacyFormat(const vtkIdType* faces, vtkIdType facesLength,
  const vtkIdType* faceLocations, vtkIdType numberOfCells, vtkIdType ptOffset)
{
  // Check the layout first, so that this is left unchanged if it is corrupt: the face stream of
  // every cell with faces must be in the faces.
  for (vtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
  {
    vtkIdType pos = faceLocations[cellId];
    if (pos < 0)
    {
      continue;
    }
    if (pos >= facesLength || faces[pos] < 0)
    {
      return false;
    }
    const vtkIdType cellFaces = faces[pos++];
    for (vtkIdType face = 0; face < cellFaces; ++face)
    {
      if (pos >= facesLength || faces[pos] < 0 || pos + 1 + faces[pos] > facesLength)
      {
        return false;
      }
      pos += 1 + faces[pos];
    }
  }
  std::vector<vtkIdType> points;
  vtkIdType faceId = this->Faces->GetNumberOfCells();
  for (vtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
  {
    vtkIdType pos = faceLocations[cellId];
    const vtkIdType cellFaces = pos < 0 ? 0 : faces[pos++];
    this->FaceLocations->InsertNextCell(static_cast<int>(cellFaces));
    for (vtkIdType face = 0; face < cellFaces; ++face)
    {
      this->FaceLocations->InsertCellPoint(faceId++);
      const vtkIdType npts = faces[pos++];
      points.resize(npts);
      std::transform(faces + pos, faces + pos + npts, points.begin(),
        [ptOffset](vtkIdType id) { return id + ptOffset; });
      this->Faces->InsertNextCell(npts, points.data());
      pos += npts;
    }
  }
  return true;
}
VTK_ABI_NAMESPACE_END
