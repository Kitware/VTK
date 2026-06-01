//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/DataWrapHelper.h>
#include <fides/viskores/ViskoresBuilder.h>

#include <viskores/CellShape.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/CoordinateSystem.h>

#include <stdexcept>

namespace fides
{

template <typename T>
viskores::cont::UnknownArrayHandle ViskoresBuilder::MakeViskoresArrayHandle(const RawArray& raw)
{
  size_t totalElements = raw.NumValues * static_cast<size_t>(raw.NumComponents);

  // Viskores requires a mutable T*, const_cast is the only way to achieve it
  T* ptr = const_cast<T*>(raw.GetPointer<T>());

  // Allocate a copy of the shared_ptr itself on the heap (not the heavy data),
  // so that we increment the reference count of the underlying data buffer.
  auto* owner = new std::shared_ptr<void>(raw.Data);
  auto flatAH = viskores::cont::ArrayHandleBasic<T>(
    ptr,
    // Hide the heap-allocated shared_ptr behind a void* so Viskores can hold it
    static_cast<void*>(owner),
    static_cast<viskores::Id>(totalElements),
    // When Viskores is done with the array handle, it calls this deleter. We cast
    // back to shared_ptr<void>* and delete it, decrementing the reference count of
    // the underlying buffer.
    [](void* container) { delete static_cast<std::shared_ptr<void>*>(container); });

  if (raw.NumComponents > 1)
  {
    return viskores::cont::make_ArrayHandleRuntimeVec(raw.NumComponents, flatAH);
  }
  return flatAH;
}

viskores::cont::UnknownArrayHandle ViskoresBuilder::MakeHandle(const RawArray& raw)
{
  switch (raw.Type)
  {
    case DataType::Float32:
      return ViskoresBuilder::MakeViskoresArrayHandle<float>(raw);
    case DataType::Float64:
      return ViskoresBuilder::MakeViskoresArrayHandle<double>(raw);
    case DataType::Int8:
      return ViskoresBuilder::MakeViskoresArrayHandle<int8_t>(raw);
    case DataType::Int16:
      return ViskoresBuilder::MakeViskoresArrayHandle<int16_t>(raw);
    case DataType::Int32:
      return ViskoresBuilder::MakeViskoresArrayHandle<int32_t>(raw);
    case DataType::Int64:
      return ViskoresBuilder::MakeViskoresArrayHandle<int64_t>(raw);
    case DataType::UInt8:
      return ViskoresBuilder::MakeViskoresArrayHandle<uint8_t>(raw);
    case DataType::UInt16:
      return ViskoresBuilder::MakeViskoresArrayHandle<uint16_t>(raw);
    case DataType::UInt32:
      return ViskoresBuilder::MakeViskoresArrayHandle<uint32_t>(raw);
    case DataType::UInt64:
      return ViskoresBuilder::MakeViskoresArrayHandle<uint64_t>(raw);
    default:
      throw std::runtime_error("ViskoresBuilder::MakeHandle: unknown DataType");
  }
}

void ViskoresBuilder::Reset()
{
  OutputBuilder::Reset();
  this->DataSetsVec.clear();
}

// --- Finalize: build all Viskores objects from populated data ---

void ViskoresBuilder::Finalize()
{
  // Step 1: Create DataSets
  this->DataSetsVec.clear();
  this->DataSetTokenMap.clear();
  for (auto token : this->DataSetTokens)
  {
    size_t index = this->DataSetsVec.size();
    this->DataSetsVec.emplace_back();
    this->DataSetTokenMap[token] = index;
  }

  // Step 2: Apply coordinate systems
  for (auto& dc : this->DeferredCoordSystems)
  {
    auto dsIt = this->DataSetTokenMap.find(dc.DataSetToken);
    if (dsIt == this->DataSetTokenMap.end())
    {
      continue;
    }
    auto& ds = this->DataSetsVec[dsIt->second];

    auto coordIt = this->StoredCoords.find(dc.CoordToken);
    if (coordIt != this->StoredCoords.end())
    {
      auto& coord = coordIt->second;
      if (coord.EntryType == CoordEntry::Type::Uniform)
      {
        auto ah = viskores::cont::ArrayHandleUniformPointCoordinates(
          viskores::Id3(coord.Dims[0], coord.Dims[1], coord.Dims[2]),
          viskores::Vec3f(static_cast<viskores::FloatDefault>(coord.Origin[0]),
                          static_cast<viskores::FloatDefault>(coord.Origin[1]),
                          static_cast<viskores::FloatDefault>(coord.Origin[2])),
          viskores::Vec3f(static_cast<viskores::FloatDefault>(coord.Spacing[0]),
                          static_cast<viskores::FloatDefault>(coord.Spacing[1]),
                          static_cast<viskores::FloatDefault>(coord.Spacing[2])));
        ds.AddCoordinateSystem(viskores::cont::CoordinateSystem("coords", ah));
      }
      else if (coord.EntryType == CoordEntry::Type::Rectilinear)
      {
        auto xIt = this->StoredArrays.find(coord.XToken);
        auto yIt = this->StoredArrays.find(coord.YToken);
        auto zIt = this->StoredArrays.find(coord.ZToken);
        if (xIt == this->StoredArrays.end() || yIt == this->StoredArrays.end() ||
            zIt == this->StoredArrays.end())
        {
          throw std::runtime_error("ViskoresBuilder::Finalize: invalid rectilinear coord tokens");
        }
        viskores::cont::UnknownArrayHandle result;
        if (xIt->second.Type == DataType::Float32)
        {
          auto xAH = ViskoresBuilder::MakeHandle(xIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<float>>();
          auto yAH = ViskoresBuilder::MakeHandle(yIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<float>>();
          auto zAH = ViskoresBuilder::MakeHandle(zIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<float>>();
          result = viskores::cont::make_ArrayHandleCartesianProduct(xAH, yAH, zAH);
        }
        else
        {
          auto xAH = ViskoresBuilder::MakeHandle(xIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<double>>();
          auto yAH = ViskoresBuilder::MakeHandle(yIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<double>>();
          auto zAH = ViskoresBuilder::MakeHandle(zIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<double>>();
          result = viskores::cont::make_ArrayHandleCartesianProduct(xAH, yAH, zAH);
        }
        ds.AddCoordinateSystem(viskores::cont::CoordinateSystem("coords", result));
      }
      else if (coord.EntryType == CoordEntry::Type::Composite)
      {
        auto xIt = this->StoredArrays.find(coord.XToken);
        auto yIt = this->StoredArrays.find(coord.YToken);
        auto zIt = this->StoredArrays.find(coord.ZToken);
        if (xIt == this->StoredArrays.end() || yIt == this->StoredArrays.end() ||
            zIt == this->StoredArrays.end())
        {
          throw std::runtime_error("ViskoresBuilder::Finalize: invalid composite coord tokens");
        }
        viskores::cont::UnknownArrayHandle result;
        if (xIt->second.Type == DataType::Float32)
        {
          auto xAH = ViskoresBuilder::MakeHandle(xIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<float>>();
          auto yAH = ViskoresBuilder::MakeHandle(yIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<float>>();
          auto zAH = ViskoresBuilder::MakeHandle(zIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<float>>();
          result = viskores::cont::ArrayHandleSOA<viskores::Vec3f_32>{ xAH, yAH, zAH };
        }
        else
        {
          auto xAH = ViskoresBuilder::MakeHandle(xIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<double>>();
          auto yAH = ViskoresBuilder::MakeHandle(yIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<double>>();
          auto zAH = ViskoresBuilder::MakeHandle(zIt->second)
                       .AsArrayHandle<viskores::cont::ArrayHandle<double>>();
          result = viskores::cont::ArrayHandleSOA<viskores::Vec3f_64>{ xAH, yAH, zAH };
        }
        ds.AddCoordinateSystem(viskores::cont::CoordinateSystem("coords", result));
      }
      continue;
    }

    // Plain array token as coordinate
    auto arrIt = this->StoredArrays.find(dc.CoordToken);
    if (arrIt != this->StoredArrays.end())
    {
      ds.AddCoordinateSystem(
        viskores::cont::CoordinateSystem("coords", ViskoresBuilder::MakeHandle(arrIt->second)));
    }
  }

  // Step 3: Cell sets are created in PostRead where the data model objects
  // (CellSetSingleType, CellSetExplicit, CellSetStructured, CellSetXGC, etc.)
  // have full context (nPoints, connectivity data) to build them correctly.

  // Step 4: Apply fields
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

    auto viskoresAssoc = fides::internal::ConvertToViskoresFieldAssociation(df.Assoc);
    auto handle = ViskoresBuilder::MakeHandle(arrIt->second);
    this->DataSetsVec[dsIt->second].AddField(viskores::cont::Field(df.Name, viskoresAssoc, handle));
  }
}

std::vector<viskores::cont::DataSet>& ViskoresBuilder::GetDataSets()
{
  return this->DataSetsVec;
}

viskores::cont::PartitionedDataSet ViskoresBuilder::GetResult()
{
  std::vector<viskores::cont::DataSet> partitions;
  for (auto token : this->PartitionTokens)
  {
    auto it = this->DataSetTokenMap.find(token);
    if (it != this->DataSetTokenMap.end())
    {
      partitions.push_back(this->DataSetsVec[it->second]);
    }
  }
  return viskores::cont::PartitionedDataSet(partitions);
}

} // namespace fides
