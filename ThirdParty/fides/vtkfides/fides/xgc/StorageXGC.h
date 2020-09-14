//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef fides_xgc_StorageXGC_h
#define fides_xgc_StorageXGC_h

#include <vtkm/internal/IndicesExtrude.h>

#include <vtkm/VecTraits.h>

#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>
#include <vtkm/cont/ArrayPortal.h>

namespace vtkm
{
namespace exec
{

template <typename PortalType>
struct VTKM_ALWAYS_EXPORT ArrayPortalXGCPlane
{
private:
  using Writable = vtkm::internal::PortalSupportsSets<PortalType>;

public:
  using ValueType = typename PortalType::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalXGCPlane()
    : NumberOfPlanesOwned(0)
    , NumberOfValuesPerPlane(0)
    , Is2DField(true){};

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalXGCPlane(const std::vector<PortalType>& p,
                      vtkm::Id numOfPlanes,
                      bool is2dField)
    : Portals(p)
    , NumberOfPlanesOwned(numOfPlanes)
    , Is2DField(is2dField)
  {
    VTKM_ASSERT(!this->Portals.empty());
    this->NumberOfValuesPerPlane = this->Portals[0].GetNumberOfValues();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->NumberOfValuesPerPlane * this->NumberOfPlanesOwned;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    vtkm::Id realIdx = index % this->NumberOfValuesPerPlane;
    vtkm::Id planeIdx = 0;
    if (!this->Is2DField)
    {
      planeIdx = index / this->NumberOfValuesPerPlane;
    }
    VTKM_ASSERT(static_cast<size_t>(planeIdx) < this->Portals.size());
    return this->Portals[static_cast<size_t>(planeIdx)].Get(realIdx);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id2 index) const
  {
    vtkm::Id realIdx = index[0];
    vtkm::Id planeIdx = 0;
    if (!this->Is2DField)
    {
      planeIdx = index[1];
    }
    VTKM_ASSERT(realIdx < this->NumberOfValuesPerPlane);
    VTKM_ASSERT(planeIdx < this->Portals.size());
    return this->Portals[planeIdx].Get(realIdx);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Vec<ValueType, 6> GetWedge(const IndicesExtrude& index) const
  {
    vtkm::Vec<ValueType, 6> result;
    result[0] = this->Portals[index.Planes[0]].Get(index.PointIds[0][0]);
    result[1] = this->Portals[index.Planes[0]].Get(index.PointIds[0][1]);
    result[2] = this->Portals[index.Planes[0]].Get(index.PointIds[0][2]);
    result[3] = this->Portals[index.Planes[1]].Get(index.PointIds[1][0]);
    result[4] = this->Portals[index.Planes[1]].Get(index.PointIds[1][1]);
    result[5] = this->Portals[index.Planes[1]].Get(index.PointIds[1][2]);

    return result;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    vtkm::Id realIdx = index % this->NumberOfValuesPerPlane;
    vtkm::Id planeIdx = 0;
    if (!this->Is2DField)
    {
      planeIdx = index / this->NumberOfValuesPerPlane;
    }
    VTKM_ASSERT(static_cast<size_t>(planeIdx) < this->Portals.size());
    this->Portals[static_cast<size_t>(planeIdx)].Set(realIdx, value);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT
  void Set(vtkm::Id2 index, const ValueType& value) const
  {
    vtkm::Id realIdx = index[0];
    vtkm::Id planeIdx = 0;
    if (!this->Is2DField)
    {
      planeIdx = index[1];
    }
    VTKM_ASSERT(realIdx < this->NumberOfValuesPerPlane);
    VTKM_ASSERT(planeIdx < this->Portals.size());
    this->Portals[planeIdx].Set(realIdx, value);
  }

  std::vector<PortalType> Portals;
  vtkm::Id NumberOfPlanesOwned;
  vtkm::Id NumberOfValuesPerPlane;
  bool Is2DField;
};
}
} // vtkm::exec

namespace vtkm
{
namespace cont
{
namespace internal
{

struct VTKM_ALWAYS_EXPORT StorageTagXGCPlane
{
};

template <typename T>
class VTKM_ALWAYS_EXPORT Storage<T, internal::StorageTagXGCPlane>
{
  using HandleType = vtkm::cont::ArrayHandle<T>;

public:
  using ValueType = T;

  using PortalType =
    vtkm::exec::ArrayPortalXGCPlane<typename HandleType::PortalControl>;

  using PortalConstType =
    vtkm::exec::ArrayPortalXGCPlane<typename HandleType::PortalConstControl>;

  VTKM_CONT
  Storage()
    : NumberOfPlanesOwned(0)
    , NumberOfValuesPerPlane(0)
    , Is2DField(true)
    , Valid(false)
    , Owner(false)
  {
  }

  VTKM_CONT
  Storage(vtkm::Id numberOfPlanes, vtkm::Id numberOfValuesPerPlane, bool is2dField)
    : NumberOfPlanesOwned(numberOfPlanes)
    , NumberOfValuesPerPlane(numberOfValuesPerPlane)
    , Is2DField(is2dField)
    , Valid(true)
    , Owner(true)
  {
    VTKM_ASSERT(this->NumberOfPlanesOwned > 0);
    VTKM_ASSERT(this->NumberOfValuesPerPlane > 0);
  }

  VTKM_CONT
  Storage(const HandleType& array, vtkm::Id numberOfPlanes,
      bool is2dField)
    : NumberOfPlanesOwned(numberOfPlanes)
    , Is2DField(is2dField)
    , Valid(true)
    , Owner(false)
  {
    VTKM_ASSERT(this->NumberOfPlanesOwned > 0);
    this->Arrays.push_back(array);
    VTKM_ASSERT(!this->Arrays.empty());
    this->NumberOfValuesPerPlane = this->Arrays[0].GetNumberOfValues();
  }

  VTKM_CONT
  Storage(const std::vector<HandleType>& arrays, vtkm::Id numberOfPlanes,
      bool is2dField)
    : Arrays(arrays)
    , NumberOfPlanesOwned(numberOfPlanes)
    , Is2DField(is2dField)
    , Valid(true)
    , Owner(false)
  {
    VTKM_ASSERT(this->NumberOfPlanesOwned > 0);
    VTKM_ASSERT(!this->Arrays.empty());
    this->NumberOfValuesPerPlane = this->Arrays[0].GetNumberOfValues();
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->Valid);
    VTKM_ASSERT(this->Arrays.size() >= 1);
    VTKM_ASSERT(this->Arrays[0].GetNumberOfValues() >= 0);
    std::vector<typename HandleType::PortalControl> portals;
    portals.reserve(this->Arrays.size());
    for (auto& array : this->Arrays)
    {
      portals.push_back(array.WritePortal());
    }
    return PortalType(portals,
      this->NumberOfPlanesOwned, this->Is2DField);
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    VTKM_ASSERT(this->Arrays.size() >= 1);
    VTKM_ASSERT(this->Arrays[0].GetNumberOfValues() >= 0);
    std::vector<typename HandleType::PortalConstControl> portals;
    portals.reserve(this->Arrays.size());
    for (auto& array : this->Arrays)
    {
      portals.push_back(array.ReadPortal());
    }
    return PortalConstType(portals,
      this->NumberOfPlanesOwned, this->Is2DField);
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Arrays[0].GetNumberOfValues() * this->NumberOfPlanesOwned;
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValuesPerPlane() const
  {
    VTKM_ASSERT(this->Valid);
    return this->NumberOfValuesPerPlane;
  }

  VTKM_CONT
  vtkm::Id GetNumberOfPlanes() const { return this->NumberOfPlanesOwned; }

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    VTKM_ASSERT(numberOfValues == this->NumberOfPlanesOwned * this->NumberOfValuesPerPlane);
    this->Arrays.resize(static_cast<size_t>(this->NumberOfPlanesOwned));
    for (auto& array : this->Arrays)
    {
      array.Allocate(this->NumberOfValuesPerPlane);
    }
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayPortalXGCPlane::Shrink() is not supported.");
  }

  VTKM_CONT
  void ReleaseResources()
  {
    // Only do this if we created the arrays
    if (this->Owner)
    {
      VTKM_ASSERT(this->Valid);
      for (auto& array : this->Arrays)
      {
        array.ReleaseResources();
      }
    }
  }

  VTKM_CONT
  bool IsField2D() const { return this->Is2DField; }

  std::vector<HandleType> Arrays;

private:
  vtkm::Id NumberOfPlanesOwned;
  vtkm::Id NumberOfValuesPerPlane;
  bool Is2DField;
  bool Valid;
  bool Owner;
};

template <typename T, typename Device>
class VTKM_ALWAYS_EXPORT ArrayTransfer<T, internal::StorageTagXGCPlane, Device>
{
  using HandleType = vtkm::cont::ArrayHandle<T>;
public:
  using ValueType = T;

private:
  using StorageTag = internal::StorageTagXGCPlane;
  using StorageType = vtkm::cont::internal::Storage<T, StorageTag>;

  using BasePortalExecution = typename HandleType::template ExecutionTypes<Device>::Portal;
  using BasePortalConstExecution = typename HandleType::template ExecutionTypes<Device>::PortalConst;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = vtkm::exec::ArrayPortalXGCPlane<BasePortalExecution>;
  using PortalConstExecution = vtkm::exec::ArrayPortalXGCPlane<BasePortalConstExecution>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : ControlData(storage)
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->ControlData->GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData), vtkm::cont::Token& token)
  {
    std::vector<BasePortalConstExecution> portals;
    for (auto& array : this->ControlData->Arrays)
    {
      portals.push_back(array.PrepareForInput(Device(), token));
    }

    return PortalConstExecution(portals,
      this->ControlData->GetNumberOfPlanes(),
      this->ControlData->IsField2D());
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool& vtkmNotUsed(updateData), vtkm::cont::Token& token)
  {
    std::vector<BasePortalExecution> portals;
    for (auto& array : this->ControlData->Arrays)
    {
      portals.push_back(array.PrepareForInPlace(Device(), token));
    }

    return PortalExecution(portals,
      this->ControlData->GetNumberOfPlanes(),
      this->ControlData->IsField2D());
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues), vtkm::cont::Token& token)
  {
    std::vector<BasePortalExecution> portals;
    for (auto& array : this->ControlData->Arrays)
    {
      portals.push_back(array.PrepareForOutput(this->ControlData->GetNumberOfValuesPerPlane(), Device(), token));
    }

    return PortalExecution(portals,
      this->ControlData->GetNumberOfPlanes(),
      this->ControlData->IsField2D());
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayPortalXGCPlane read only. Cannot shrink.");
  }

  VTKM_CONT
  void ReleaseResources()
  {
    this->ControlData->ReleaseResources();
  }

private:
  StorageType* ControlData;
};
}
}
} // vtkm::cont::internal

namespace vtkm
{
namespace exec
{

template <typename PortalType>
struct VTKM_ALWAYS_EXPORT ArrayPortalXGC
{
  using ValueType = vtkm::Vec<typename PortalType::ValueType, 3>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalXGC()
    : Portal()
    , NumberOfValues(0)
    , NumberOfPlanes(0)
    , NumberOfPlanesOwned(0)
    , PlaneStartId(0)
    , UseCylindrical(false){};

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalXGC(const PortalType& p,
                     vtkm::Id numOfValues,
                     vtkm::Id numOfPlanes,
                     vtkm::Id numOfPlanesOwned,
                     vtkm::Id planeStartId,
                     bool cylindrical = false)
    : Portal(p)
    , NumberOfValues(numOfValues)
    , NumberOfPlanes(numOfPlanes)
    , NumberOfPlanesOwned(numOfPlanesOwned)
    , PlaneStartId(planeStartId)
    , UseCylindrical(cylindrical)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return ((NumberOfValues / 2) * static_cast<vtkm::Id>(NumberOfPlanesOwned));
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id2 index) const;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Vec<ValueType, 6> GetWedge(const IndicesExtrude& index) const;

  PortalType Portal;
  vtkm::Id NumberOfValues;
  vtkm::Id NumberOfPlanes;
  vtkm::Id NumberOfPlanesOwned;
  vtkm::Id PlaneStartId;
  bool UseCylindrical;
};

template <typename PortalType>
typename ArrayPortalXGC<PortalType>::ValueType
ArrayPortalXGC<PortalType>::ArrayPortalXGC::Get(vtkm::Id index) const
{
  using CompType = typename ValueType::ComponentType;

  const vtkm::Id realIdx = (index * 2) % this->NumberOfValues;
  const vtkm::Id whichPlane = (index * 2) / this->NumberOfValues + this->PlaneStartId;
  const auto phi = static_cast<CompType>(whichPlane * (vtkm::TwoPi() / this->NumberOfPlanes));

  auto r = this->Portal.Get(realIdx);
  auto z = this->Portal.Get(realIdx + 1);
  if (this->UseCylindrical)
  {
    return ValueType(r, phi, z);
  }
  else
  {
    return ValueType(static_cast<CompType>(r * vtkm::Cos(phi)),
      static_cast<CompType>(r * vtkm::Sin(phi)), z);
  }
}

template <typename PortalType>
typename ArrayPortalXGC<PortalType>::ValueType
ArrayPortalXGC<PortalType>::ArrayPortalXGC::Get(vtkm::Id2 index) const
{
  using CompType = typename ValueType::ComponentType;

  const vtkm::Id realIdx = (index[0] * 2);
  const vtkm::Id whichPlane = index[1];
  const auto phi = static_cast<CompType>(whichPlane * (vtkm::TwoPi() / this->NumberOfPlanes));

  auto r = this->Portal.Get(realIdx);
  auto z = this->Portal.Get(realIdx + 1);
  if (this->UseCylindrical)
  {
    return ValueType(r, phi, z);
  }
  else
  {
    return ValueType(r * vtkm::Cos(phi), r * vtkm::Sin(phi), z);
  }
}

template <typename PortalType>
vtkm::Vec<typename ArrayPortalXGC<PortalType>::ValueType, 6>
ArrayPortalXGC<PortalType>::ArrayPortalXGC::GetWedge(const IndicesExtrude& index) const
{
  using CompType = typename ValueType::ComponentType;

  vtkm::Vec<ValueType, 6> result;
  for (int j = 0; j < 2; ++j)
  {
    const auto phi =
      static_cast<CompType>(index.Planes[j] * (vtkm::TwoPi() / this->NumberOfPlanes));
    for (int i = 0; i < 3; ++i)
    {
      const vtkm::Id realIdx = index.PointIds[j][i] * 2;
      auto r = this->Portal.Get(realIdx);
      auto z = this->Portal.Get(realIdx + 1);
      result[3 * j + i] = this->UseCylindrical
        ? ValueType(r, phi, z)
        : ValueType(r * vtkm::Cos(phi), r * vtkm::Sin(phi), z);
    }
  }

  return result;
}
}
}

namespace vtkm
{
namespace cont
{
namespace internal
{
struct VTKM_ALWAYS_EXPORT StorageTagXGC
{
};

template <typename T>
class Storage<T, internal::StorageTagXGC>
{
  using BaseT = typename VecTraits<T>::BaseComponentType;
  using HandleType = vtkm::cont::ArrayHandle<BaseT>;
  using TPortalType = typename HandleType::PortalConstControl;

public:
  using ValueType = T;

  using PortalConstType = exec::ArrayPortalXGC<TPortalType>;

  // Note that this array is read only, so you really should only be getting the const
  // version of the portal. If you actually try to write to this portal, you will
  // get an error.
  using PortalType = PortalConstType;

  Storage()
    : Array()
    , NumberOfPlanes(0)
    , NumberOfPlanesOwned(0)
    , PlaneStartId(0)
  {
  }

  // Create with externally managed memory
  Storage(const BaseT* array, vtkm::Id arrayLength, vtkm::Id numberOfPlanes,
      vtkm::Id numberOfPlanesOwned, vtkm::Id planeStartId, bool cylindrical)
    : Array(vtkm::cont::make_ArrayHandle(array, arrayLength))
    , NumberOfPlanes(numberOfPlanes)
    , NumberOfPlanesOwned(numberOfPlanesOwned)
    , PlaneStartId(planeStartId)
    , UseCylindrical(cylindrical)
  {
    VTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
  }

  Storage(const HandleType& array, vtkm::Id numberOfPlanes,
      vtkm::Id numberOfPlanesOwned, vtkm::Id planeStartId, bool cylindrical)
    : Array(array)
    , NumberOfPlanes(numberOfPlanes)
    , NumberOfPlanesOwned(numberOfPlanesOwned)
    , PlaneStartId(planeStartId)
    , UseCylindrical(cylindrical)
  {
    VTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
  }

  PortalType GetPortal()
  {
    throw vtkm::cont::ErrorBadType(
      "XGC ArrayHandles are read only. Cannot get writable portal.");
  }

  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
    return PortalConstType(this->Array.ReadPortal(),
                           this->Array.GetNumberOfValues(),
                           this->NumberOfPlanes,
                           this->NumberOfPlanesOwned,
                           this->PlaneStartId,
                           this->UseCylindrical);
  }

  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Array.GetNumberOfValues() >= 0);
    return (this->Array.GetNumberOfValues() / 2) * static_cast<vtkm::Id>(this->NumberOfPlanesOwned);
  }

  vtkm::Id GetLength() const { return this->Array.GetNumberOfValues(); }

  vtkm::Id GetNumberOfPlanes() const { return NumberOfPlanes; }

  vtkm::Id GetNumberOfPlanesOwned() const { return this->NumberOfPlanesOwned; }

  vtkm::Id GetPlaneStartId() const { return this->PlaneStartId; }

  bool GetUseCylindrical() const { return UseCylindrical; }
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("StorageTagXGC is read only. It cannot be allocated.");
  }

  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("StoraageTagXGC is read only. It cannot shrink.");
  }

  void ReleaseResources()
  {
    // This request is ignored since we don't own the memory that was past
    // to us
  }


  vtkm::cont::ArrayHandle<BaseT> Array;

private:
  vtkm::Id NumberOfPlanes;
  vtkm::Id NumberOfPlanesOwned;
  vtkm::Id PlaneStartId;
  bool UseCylindrical;
};

template <typename T, typename Device>
class VTKM_ALWAYS_EXPORT ArrayTransfer<T, internal::StorageTagXGC, Device>
{
  using BaseT = typename VecTraits<T>::BaseComponentType;
  using TPortalType = decltype(vtkm::cont::ArrayHandle<BaseT>{}.PrepareForInput(Device{}, std::declval<vtkm::cont::Token&>()));

public:
  using ValueType = T;
  using StorageType = vtkm::cont::internal::Storage<T, internal::StorageTagXGC>;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  //meant to be an invalid writeable execution portal
  using PortalExecution = typename StorageType::PortalType;

  using PortalConstExecution = vtkm::exec::ArrayPortalXGC<TPortalType>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : ControlData(storage)
  {
  }
  vtkm::Id GetNumberOfValues() const { return this->ControlData->GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData), vtkm::cont::Token& token)
  {
    return PortalConstExecution(
      this->ControlData->Array.PrepareForInput(Device(), token),
      this->ControlData->Array.GetNumberOfValues(),
      this->ControlData->GetNumberOfPlanes(),
      this->ControlData->GetNumberOfPlanesOwned(),
      this->ControlData->GetPlaneStartId(),
      this->ControlData->GetUseCylindrical());
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool& vtkmNotUsed(updateData), vtkm::cont::Token& vtkmNotUsed(token))
  {
    throw vtkm::cont::ErrorBadType("StorageXGC read only. "
                                   "Cannot be used for in-place operations.");
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues), vtkm::cont::Token& vtkmNotUsed(token))
  {
    throw vtkm::cont::ErrorBadType("StorageXGC read only. Cannot be used as output.");
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    throw vtkm::cont::ErrorInternal(
      "ArrayHandleExrPointCoordinates read only. "
      "There should be no occurance of the ArrayHandle trying to pull "
      "data from the execution environment.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("StorageXGC read only. Cannot shrink.");
  }

  VTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since we don't own the memory that was past
    // to us
  }

private:
  const StorageType* const ControlData;
};
}
}
}

#endif
