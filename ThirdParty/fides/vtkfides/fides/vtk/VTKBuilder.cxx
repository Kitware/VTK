//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/vtk/VTKBuilder.h>

#include <vtkArrayDispatch.h>
#include <vtkCellArray.h>
#include <vtkCellAttribute.h>
#include <vtkCellData.h>
#include <vtkCellGrid.h>
#include <vtkCellMetadata.h>
#include <vtkCellType.h>
#include <vtkCompositeDataSet.h>
#include <vtkDGCell.h>
#include <vtkDataArrayRange.h>
#include <vtkDataAssembly.h>
#include <vtkDataSet.h>
#include <vtkDataSetAttributes.h>
#include <vtkFiltersCellGrid.h>
#include <vtkIOCellGrid.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStringToken.h>
#include <vtkStructuredGrid.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>

#include <vtkSOADataArrayTemplate.h>

#include <mutex>
#include <stdexcept>
#include <unordered_map>

namespace fides
{

namespace
{

// --- Lifetime management for zero-copy VTK arrays ---
// VTK's SetArrayFreeFunction takes a plain void(*)(void*) callback.
// We use a global map to tie the RawArray's shared_ptr lifetime to
// the VTK array buffer pointer so the data stays alive until VTK
// releases it.
std::mutex g_rawArrayRefsMutex;
// A single source pointer may back multiple VTK arrays: two fields that
// share one deduplicated read buffer each wrap the same pointer, and SOA
// composite coordinates can share a base address across components. Using a
// multimap (rather than a keyed map) means each wrapping registers its own
// reference, and freeing one array erases only that one reference instead of
// dropping the buffer out from under the still-living siblings.
std::unordered_multimap<void*, std::shared_ptr<void>> g_rawArrayRefs;

void KeepRawArrayAlive(void* ptr, std::shared_ptr<void> ref)
{
  std::lock_guard<std::mutex> lock(g_rawArrayRefsMutex);
  g_rawArrayRefs.emplace(ptr, std::move(ref));
}

void ReleaseRawArrayRef(void* ptr)
{
  std::lock_guard<std::mutex> lock(g_rawArrayRefsMutex);
  auto it = g_rawArrayRefs.find(ptr);
  if (it != g_rawArrayRefs.end())
  {
    g_rawArrayRefs.erase(it);
  }
}

// --- Zero-copy AOS array from RawArray ---
template <typename T>
vtkSmartPointer<vtkAOSDataArrayTemplate<T>> MakeZeroCopyVTKArrayImpl(const RawArray& raw)
{
  auto arr = vtkSmartPointer<vtkAOSDataArrayTemplate<T>>::New();
  size_t totalElements = raw.NumValues * static_cast<size_t>(raw.NumComponents);
  T* ptr = const_cast<T*>(raw.GetPointer<T>());
  KeepRawArrayAlive(ptr, raw.Data);
  arr->SetArray(
    ptr, static_cast<vtkIdType>(totalElements), 0, vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED);
  arr->SetArrayFreeFunction(ReleaseRawArrayRef);
  arr->SetNumberOfComponents(raw.NumComponents);
  return arr;
}

// --- Zero-copy SOA points array from three separate component RawArrays ---
template <typename T>
vtkSmartPointer<vtkSOADataArrayTemplate<T>> MakeSOAPointsArray(const RawArray& xRaw,
                                                               const RawArray& yRaw,
                                                               const RawArray& zRaw)
{
  auto arr = vtkSmartPointer<vtkSOADataArrayTemplate<T>>::New();
  vtkIdType nPts = static_cast<vtkIdType>(xRaw.NumValues);
  arr->SetNumberOfComponents(3);

  T* xPtr = const_cast<T*>(xRaw.GetPointer<T>());
  T* yPtr = const_cast<T*>(yRaw.GetPointer<T>());
  T* zPtr = const_cast<T*>(zRaw.GetPointer<T>());

  KeepRawArrayAlive(xPtr, xRaw.Data);
  KeepRawArrayAlive(yPtr, yRaw.Data);
  KeepRawArrayAlive(zPtr, zRaw.Data);

  arr->SetArray(0, xPtr, nPts, false, false, vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED);
  arr->SetArrayFreeFunction(0, ReleaseRawArrayRef);
  arr->SetArray(1, yPtr, nPts, false, false, vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED);
  arr->SetArrayFreeFunction(1, ReleaseRawArrayRef);
  arr->SetArray(2, zPtr, nPts, true, false, vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED);
  arr->SetArrayFreeFunction(2, ReleaseRawArrayRef);

  return arr;
}

// --- Zero-copy connectivity array for vtkCellArray ---
// vtkCellArray natively handles int32 and int64 storage.
// For unsigned types, we bulk-convert to the matching signed type.
vtkSmartPointer<vtkDataArray> MakeConnectivityArray(const RawArray& connRaw)
{
  switch (connRaw.Type)
  {
    case DataType::Int32:
      return MakeZeroCopyVTKArrayImpl<int32_t>(connRaw);
    case DataType::Int64:
      return MakeZeroCopyVTKArrayImpl<int64_t>(connRaw);
    case DataType::UInt32:
    {
      // Bulk-convert uint32 -> int32
      size_t n = connRaw.NumValues;
      auto arr = vtkSmartPointer<vtkAOSDataArrayTemplate<int32_t>>::New();
      arr->SetNumberOfTuples(static_cast<vtkIdType>(n));
      const uint32_t* src = connRaw.GetPointer<uint32_t>();
      int32_t* dst = arr->GetPointer(0);
      for (size_t i = 0; i < n; i++)
      {
        dst[i] = static_cast<int32_t>(src[i]);
      }
      return arr;
    }
    case DataType::UInt64:
    {
      // Bulk-convert uint64 -> int64
      size_t n = connRaw.NumValues;
      auto arr = vtkSmartPointer<vtkAOSDataArrayTemplate<int64_t>>::New();
      arr->SetNumberOfTuples(static_cast<vtkIdType>(n));
      const uint64_t* src = connRaw.GetPointer<uint64_t>();
      int64_t* dst = arr->GetPointer(0);
      for (size_t i = 0; i < n; i++)
      {
        dst[i] = static_cast<int64_t>(src[i]);
      }
      return arr;
    }
    default:
      throw std::runtime_error("MakeConnectivityArray: unsupported DataType for connectivity");
  }
}

// --- Type-dispatched single-value read from RawArray ---
template <typename OutT>
OutT ReadRawValue(const RawArray& raw, size_t index)
{
  switch (raw.Type)
  {
    case DataType::UInt8:
      return static_cast<OutT>(raw.GetPointer<uint8_t>()[index]);
    case DataType::Int32:
      return static_cast<OutT>(raw.GetPointer<int32_t>()[index]);
    case DataType::Int64:
      return static_cast<OutT>(raw.GetPointer<int64_t>()[index]);
    case DataType::UInt32:
      return static_cast<OutT>(raw.GetPointer<uint32_t>()[index]);
    case DataType::UInt64:
      return static_cast<OutT>(raw.GetPointer<uint64_t>()[index]);
    default:
      return static_cast<OutT>(raw.GetPointer<int32_t>()[index]);
  }
}

/// CellShape enum values match VTK cell type constants directly.
int ConvertCellShapeToVTK(CellShape shape)
{
  return static_cast<int>(shape);
}

} // end anon namespace

vtkSmartPointer<vtkDataArray> VTKBuilder::MakeVTKArray(const RawArray& raw)
{
  switch (raw.Type)
  {
    case DataType::Float32:
      return MakeZeroCopyVTKArrayImpl<float>(raw);
    case DataType::Float64:
      return MakeZeroCopyVTKArrayImpl<double>(raw);
    case DataType::Int8:
      return MakeZeroCopyVTKArrayImpl<int8_t>(raw);
    case DataType::Int16:
      return MakeZeroCopyVTKArrayImpl<int16_t>(raw);
    case DataType::Int32:
      return MakeZeroCopyVTKArrayImpl<int32_t>(raw);
    case DataType::Int64:
      return MakeZeroCopyVTKArrayImpl<int64_t>(raw);
    case DataType::UInt8:
      return MakeZeroCopyVTKArrayImpl<uint8_t>(raw);
    case DataType::UInt16:
      return MakeZeroCopyVTKArrayImpl<uint16_t>(raw);
    case DataType::UInt32:
      return MakeZeroCopyVTKArrayImpl<uint32_t>(raw);
    case DataType::UInt64:
      return MakeZeroCopyVTKArrayImpl<uint64_t>(raw);
    default:
      throw std::runtime_error("VTKBuilder::MakeVTKArray: unknown DataType");
  }
}

size_t VTKBuilder::CreateUniformCoordinates(const int64_t dims[3],
                                            const double origin[3],
                                            const double spacing[3],
                                            const int64_t start[3])
{
  if (dims[0] < 0 || dims[0] > INT_MAX || dims[1] < 0 || dims[1] > INT_MAX || dims[2] < 0 ||
      dims[2] > INT_MAX)
  {
    throw std::invalid_argument("VTKBuilder::CreateUniformCoordinates: VTK uniform grid dimensions "
                                "must be between 0 and INT_MAX.");
  }
  // Store origin unshifted and keep start separately. Partitions inside a
  // vtkPartitionedDataSet should share origin/spacing and differ only in
  // extent, so Finalize uses Start+Dims to call SetExtent rather than
  // folding Start into Origin (which would shift each partition into its
  // own local coordinate system and break filters that compare extents).
  size_t token = this->AllocToken();
  CoordEntry entry;
  entry.EntryType = CoordEntry::Type::Uniform;
  for (int i = 0; i < 3; i++)
  {
    entry.Dims[i] = dims[i];
    entry.Origin[i] = origin[i];
    entry.Spacing[i] = spacing[i];
    entry.Start[i] = start ? start[i] : 0;
  }
  this->StoredCoords[token] = entry;
  return token;
}

size_t VTKBuilder::CreateStructuredCellSet(const int64_t dims[3])
{

  if (dims[0] < 0 || dims[0] > INT_MAX || dims[1] < 0 || dims[1] > INT_MAX || dims[2] < 0 ||
      dims[2] > INT_MAX)
  {
    throw std::invalid_argument("VTKBuilder::CreateStructuredCellSet: VTK structured grid "
                                "dimensions must be between 0 and INT_MAX.");
  }
  return OutputBuilder::CreateStructuredCellSet(dims);
}

void VTKBuilder::Reset()
{
  OutputBuilder::Reset();
  this->DataSetsVec.clear();
}

// --- Finalize: build all VTK objects from populated data ---

void VTKBuilder::Finalize()
{
  this->DataSetsVec.clear();
  this->DataSetTokenMap.clear();

  // Build per-dataset lookup: dsToken -> coordToken, dsToken -> cellSetToken
  std::unordered_map<size_t, size_t> dsToCoord;
  std::unordered_map<size_t, size_t> dsToCellSet;

  for (auto& dc : this->DeferredCoordSystems)
  {
    dsToCoord[dc.DataSetToken] = dc.CoordToken;
  }
  for (auto& dcs : this->DeferredCellSets)
  {
    dsToCellSet[dcs.DataSetToken] = dcs.CellSetToken;
  }

  // For each dataset token, determine the VTK type and create it
  for (auto dsToken : this->DataSetTokens)
  {
    size_t index = this->DataSetsVec.size();
    this->DataSetTokenMap[dsToken] = index;

    // Look up coord and cell set entries
    CoordEntry::Type coordType = CoordEntry::Type::Array;
    CoordEntry* coordEntry = nullptr;
    auto coordIt = dsToCoord.find(dsToken);
    if (coordIt != dsToCoord.end())
    {
      auto ceIt = this->StoredCoords.find(coordIt->second);
      if (ceIt != this->StoredCoords.end())
      {
        coordEntry = &ceIt->second;
        coordType = ceIt->second.EntryType;
      }
      else
      {
        // Plain array token as coordinate - treat as Array type
        coordType = CoordEntry::Type::Array;
      }
    }

    CellSetEntry::Type cellSetType = CellSetEntry::Type::Structured;
    CellSetEntry* cellSetEntry = nullptr;
    auto csIt = dsToCellSet.find(dsToken);
    if (csIt != dsToCellSet.end())
    {
      auto seIt = this->StoredCellSets.find(csIt->second);
      if (seIt != this->StoredCellSets.end())
      {
        cellSetEntry = &seIt->second;
        cellSetType = seIt->second.EntryType;
      }
    }

    // Determine VTK dataset type
    if (cellSetType == CellSetEntry::Type::Structured)
    {
      if (coordType == CoordEntry::Type::Uniform && coordEntry)
      {
        // vtkImageData
        auto imageData = vtkSmartPointer<vtkImageData>::New();
        // Use SetExtent (not SetDimensions) so sibling partitions in a
        // vtkPartitionedDataSet share origin and differ only in extent.
        const int64_t* s = coordEntry->Start;
        const int64_t* d = coordEntry->Dims;
        imageData->SetExtent(static_cast<int>(s[0]),
                             static_cast<int>(s[0] + d[0] - 1),
                             static_cast<int>(s[1]),
                             static_cast<int>(s[1] + d[1] - 1),
                             static_cast<int>(s[2]),
                             static_cast<int>(s[2] + d[2] - 1));
        imageData->SetOrigin(coordEntry->Origin[0], coordEntry->Origin[1], coordEntry->Origin[2]);
        imageData->SetSpacing(
          coordEntry->Spacing[0], coordEntry->Spacing[1], coordEntry->Spacing[2]);
        this->DataSetsVec.push_back(imageData);
      }
      else if (coordType == CoordEntry::Type::Rectilinear && coordEntry)
      {
        // vtkRectilinearGrid
        auto rectGrid = vtkSmartPointer<vtkRectilinearGrid>::New();
        auto xIt = this->StoredArrays.find(coordEntry->XToken);
        auto yIt = this->StoredArrays.find(coordEntry->YToken);
        auto zIt = this->StoredArrays.find(coordEntry->ZToken);
        if (xIt == this->StoredArrays.end() || yIt == this->StoredArrays.end() ||
            zIt == this->StoredArrays.end())
        {
          throw std::runtime_error("VTKBuilder::Finalize: invalid rectilinear coord tokens");
        }
        auto xArr = VTKBuilder::MakeVTKArray(xIt->second);
        auto yArr = VTKBuilder::MakeVTKArray(yIt->second);
        auto zArr = VTKBuilder::MakeVTKArray(zIt->second);
        rectGrid->SetDimensions(static_cast<int>(xIt->second.NumValues),
                                static_cast<int>(yIt->second.NumValues),
                                static_cast<int>(zIt->second.NumValues));
        rectGrid->SetXCoordinates(xArr);
        rectGrid->SetYCoordinates(yArr);
        rectGrid->SetZCoordinates(zArr);
        this->DataSetsVec.push_back(rectGrid);
      }
      else
      {
        // vtkStructuredGrid (Composite/Array coords + structured cell set)
        auto structGrid = vtkSmartPointer<vtkStructuredGrid>::New();

        // Set dimensions from cell set entry
        if (cellSetEntry)
        {
          structGrid->SetDimensions(static_cast<int>(cellSetEntry->Dims[0]),
                                    static_cast<int>(cellSetEntry->Dims[1]),
                                    static_cast<int>(cellSetEntry->Dims[2]));
        }

        // Build vtkPoints from coordinate data
        auto points = vtkSmartPointer<vtkPoints>::New();
        if (coordEntry && coordType == CoordEntry::Type::Composite)
        {
          // Three separate component arrays that need interleaving
          auto xIt = this->StoredArrays.find(coordEntry->XToken);
          auto yIt = this->StoredArrays.find(coordEntry->YToken);
          auto zIt = this->StoredArrays.find(coordEntry->ZToken);
          if (xIt == this->StoredArrays.end() || yIt == this->StoredArrays.end() ||
              zIt == this->StoredArrays.end())
          {
            throw std::runtime_error("VTKBuilder::Finalize: invalid composite coord tokens");
          }
          if (xIt->second.Type == DataType::Float32)
          {
            auto soaArr = MakeSOAPointsArray<float>(xIt->second, yIt->second, zIt->second);
            points->SetData(soaArr);
          }
          else
          {
            auto soaArr = MakeSOAPointsArray<double>(xIt->second, yIt->second, zIt->second);
            points->SetData(soaArr);
          }
        }
        else if (coordIt != dsToCoord.end())
        {
          // Plain array token for coordinates (interleaved xyz)
          auto arrIt = this->StoredArrays.find(coordIt->second);
          if (arrIt != this->StoredArrays.end())
          {
            auto vtkArr = VTKBuilder::MakeVTKArray(arrIt->second);
            vtkArr->SetNumberOfComponents(3);
            points->SetData(vtkArr);
          }
        }
        structGrid->SetPoints(points);
        this->DataSetsVec.push_back(structGrid);
      }
    }
    else if (cellSetType == CellSetEntry::Type::PolyData)
    {
      // vtkPolyData -- points from explicit coords, plus up to four
      // vtkCellArrays for verts/lines/polys/strips. Any role whose
      // token pair is InvalidToken (or absent from StoredArrays) is
      // left as an empty vtkCellArray, which is the vtkPolyData
      // default.
      auto pd = vtkSmartPointer<vtkPolyData>::New();

      auto points = vtkSmartPointer<vtkPoints>::New();
      if (coordEntry)
      {
        if (coordType == CoordEntry::Type::Composite)
        {
          auto xIt = this->StoredArrays.find(coordEntry->XToken);
          auto yIt = this->StoredArrays.find(coordEntry->YToken);
          auto zIt = this->StoredArrays.find(coordEntry->ZToken);
          if (xIt == this->StoredArrays.end() || yIt == this->StoredArrays.end() ||
              zIt == this->StoredArrays.end())
          {
            throw std::runtime_error("VTKBuilder::Finalize: invalid composite coord tokens "
                                     "for polydata partition");
          }
          if (xIt->second.Type == DataType::Float32)
          {
            auto soaArr = MakeSOAPointsArray<float>(xIt->second, yIt->second, zIt->second);
            points->SetData(soaArr);
          }
          else
          {
            auto soaArr = MakeSOAPointsArray<double>(xIt->second, yIt->second, zIt->second);
            points->SetData(soaArr);
          }
        }
        else
        {
          throw std::runtime_error("VTKBuilder::Finalize: only Composite or Array coordinates "
                                   "are supported with polydata");
        }
      }
      else if (coordIt != dsToCoord.end())
      {
        auto arrIt = this->StoredArrays.find(coordIt->second);
        if (arrIt != this->StoredArrays.end())
        {
          auto vtkArr = VTKBuilder::MakeVTKArray(arrIt->second);
          vtkArr->SetNumberOfComponents(3);
          points->SetData(vtkArr);
        }
      }
      pd->SetPoints(points);

      auto buildRole = [&](size_t offsetsToken, size_t connToken) -> vtkSmartPointer<vtkCellArray> {
        if (offsetsToken == InvalidToken || connToken == InvalidToken)
        {
          return nullptr;
        }
        auto offIt = this->StoredArrays.find(offsetsToken);
        auto connIt = this->StoredArrays.find(connToken);
        if (offIt == this->StoredArrays.end() || connIt == this->StoredArrays.end())
        {
          return nullptr;
        }
        auto offArr = MakeConnectivityArray(offIt->second);
        auto connArr = MakeConnectivityArray(connIt->second);
        auto cellArray = vtkSmartPointer<vtkCellArray>::New();
        cellArray->SetData(offArr, connArr);
        return cellArray;
      };

      if (cellSetEntry)
      {
        if (auto ca =
              buildRole(cellSetEntry->PolyVertsOffsetsToken, cellSetEntry->PolyVertsConnToken))
        {
          pd->SetVerts(ca);
        }
        if (auto ca =
              buildRole(cellSetEntry->PolyLinesOffsetsToken, cellSetEntry->PolyLinesConnToken))
        {
          pd->SetLines(ca);
        }
        if (auto ca =
              buildRole(cellSetEntry->PolyPolysOffsetsToken, cellSetEntry->PolyPolysConnToken))
        {
          pd->SetPolys(ca);
        }
        if (auto ca =
              buildRole(cellSetEntry->PolyStripsOffsetsToken, cellSetEntry->PolyStripsConnToken))
        {
          pd->SetStrips(ca);
        }
      }

      this->DataSetsVec.push_back(pd);
    }
    else
    {
      // SingleType or Explicit -> vtkUnstructuredGrid
      auto ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();

      // Build points from coordinate data
      auto points = vtkSmartPointer<vtkPoints>::New();
      if (coordEntry)
      {
        if (coordType == CoordEntry::Type::Composite)
        {
          auto xIt = this->StoredArrays.find(coordEntry->XToken);
          auto yIt = this->StoredArrays.find(coordEntry->YToken);
          auto zIt = this->StoredArrays.find(coordEntry->ZToken);
          if (xIt == this->StoredArrays.end() || yIt == this->StoredArrays.end() ||
              zIt == this->StoredArrays.end())
          {
            throw std::runtime_error("VTKBuilder::Finalize: invalid composite coord tokens");
          }
          if (xIt->second.Type == DataType::Float32)
          {
            auto soaArr = MakeSOAPointsArray<float>(xIt->second, yIt->second, zIt->second);
            points->SetData(soaArr);
          }
          else
          {
            auto soaArr = MakeSOAPointsArray<double>(xIt->second, yIt->second, zIt->second);
            points->SetData(soaArr);
          }
        }
        else if (coordType == CoordEntry::Type::Rectilinear)
        {
          throw std::runtime_error("VTKBuilder::Finalize: rectilinear coords with "
                                   "unstructured cell set is not supported");
        }
        else if (coordType == CoordEntry::Type::Uniform)
        {
          throw std::runtime_error("VTKBuilder::Finalize: uniform coords with "
                                   "unstructured cell set is not supported");
        }
      }
      else if (coordIt != dsToCoord.end())
      {
        // Plain array token for coordinates
        auto arrIt = this->StoredArrays.find(coordIt->second);
        if (arrIt != this->StoredArrays.end())
        {
          auto vtkArr = VTKBuilder::MakeVTKArray(arrIt->second);
          vtkArr->SetNumberOfComponents(3);
          points->SetData(vtkArr);
        }
      }
      ugrid->SetPoints(points);

      // Build cell array from connectivity
      if (cellSetEntry)
      {
        if (cellSetType == CellSetEntry::Type::SingleType)
        {
          // TODO: SingleType is currently restricted to the 8 linear shapes
          // because the JSON shape vocabulary and ConvertCellShapeToVTK only
          // know those. Extend the vocabulary + helpers to cover quadratic /
          // Lagrange / Bezier types, and consider backing the cell-types
          // array with vtkConstantArray<unsigned char> (an implicit array)
          // so a uniform-type grid pays zero memory for cell types.
          auto connIt = this->StoredArrays.find(cellSetEntry->ConnToken);
          if (connIt == this->StoredArrays.end())
          {
            throw std::runtime_error("VTKBuilder::Finalize: invalid connectivity token");
          }
          const auto& connRaw = connIt->second;
          int vtkCellType = ConvertCellShapeToVTK(cellSetEntry->Shape);
          int vpc = cellSetEntry->VertsPerCell;

          auto connArr = MakeConnectivityArray(connRaw);
          auto cellArray = vtkSmartPointer<vtkCellArray>::New();
          cellArray->SetData(static_cast<vtkIdType>(vpc), connArr);
          ugrid->SetCells(vtkCellType, cellArray);
        }
        else if (cellSetType == CellSetEntry::Type::Explicit)
        {
          auto typesIt = this->StoredArrays.find(cellSetEntry->TypesToken);
          auto nVertsIt = this->StoredArrays.find(cellSetEntry->NVertsToken);
          auto connIt = this->StoredArrays.find(cellSetEntry->ConnTokenExplicit);
          if (typesIt == this->StoredArrays.end() || nVertsIt == this->StoredArrays.end() ||
              connIt == this->StoredArrays.end())
          {
            throw std::runtime_error("VTKBuilder::Finalize: invalid explicit cell set tokens");
          }

          const auto& typesRaw = typesIt->second;
          const auto& nVertsRaw = nVertsIt->second;
          const auto& connRaw = connIt->second;

          vtkIdType nCells = static_cast<vtkIdType>(typesRaw.NumValues);

          // Cell types: zero-copy. The cell-types variable stores VTK
          // cell-type IDs directly (e.g. 12 == VTK_HEXAHEDRON). For the
          // linear shapes Fides supports, Viskores' cell shape IDs use
          // the same numeric values, so files written by the legacy
          // Viskores backend are compatible without translation.
          auto cellTypes = MakeZeroCopyVTKArrayImpl<unsigned char>(typesRaw);

          // Pass 2: Build offsets array via prefix sum over nVerts.
          // Use the same integer width as connectivity for typed storage.
          bool use64 = (connRaw.Type == DataType::Int64 || connRaw.Type == DataType::UInt64);
          vtkSmartPointer<vtkDataArray> offsets;
          if (use64)
          {
            auto oArr = vtkSmartPointer<vtkAOSDataArrayTemplate<int64_t>>::New();
            oArr->SetNumberOfTuples(nCells + 1);
            int64_t* oPtr = oArr->GetPointer(0);
            oPtr[0] = 0;
            for (vtkIdType c = 0; c < nCells; c++)
            {
              int64_t nv = ReadRawValue<int64_t>(nVertsRaw, static_cast<size_t>(c));
              oPtr[c + 1] = oPtr[c] + nv;
            }
            offsets = oArr;
          }
          else
          {
            auto oArr = vtkSmartPointer<vtkAOSDataArrayTemplate<int32_t>>::New();
            oArr->SetNumberOfTuples(nCells + 1);
            int32_t* oPtr = oArr->GetPointer(0);
            oPtr[0] = 0;
            for (vtkIdType c = 0; c < nCells; c++)
            {
              int32_t nv = ReadRawValue<int32_t>(nVertsRaw, static_cast<size_t>(c));
              oPtr[c + 1] = oPtr[c] + nv;
            }
            offsets = oArr;
          }

          // Pass 3: Connectivity (bulk, zero-copy when possible)
          auto connArr = MakeConnectivityArray(connRaw);

          auto cellArray = vtkSmartPointer<vtkCellArray>::New();
          cellArray->SetData(offsets, connArr);
          ugrid->SetCells(cellTypes, cellArray);
        }
      }

      this->DataSetsVec.push_back(ugrid);
    }
  }

  // Apply fields to all datasets
  for (auto& df : this->DeferredFields)
  {
    auto dsIt = this->DataSetTokenMap.find(df.DataSetToken);
    if (dsIt == this->DataSetTokenMap.end())
    {
      continue;
    }

    auto arrIt = this->StoredArrays.find(df.ArrayToken);
    if (arrIt == this->StoredArrays.end())
    {
      continue;
    }

    auto vtkArr = VTKBuilder::MakeVTKArray(arrIt->second);
    vtkArr->SetName(df.Name.c_str());

    auto* ds = vtkDataSet::SafeDownCast(this->DataSetsVec[dsIt->second]);
    if (!ds)
    {
      // Cell grids do not use the point/cell/field data triad; their fields
      // are routed through the cellgrid attribute system in FinalizeCellGrids.
      continue;
    }
    switch (df.Assoc)
    {
      case FieldAssociation::Points:
        ds->GetPointData()->AddArray(vtkArr);
        break;
      case FieldAssociation::Cells:
        ds->GetCellData()->AddArray(vtkArr);
        break;
      case FieldAssociation::WholeDataSet:
        ds->GetFieldData()->AddArray(vtkArr);
        break;
      case FieldAssociation::CellGrid:
        // Unreachable: cell-grid attributes are skipped above when the
        // dataset is a vtkCellGrid (handled in FinalizeCellGrids). Listed
        // explicitly to keep the switch exhaustive.
        break;
    }
  }

  this->FinalizeCellGrids();
}

namespace
{

// Lazy one-shot registration of vtkCellGrid cell types and their I/O
// responders. vtkCellMetadata::NewInstance returns nullptr unless these
// have run; the registrations are idempotent.
void EnsureCellGridRegistration()
{
  static std::once_flag flag;
  std::call_once(flag, []() {
    vtkFiltersCellGrid::RegisterCellsAndResponders();
    vtkIOCellGrid::RegisterCellsAndResponders();
  });
}

} // anon namespace

void VTKBuilder::FinalizeCellGrids()
{
  if (this->StoredCellGrids.empty())
  {
    return;
  }

  EnsureCellGridRegistration();

  for (auto& cgPair : this->StoredCellGrids)
  {
    size_t cgToken = cgPair.first;
    const CellGridEntry& cgEntry = cgPair.second;

    auto cg = vtkSmartPointer<vtkCellGrid>::New();
    cg->Initialize();

    // 1. Materialize all per-attribute arrays referenced by the
    //    cellgrid's attributes and register them in the corresponding
    //    cellgrid attribute group. The materialization cache is keyed on
    //    the underlying RawArray buffer pointer so that two attributes
    //    sharing one buffer (e.g. the shared coordinates pool, or a
    //    connectivity variable referenced by every attribute) get one
    //    vtkDataArray, while two attributes that hold *different*
    //    buffers under the same logical name (e.g. a static-conn cache
    //    holding step-0 bytes alongside a fresh-read step-N buffer)
    //    each materialize their own array. Group-registration is
    //    deduplicated separately via addedGroupArrays so the cellgrid's
    //    attribute group still holds one array per (group, name).
    std::unordered_map<const void*, vtkSmartPointer<vtkDataArray>> arrayCache;
    std::unordered_map<std::string, vtkSmartPointer<vtkDataArray>> addedGroupArrays;
    auto groupKey = [](const std::string& group, const std::string& name) {
      return group + "\x1f" + name;
    };
    auto materializeArray = [&](const CellAttributeArrayRef& ref) -> vtkSmartPointer<vtkDataArray> {
      if (ref.Token == InvalidToken)
      {
        return nullptr;
      }
      auto storedIt = this->StoredArrays.find(ref.Token);
      if (storedIt == this->StoredArrays.end())
      {
        return nullptr;
      }
      const void* bufKey = storedIt->second.Data.get();
      auto cached = arrayCache.find(bufKey);
      if (cached != arrayCache.end())
      {
        return cached->second;
      }
      auto vtkArr = VTKBuilder::MakeVTKArray(storedIt->second);
      vtkArr->SetName(ref.ArrayName.c_str());
      arrayCache[bufKey] = vtkArr;

      // Register the array on the cellgrid's named attribute group only
      // once per (group, name) — duplicate AddArray calls would replace
      // earlier entries, leaving the group inconsistent with the
      // per-attribute ArraysByRole pointers that already captured the
      // first materialization.
      const std::string gkey = groupKey(ref.Group, ref.ArrayName);
      if (!addedGroupArrays.count(gkey))
      {
        vtkStringToken groupToken(ref.Group);
        auto* attrs = cg->GetAttributes(groupToken.GetId());
        if (attrs)
        {
          attrs->AddArray(vtkArr);
        }
        addedGroupArrays[gkey] = vtkArr;
      }
      return vtkArr;
    };

    for (size_t attrToken : cgEntry.AttributeTokens)
    {
      auto entryIt = this->StoredCellAttributes.find(attrToken);
      if (entryIt == this->StoredCellAttributes.end())
      {
        continue;
      }
      const CellAttributeEntry& attrEntry = entryIt->second;
      for (const auto& perType : attrEntry.PerCellType)
      {
        for (const auto& rolePair : perType.Roles)
        {
          materializeArray(rolePair.second);
        }
      }
    }

    // 2. Register cell types. NewInstance attaches the metadata to the grid;
    //    discard the returned pointer.
    for (const auto& ct : cgEntry.CellTypes)
    {
      vtkStringToken typeToken(ct.CellTypeName);
      auto cell = vtkCellMetadata::NewInstance(typeToken, cg);
      if (!cell)
      {
        throw std::runtime_error("VTKBuilder::FinalizeCellGrids: unknown cell type '" +
                                 ct.CellTypeName + "'.");
      }
      // This path constructs the grid by hand and currently understands
      // only the vtkDGCell family (every cell type VTK ships today). Reject
      // any other vtkCellMetadata subclass loudly rather than producing a
      // silently incomplete grid; generic (de)serialization through the
      // cell-grid responder framework is tracked as future work.
      if (!vtkDGCell::SafeDownCast(cell))
      {
        throw std::runtime_error(
          "VTKBuilder::FinalizeCellGrids: cell type '" + ct.CellTypeName +
          "' is not a vtkDGCell; responder-based deserialization is not yet implemented.");
      }
    }

    // 3. Build vtkCellAttribute objects. For the shape attribute, also wire
    //    each cell type's vtkDGCell::CellSpec with its connectivity and
    //    source shape so vtkCellGrid can answer geometry queries.
    for (size_t attrToken : cgEntry.AttributeTokens)
    {
      auto entryIt = this->StoredCellAttributes.find(attrToken);
      if (entryIt == this->StoredCellAttributes.end())
      {
        continue;
      }
      const CellAttributeEntry& attrEntry = entryIt->second;

      auto attribute = vtkSmartPointer<vtkCellAttribute>::New();
      attribute->Initialize(
        vtkStringToken(attrEntry.Name), vtkStringToken(attrEntry.Space), attrEntry.Components);
      cg->AddCellAttribute(attribute);
      if (attrEntry.IsShape)
      {
        cg->SetShapeAttribute(attribute);
      }

      for (const auto& perType : attrEntry.PerCellType)
      {
        vtkStringToken cellTypeToken(perType.CellTypeName);

        vtkCellAttribute::CellTypeInfo info;
        info.FunctionSpace = vtkStringToken(perType.FunctionSpace);
        info.Basis = vtkStringToken(perType.Basis);
        info.Order = perType.Order;
        if (!perType.DOFSharing.empty())
        {
          info.DOFSharing = vtkStringToken(perType.DOFSharing);
        }
        // Populate every role the (attribute, cell-type) carries, not just
        // a fixed values/connectivity pair. "ghost-node" is a vtkDGCell
        // source field (NodalGhostMarks), not a canonical attribute role,
        // so it is wired into the CellSpec below and kept out of
        // ArraysByRole.
        for (const auto& [role, ref] : perType.Roles)
        {
          if (role == "ghost-node")
          {
            continue;
          }
          if (auto arr = materializeArray(ref))
          {
            info.ArraysByRole[vtkStringToken(role)] = arr;
          }
        }
        attribute->SetCellTypeInfo(cellTypeToken, info);

        if (attrEntry.IsShape)
        {
          // The shape attribute drives the cell type's geometric source
          // (vtkDGCell::Source) so the cellgrid can iterate cells.
          auto* meta = cg->GetCellType(cellTypeToken);
          if (auto* dgCell = vtkDGCell::SafeDownCast(meta))
          {
            auto& spec = dgCell->GetCellSpec();
            auto connIt = perType.Roles.find("connectivity");
            if (connIt != perType.Roles.end())
            {
              if (auto connArr = materializeArray(connIt->second))
              {
                spec.Connectivity = connArr;
              }
            }
            // NodalGhostMarks is present only for distributed-memory grids.
            auto ghostIt = perType.Roles.find("ghost-node");
            if (ghostIt != perType.Roles.end())
            {
              if (auto ghostArr = materializeArray(ghostIt->second))
              {
                spec.NodalGhostMarks = ghostArr;
              }
            }
            spec.Offset = static_cast<vtkIdType>(perType.Offset);
            spec.Blanked = perType.Blanked;
            // Match shape name to vtkDGCell::Shape enum (e.g. "hexahedron").
            for (const auto& ct : cgEntry.CellTypes)
            {
              if (ct.CellTypeName == perType.CellTypeName)
              {
                spec.SourceShape = vtkDGCell::GetShapeEnum(vtkStringToken(ct.ShapeName));
                break;
              }
            }
          }
        }
      }
    }

    size_t index = this->DataSetsVec.size();
    this->DataSetsVec.push_back(cg);
    this->DataSetTokenMap[cgToken] = index;
  }
}

vtkSmartPointer<vtkPartitionedDataSet> VTKBuilder::BuildPartitionedDataSet(
  const std::vector<size_t>& partitionTokens)
{
  auto pds = vtkSmartPointer<vtkPartitionedDataSet>::New();
  unsigned int idx = 0;
  for (auto token : partitionTokens)
  {
    auto it = this->DataSetTokenMap.find(token);
    if (it != this->DataSetTokenMap.end())
    {
      pds->SetPartition(idx++, this->DataSetsVec[it->second]);
    }
  }
  return pds;
}

vtkSmartPointer<vtkPartitionedDataSet> VTKBuilder::GetResult()
{
  return this->BuildPartitionedDataSet(this->LegacyPartitionTokens());
}

namespace
{
/// Recursively add a schema assembly subtree under \c parent. Leaf
/// dataset-name references are resolved to PDC slot indices via
/// \c nameToSlot; unresolved names (e.g. filtered out by selection) are
/// silently skipped so the surrounding tree stays informative.
void AddAssemblySubtree(vtkDataAssembly* assembly,
                        int parent,
                        const OutputBuilder::AssemblyNode& node,
                        const std::unordered_map<std::string, unsigned int>& nameToSlot)
{
  int self = parent;
  if (!node.Name.empty())
  {
    // Names that flow through OutputBuilder::AssemblyNode were
    // validated against vtkDataAssembly's rules at schema-parse time
    // (DataSetReader's IsValidAssemblyNodeName), so they don't need
    // mangling here.
    self = assembly->AddNode(node.Name.c_str(), parent);
  }
  for (const auto& dsName : node.Datasets)
  {
    auto it = nameToSlot.find(dsName);
    if (it != nameToSlot.end())
    {
      assembly->AddDataSetIndex(self, it->second);
    }
  }
  for (const auto& child : node.Children)
  {
    AddAssemblySubtree(assembly, self, child, nameToSlot);
  }
}
}

vtkSmartPointer<vtkPartitionedDataSetCollection> VTKBuilder::GetResultCollection()
{
  auto pdc = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
  std::unordered_map<std::string, unsigned int> nameToSlot;
  const size_t nItems = this->GetNumberOfItems();
  for (unsigned int i = 0; i < nItems; ++i)
  {
    pdc->SetPartitionedDataSet(i, this->BuildPartitionedDataSet(this->GetItemPartitions(i)));
    const std::string& name = this->GetItemName(i);
    if (!name.empty())
    {
      nameToSlot[name] = i;
      pdc->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), name.c_str());
    }
  }

  // Always attach an assembly with a synthesized auto-names subtree
  // (one leaf per named item), plus any schema-declared subtrees as
  // siblings. The subtree's reserved name keeps it out of the user's
  // assembly namespace; schemas declaring a node with this name are
  // rejected at parse time, and the writer skips it on re-serialize.
  auto assembly = vtkSmartPointer<vtkDataAssembly>::New();
  assembly->Initialize();
  int namesNode = assembly->AddNode(fides::kAutoNamesAssemblySubtree);
  for (unsigned int i = 0; i < nItems; ++i)
  {
    const std::string& name = this->GetItemName(i);
    if (name.empty())
    {
      continue;
    }
    int leaf =
      assembly->AddNode(vtkDataAssembly::MakeValidNodeName(name.c_str()).c_str(), namesNode);
    assembly->AddDataSetIndex(leaf, i);
  }
  if (this->HasAssembly())
  {
    const auto& root = this->GetAssembly();
    if (!root.Name.empty())
    {
      assembly->SetRootNodeName(root.Name.c_str());
    }
    // The schema root's children become siblings of "names" under the
    // assembly root.
    for (const auto& child : root.Children)
    {
      AddAssemblySubtree(assembly, vtkDataAssembly::GetRootNode(), child, nameToSlot);
    }
    // Leaf datasets declared directly on the schema root attach to the root.
    for (const auto& dsName : root.Datasets)
    {
      auto it = nameToSlot.find(dsName);
      if (it != nameToSlot.end())
      {
        assembly->AddDataSetIndex(vtkDataAssembly::GetRootNode(), it->second);
      }
    }
  }
  pdc->SetDataAssembly(assembly);
  return pdc;
}

} // namespace fides
