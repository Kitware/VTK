//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

// Only compiled when FIDES_HAS_VISKORES is enabled (guarded by CMakeLists.txt).

#include <fides/internal/DataWrapHelper.h>
#include <fides/viskores/FidesDataSetWriterViskores.h>

#include <fides/FidesTypes.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace fides
{

namespace
{

/// Wrap a Viskores ArrayHandleBasic<T> as a RawArray, zero-copy.
///
/// The returned RawArray's shared_ptr deleter keeps a copy of the
/// ArrayHandleBasic alive, ensuring the underlying buffer remains valid
/// for as long as the RawArray exists. Viskores ArrayHandles are
/// reference-counted internally, so this is cheap and safe.
///
/// GPU array limitation: GetReadPointer() forces a device-to-host
/// transfer if the array currently lives on a device. The data is then
/// copied through the CPU on its way to ADIOS2. A future GPU put API
/// path could allow direct device-to-device transfers and avoid this
/// round-trip.
template <typename T>
RawArray WrapBasicArray(const viskores::cont::ArrayHandleBasic<T>& handle, int numComponents = 1)
{
  auto handlePtr = std::make_shared<viskores::cont::ArrayHandleBasic<T>>(handle);
  // GetReadPointer() syncs to host if necessary and returns a pointer
  // into the handle's reference-counted buffer.
  const T* readPtr = handlePtr->GetReadPointer();
  std::shared_ptr<void> buf(const_cast<T*>(readPtr), [handlePtr](void*) {});

  size_t totalElements = static_cast<size_t>(handlePtr->GetNumberOfValues());
  size_t numValues =
    (numComponents > 1) ? totalElements / static_cast<size_t>(numComponents) : totalElements;
  return RawArray(std::move(buf), numValues, numComponents, fides::GetDataType<T>());
}

/// Wrap a Viskores ArrayHandle<Vec<T,N>> as an NxN interleaved RawArray, zero-copy.
/// Vec<T,N> is layout-compatible with T[N], so the underlying buffer is
/// already a flat T* array — we just reinterpret_cast it.
template <typename T, viskores::IdComponent N>
RawArray WrapVecArray(const viskores::cont::ArrayHandle<viskores::Vec<T, N>>& handle)
{
  viskores::cont::ArrayHandleBasic<viskores::Vec<T, N>> basic(handle);
  auto basicPtr = std::make_shared<viskores::cont::ArrayHandleBasic<viskores::Vec<T, N>>>(basic);
  const viskores::Vec<T, N>* vecPtr = basicPtr->GetReadPointer();
  const T* flatPtr = reinterpret_cast<const T*>(vecPtr);
  std::shared_ptr<void> buf(const_cast<T*>(flatPtr), [basicPtr](void*) {});

  size_t numValues = static_cast<size_t>(basicPtr->GetNumberOfValues());
  return RawArray(std::move(buf), numValues, N, fides::GetDataType<T>());
}

/// Try to extract a flat-component view of an UnknownArrayHandle as type T,
/// then wrap it as a RawArray with the given number of components.
///
/// Fast paths (zero-copy) are tried first for the storage layouts that
/// dominate real-world usage:
///   - scalar ArrayHandleBasic<T>
///   - ArrayHandle<Vec<T,N>> for N=2,3 (Vec<T,N> is layout-compatible
///     with T[N], so the underlying buffer is already a flat T*).
///
/// Fallback: ExtractArrayFromComponents<T>() returns an
/// ArrayHandleRecombineVec<T> (a runtime-sized multi-component view) over
/// arbitrary storage (e.g. SOA). We ArrayCopy that into an
/// ArrayHandleRuntimeVec<T>, which materializes the components into a
/// single AOS-interleaved buffer of length numValues * numComponents, then
/// wrap that buffer. This path is not pointer-stable, but produces the
/// same AOS-interleaved layout as the fast paths and is correct for any
/// supported storage.
template <typename T>
RawArray WrapFlatArray(const viskores::cont::UnknownArrayHandle& handle, int numComponents)
{
  // Fast path: scalar ArrayHandleBasic<T>.
  if (numComponents == 1 && handle.IsType<viskores::cont::ArrayHandleBasic<T>>())
  {
    return WrapBasicArray<T>(handle.AsArrayHandle<viskores::cont::ArrayHandleBasic<T>>(), 1);
  }

  // Fast path: ArrayHandle<Vec<T,N>> for common N. Vec<T,N> is layout-
  // compatible with T[N], so the underlying buffer is already flat.
  if (numComponents == 2 && handle.IsType<viskores::cont::ArrayHandle<viskores::Vec<T, 2>>>())
  {
    return WrapVecArray<T, 2>(
      handle.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec<T, 2>>>());
  }
  if (numComponents == 3 && handle.IsType<viskores::cont::ArrayHandle<viskores::Vec<T, 3>>>())
  {
    return WrapVecArray<T, 3>(
      handle.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec<T, 3>>>());
  }

  // Fallback: arbitrary storage (e.g. SOA). Materialize via ArrayCopy.
  // This path is correct but allocates and copies. If you hit this warning
  // for a storage layout you use frequently, please report it so a
  // zero-copy fast path can be added in WrapFlatArray.
  //
  // The warning is gated behind the FIDES_DEBUG_WRITER environment variable
  // so it doesn't spam every step for workloads that legitimately use
  // non-fast-path storage. Set FIDES_DEBUG_WRITER=1 to see it.
  static const bool debugWriter = []() {
    const char* env = std::getenv("FIDES_DEBUG_WRITER");
    return env != nullptr && env[0] != '\0' && env[0] != '0';
  }();
  if (debugWriter)
  {
    std::cerr << "FidesDataSetWriter: falling back to ArrayCopy for Viskores storage '"
              << handle.GetStorageTypeName() << "' (component type '"
              << viskores::cont::TypeToString<T>() << "', " << numComponents
              << " components). This works but is not zero-copy." << std::endl;
  }

  // ExtractArrayFromComponents<T> returns an ArrayHandleRecombineVec<T>
  // whose ValueType is a runtime-sized Vec<T>. Copy it into an
  // ArrayHandleRuntimeVec<T>, whose component buffer is a single flat,
  // AOS-interleaved T array of length numValues * numComponents.
  auto recombine = handle.ExtractArrayFromComponents<T>(viskores::CopyFlag::Off);
  viskores::cont::ArrayHandleRuntimeVec<T> runtimeVec(numComponents);
  viskores::cont::ArrayCopy(recombine, runtimeVec);
  viskores::cont::ArrayHandleBasic<T> componentsArray = runtimeVec.GetComponentsArray();
  return WrapBasicArray<T>(componentsArray, numComponents);
}

/// Extract a Viskores field's data into a RawArray, dispatching over the
/// supported primitive types via FIDES_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG.
RawArray ExtractFieldArray(const viskores::cont::Field& field)
{
  const auto& handle = field.GetData();
  int numComponents = static_cast<int>(handle.GetNumberOfComponentsFlat());
  if (numComponents <= 0)
  {
    return RawArray();
  }

  RawArray result;
#define FIDES_TRY_EXTRACT(T)                                \
  if (!result.IsValid() && handle.IsBaseComponentType<T>()) \
  {                                                         \
    result = WrapFlatArray<T>(handle, numComponents);       \
  }
  FIDES_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(FIDES_TRY_EXTRACT)
#undef FIDES_TRY_EXTRACT

  if (!result.IsValid())
  {
    std::cerr << "FidesDataSetWriter: skipping field '" << field.GetName()
              << "': unsupported component type" << std::endl;
  }
  return result;
}

/// Extract a Viskores coordinate system as an Nx3 explicit RawArray.
RawArray ExtractExplicitCoords(const viskores::cont::CoordinateSystem& coordSys)
{
  auto handle = coordSys.GetData();

  // Common case: ArrayHandle<Vec<FloatDefault, 3>>
  using Vec3Default = viskores::Vec<viskores::FloatDefault, 3>;
  if (handle.IsType<viskores::cont::ArrayHandle<Vec3Default>>())
  {
    return WrapVecArray<viskores::FloatDefault, 3>(
      handle.AsArrayHandle<viskores::cont::ArrayHandle<Vec3Default>>());
  }
  if (handle.IsType<viskores::cont::ArrayHandle<viskores::Vec<float, 3>>>())
  {
    return WrapVecArray<float, 3>(
      handle.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec<float, 3>>>());
  }
  if (handle.IsType<viskores::cont::ArrayHandle<viskores::Vec<double, 3>>>())
  {
    return WrapVecArray<double, 3>(
      handle.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec<double, 3>>>());
  }

  // Fallback: try to extract through the flat-component path as 3-component
  // float / double.
  if (handle.IsBaseComponentType<viskores::FloatDefault>())
  {
    return WrapFlatArray<viskores::FloatDefault>(handle, 3);
  }
  if (handle.IsBaseComponentType<float>())
  {
    return WrapFlatArray<float>(handle, 3);
  }
  if (handle.IsBaseComponentType<double>())
  {
    return WrapFlatArray<double>(handle, 3);
  }

  throw std::runtime_error(
    "FidesDataSetWriter: unsupported Viskores coordinate system component type");
}

/// Extract one Viskores DataSet into a PartitionInfo.
PartitionInfo ExtractPartition(const viskores::cont::DataSet& ds,
                               const std::set<std::string>& fieldsToWrite)
{
  PartitionInfo pi;

  // --- Coordinates ---
  const auto& coordSys = ds.GetCoordinateSystem();
  auto coordsHandle = coordSys.GetData();

  using UniformCoordType = viskores::cont::ArrayHandleUniformPointCoordinates;
  using RectCoordType = viskores::cont::ArrayHandleCartesianProduct<
    viskores::cont::ArrayHandle<viskores::FloatDefault>,
    viskores::cont::ArrayHandle<viskores::FloatDefault>,
    viskores::cont::ArrayHandle<viskores::FloatDefault>>;

  bool isUniform = coordsHandle.IsType<UniformCoordType>();
  bool isRect = coordsHandle.IsType<RectCoordType>();

  if (isUniform)
  {
    pi.Coordinates = PartitionInfo::CoordType::Uniform;
    auto uc = coordsHandle.AsArrayHandle<UniformCoordType>();
    auto portal = uc.ReadPortal();
    auto origin = portal.GetOrigin();
    auto spacing = portal.GetSpacing();
    auto dims = portal.GetRange3();
    for (int j = 0; j < 3; j++)
    {
      pi.Origin[j] = origin[j];
      pi.Spacing[j] = spacing[j];
      pi.Dims[j] = static_cast<size_t>(dims[j]);
    }
  }
  else if (isRect)
  {
    pi.Coordinates = PartitionInfo::CoordType::Rectilinear;
    auto rc = coordsHandle.AsArrayHandle<RectCoordType>();
    pi.XCoords = WrapBasicArray<viskores::FloatDefault>(
      viskores::cont::ArrayHandleBasic<viskores::FloatDefault>(rc.GetFirstArray()));
    pi.YCoords = WrapBasicArray<viskores::FloatDefault>(
      viskores::cont::ArrayHandleBasic<viskores::FloatDefault>(rc.GetSecondArray()));
    pi.ZCoords = WrapBasicArray<viskores::FloatDefault>(
      viskores::cont::ArrayHandleBasic<viskores::FloatDefault>(rc.GetThirdArray()));
  }
  else
  {
    pi.Coordinates = PartitionInfo::CoordType::Explicit;
    pi.ExplicitCoords = ExtractExplicitCoords(coordSys);
  }

  // --- Cell set ---
  const auto& cellSet = ds.GetCellSet();

  using Structured3D = viskores::cont::CellSetStructured<3>;
  using Structured2D = viskores::cont::CellSetStructured<2>;
  using Structured1D = viskores::cont::CellSetStructured<1>;
  using SingleType = viskores::cont::CellSetSingleType<>;
  using Explicit = viskores::cont::CellSetExplicit<>;

  if (cellSet.IsType<Structured3D>())
  {
    pi.Cells = PartitionInfo::CellType::Structured;
    auto cs = cellSet.AsCellSet<Structured3D>();
    auto dims = cs.GetPointDimensions();
    pi.Dims[0] = static_cast<size_t>(dims[0]);
    pi.Dims[1] = static_cast<size_t>(dims[1]);
    pi.Dims[2] = static_cast<size_t>(dims[2]);
  }
  else if (cellSet.IsType<Structured2D>())
  {
    pi.Cells = PartitionInfo::CellType::Structured;
    auto cs = cellSet.AsCellSet<Structured2D>();
    auto dims = cs.GetPointDimensions();
    pi.Dims[0] = static_cast<size_t>(dims[0]);
    pi.Dims[1] = static_cast<size_t>(dims[1]);
    pi.Dims[2] = 1;
  }
  else if (cellSet.IsType<Structured1D>())
  {
    pi.Cells = PartitionInfo::CellType::Structured;
    auto cs = cellSet.AsCellSet<Structured1D>();
    auto dims = cs.GetPointDimensions();
    pi.Dims[0] = static_cast<size_t>(dims);
    pi.Dims[1] = 1;
    pi.Dims[2] = 1;
  }
  else if (cellSet.IsType<SingleType>())
  {
    pi.Cells = PartitionInfo::CellType::SingleType;
    auto cs = cellSet.AsCellSet<SingleType>();
    viskores::UInt8 shape = cs.GetCellShape(0);
    pi.SingleCellShape = internal::ConvertCellShapeFromViskores(shape);
    pi.VertsPerCell = cs.GetNumberOfPointsInCell(0);

    const auto& conn = cs.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                               viskores::TopologyElementTagPoint{});
    pi.SingleTypeConnectivity =
      WrapBasicArray<viskores::Id>(viskores::cont::ArrayHandleBasic<viskores::Id>(conn));
  }
  else if (cellSet.IsType<Explicit>())
  {
    pi.Cells = PartitionInfo::CellType::Explicit;
    auto cs = cellSet.AsCellSet<Explicit>();

    const auto& shapes =
      cs.GetShapesArray(viskores::TopologyElementTagCell{}, viskores::TopologyElementTagPoint{});
    const auto& offsets =
      cs.GetOffsetsArray(viskores::TopologyElementTagCell{}, viskores::TopologyElementTagPoint{});
    const auto& conn = cs.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                               viskores::TopologyElementTagPoint{});

    // Cell types: shapes is ArrayHandleBasic<UInt8>; CellShape values match
    // Viskores cell shape constants so this is a direct wrap.
    pi.ExplicitCellTypes =
      WrapBasicArray<viskores::UInt8>(viskores::cont::ArrayHandleBasic<viskores::UInt8>(shapes));

    // Num verts: derived from offsets (offsets[c+1] - offsets[c]).
    // TODO: As with the VTK path, the fides schema currently stores per-cell
    // vertex counts rather than offsets. Extending the schema to support
    // offsets directly would let us zero-copy the offsets array instead of
    // allocating and computing num_verts here.
    size_t nCells = static_cast<size_t>(cs.GetNumberOfCells());
    auto nVertsRaw = AllocateRawArray<int32_t>(nCells, 1);
    int32_t* nVertsBuf = nVertsRaw.GetWritePointer<int32_t>();
    auto offsetsPortal = offsets.ReadPortal();
    for (size_t c = 0; c < nCells; c++)
    {
      auto o0 = offsetsPortal.Get(static_cast<viskores::Id>(c));
      auto o1 = offsetsPortal.Get(static_cast<viskores::Id>(c + 1));
      nVertsBuf[c] = static_cast<int32_t>(o1 - o0);
    }
    pi.ExplicitNumVerts = nVertsRaw;

    pi.ExplicitConnectivity =
      WrapBasicArray<viskores::Id>(viskores::cont::ArrayHandleBasic<viskores::Id>(conn));
  }
  else
  {
    throw std::runtime_error("FidesDataSetWriter: unsupported Viskores cell set type");
  }

  // --- Fields ---
  bool filterFields = !fieldsToWrite.empty();
  for (viskores::IdComponent i = 0; i < ds.GetNumberOfFields(); i++)
  {
    const auto& field = ds.GetField(i);
    const std::string& name = field.GetName();

    // Coordinate systems are also exposed as fields; skip them here.
    if (ds.HasCoordinateSystem(name))
    {
      continue;
    }
    if (filterFields && fieldsToWrite.find(name) == fieldsToWrite.end())
    {
      continue;
    }

    auto assoc = field.GetAssociation();
    if (assoc != viskores::cont::Field::Association::Points &&
        assoc != viskores::cont::Field::Association::Cells)
    {
      continue;
    }

    RawArray data = ExtractFieldArray(field);
    if (!data.IsValid())
    {
      continue;
    }

    FieldInfo fi;
    fi.Name = name;
    fi.Association = (assoc == viskores::cont::Field::Association::Points)
      ? FieldAssociation::Points
      : FieldAssociation::Cells;
    fi.Data = std::move(data);
    pi.Fields.push_back(std::move(fi));
  }

  return pi;
}

} // end anon namespace

std::vector<PartitionInfo> ExtractViskoresPartitions(
  const viskores::cont::PartitionedDataSet& dataSets,
  const std::set<std::string>& fieldsToWrite)
{
  std::vector<PartitionInfo> result;

  viskores::Id nParts = dataSets.GetNumberOfPartitions();
  for (viskores::Id i = 0; i < nParts; i++)
  {
    const auto& ds = dataSets.GetPartition(i);
    if (ds.GetNumberOfPoints() == 0)
    {
      continue;
    }
    result.push_back(ExtractPartition(ds, fieldsToWrite));
  }
  return result;
}

} // namespace fides
