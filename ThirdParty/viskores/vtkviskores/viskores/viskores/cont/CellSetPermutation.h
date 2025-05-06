//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_CellSetPermutation_h
#define viskores_cont_CellSetPermutation_h

#include <viskores/CellShape.h>
#include <viskores/CellTraits.h>
#include <viskores/Deprecated.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/CellSet.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/internal/ConnectivityExplicitInternals.h>
#include <viskores/cont/internal/ConvertNumComponentsToOffsetsTemplate.h>
#include <viskores/cont/internal/ReverseConnectivityBuilder.h>
#include <viskores/internal/ConnectivityStructuredInternals.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/exec/ConnectivityPermuted.h>

#ifndef VISKORES_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG
#define VISKORES_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG VISKORES_DEFAULT_STORAGE_TAG
#endif

namespace viskores
{
namespace cont
{

namespace internal
{

// To generate the reverse connectivity table with the
// ReverseConnectivityBuilder, we need a compact connectivity array that
// contains only the cell definitions from the permuted dataset, and an offsets
// array. These helpers are used to generate these arrays so that they can be
// converted in the reverse conn table.
class RConnTableHelpers
{
public:
  struct WriteNumIndices : public viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn cellset, FieldOutCell numIndices);
    using ExecutionSignature = void(PointCount, _2);
    using InputDomain = _1;

    VISKORES_EXEC void operator()(viskores::IdComponent pointCount,
                                  viskores::IdComponent& numIndices) const
    {
      numIndices = pointCount;
    }
  };

  struct WriteConnectivity : public viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn cellset, FieldOutCell connectivity);
    using ExecutionSignature = void(PointCount, PointIndices, _2);
    using InputDomain = _1;

    template <typename PointIndicesType, typename OutConnectivityType>
    VISKORES_EXEC void operator()(viskores::IdComponent pointCount,
                                  const PointIndicesType& pointIndices,
                                  OutConnectivityType& connectivity) const
    {
      for (viskores::IdComponent i = 0; i < pointCount; ++i)
      {
        connectivity[i] = pointIndices[i];
      }
    }
  };

  template <typename CellSetPermutationType>
  static VISKORES_CONT viskores::cont::ArrayHandle<viskores::IdComponent> GetNumIndicesArray(
    const CellSetPermutationType& cs,
    viskores::cont::DeviceAdapterId device)
  {
    viskores::cont::ArrayHandle<viskores::IdComponent> numIndices;
    viskores::cont::Invoker{ device }(WriteNumIndices{}, cs, numIndices);
    return numIndices;
  }

  template <typename NumIndicesStorageType>
  static VISKORES_CONT viskores::cont::ArrayHandle<viskores::Id> GetOffsetsArray(
    const viskores::cont::ArrayHandle<viskores::IdComponent, NumIndicesStorageType>& numIndices,
    viskores::Id& connectivityLength /* outparam */,
    viskores::cont::DeviceAdapterId)
  {
    return viskores::cont::internal::ConvertNumComponentsToOffsetsTemplate(numIndices,
                                                                           connectivityLength);
  }

  template <typename CellSetPermutationType, typename OffsetsStorageType>
  static viskores::cont::ArrayHandle<viskores::Id> GetConnectivityArray(
    const CellSetPermutationType& cs,
    const viskores::cont::ArrayHandle<viskores::Id, OffsetsStorageType>& offsets,
    viskores::Id connectivityLength,
    viskores::cont::DeviceAdapterId device)
  {
    viskores::cont::ArrayHandle<viskores::Id> connectivity;
    connectivity.Allocate(connectivityLength);
    auto connWrap = viskores::cont::make_ArrayHandleGroupVecVariable(connectivity, offsets);
    viskores::cont::Invoker{ device }(WriteConnectivity{}, cs, connWrap);
    return connectivity;
  }
};

// This holds the temporary input arrays for the ReverseConnectivityBuilder
// algorithm.
template <typename ConnectivityStorageTag = VISKORES_DEFAULT_STORAGE_TAG,
          typename OffsetsStorageTag = VISKORES_DEFAULT_STORAGE_TAG,
          typename NumIndicesStorageTag = VISKORES_DEFAULT_STORAGE_TAG>
struct RConnBuilderInputData
{
  using ConnectivityArrayType = viskores::cont::ArrayHandle<viskores::Id, ConnectivityStorageTag>;
  using OffsetsArrayType = viskores::cont::ArrayHandle<viskores::Id, OffsetsStorageTag>;
  using NumIndicesArrayType =
    viskores::cont::ArrayHandle<viskores::IdComponent, NumIndicesStorageTag>;

  ConnectivityArrayType Connectivity;
  OffsetsArrayType Offsets; // Includes the past-the-end offset.
  NumIndicesArrayType NumIndices;
};

// default for CellSetPermutations of any cell type
template <typename CellSetPermutationType>
class RConnBuilderInput
{
public:
  using ConnectivityArrays = viskores::cont::internal::RConnBuilderInputData<>;

  static ConnectivityArrays Get(const CellSetPermutationType& cellset,
                                viskores::cont::DeviceAdapterId device)
  {
    using Helper = RConnTableHelpers;
    ConnectivityArrays conn;
    viskores::Id connectivityLength = 0;

    conn.NumIndices = Helper::GetNumIndicesArray(cellset, device);
    conn.Offsets = Helper::GetOffsetsArray(conn.NumIndices, connectivityLength, device);
    conn.Connectivity =
      Helper::GetConnectivityArray(cellset, conn.Offsets, connectivityLength, device);

    return conn;
  }
};

// Specialization for CellSetExplicit/CellSetSingleType
template <typename InShapesST,
          typename InConnST,
          typename InOffsetsST,
          typename PermutationArrayHandleType>
class RConnBuilderInput<CellSetPermutation<CellSetExplicit<InShapesST, InConnST, InOffsetsST>,
                                           PermutationArrayHandleType>>
{
private:
  using BaseCellSetType = CellSetExplicit<InShapesST, InConnST, InOffsetsST>;
  using CellSetPermutationType = CellSetPermutation<BaseCellSetType, PermutationArrayHandleType>;

  using InShapesArrayType = typename BaseCellSetType::ShapesArrayType;
  using InNumIndicesArrayType = typename BaseCellSetType::NumIndicesArrayType;

  using ConnectivityStorageTag = viskores::cont::ArrayHandle<viskores::Id>::StorageTag;
  using OffsetsStorageTag = viskores::cont::ArrayHandle<viskores::Id>::StorageTag;
  using NumIndicesStorageTag =
    typename viskores::cont::ArrayHandlePermutation<PermutationArrayHandleType,
                                                    InNumIndicesArrayType>::StorageTag;


public:
  using ConnectivityArrays = viskores::cont::internal::
    RConnBuilderInputData<ConnectivityStorageTag, OffsetsStorageTag, NumIndicesStorageTag>;

  static ConnectivityArrays Get(const CellSetPermutationType& cellset,
                                viskores::cont::DeviceAdapterId device)
  {
    using Helper = RConnTableHelpers;

    static constexpr viskores::TopologyElementTagCell cell{};
    static constexpr viskores::TopologyElementTagPoint point{};

    auto fullCellSet = cellset.GetFullCellSet();

    viskores::Id connectivityLength = 0;
    ConnectivityArrays conn;

    fullCellSet.GetOffsetsArray(cell, point);

    // We can use the implicitly generated NumIndices array to save a bit of
    // memory:
    conn.NumIndices = viskores::cont::make_ArrayHandlePermutation(
      cellset.GetValidCellIds(), fullCellSet.GetNumIndicesArray(cell, point));

    // Need to generate the offsets from scratch so that they're ordered for the
    // lower-bounds binary searches in ReverseConnectivityBuilder.
    conn.Offsets = Helper::GetOffsetsArray(conn.NumIndices, connectivityLength, device);

    // Need to create a copy of this containing *only* the permuted cell defs,
    // in order, since the ReverseConnectivityBuilder will process every entry
    // in the connectivity array and we don't want the removed cells to be
    // included.
    conn.Connectivity =
      Helper::GetConnectivityArray(cellset, conn.Offsets, connectivityLength, device);

    return conn;
  }
};

// Specialization for CellSetStructured
template <viskores::IdComponent DIMENSION, typename PermutationArrayHandleType>
class RConnBuilderInput<
  CellSetPermutation<CellSetStructured<DIMENSION>, PermutationArrayHandleType>>
{
private:
  using CellSetPermutationType =
    CellSetPermutation<CellSetStructured<DIMENSION>, PermutationArrayHandleType>;

public:
  using ConnectivityArrays = viskores::cont::internal::RConnBuilderInputData<
    VISKORES_DEFAULT_STORAGE_TAG,
    typename viskores::cont::ArrayHandleCounting<viskores::Id>::StorageTag,
    typename viskores::cont::ArrayHandleConstant<viskores::IdComponent>::StorageTag>;

  static ConnectivityArrays Get(const CellSetPermutationType& cellset,
                                viskores::cont::DeviceAdapterId device)
  {
    viskores::Id numberOfCells = cellset.GetNumberOfCells();
    viskores::IdComponent numPointsInCell =
      viskores::internal::ConnectivityStructuredInternals<DIMENSION>::NUM_POINTS_IN_CELL;
    viskores::Id connectivityLength = numberOfCells * numPointsInCell;

    ConnectivityArrays conn;
    conn.NumIndices = make_ArrayHandleConstant(numPointsInCell, numberOfCells);
    conn.Offsets = ArrayHandleCounting<viskores::Id>(0, numPointsInCell, numberOfCells + 1);
    conn.Connectivity =
      RConnTableHelpers::GetConnectivityArray(cellset, conn.Offsets, connectivityLength, device);

    return conn;
  }
};

template <typename CellSetPermutationType>
struct CellSetPermutationTraits;

template <typename OriginalCellSet_, typename PermutationArrayHandleType_>
struct CellSetPermutationTraits<CellSetPermutation<OriginalCellSet_, PermutationArrayHandleType_>>
{
  using OriginalCellSet = OriginalCellSet_;
  using PermutationArrayHandleType = PermutationArrayHandleType_;
};

template <typename OriginalCellSet_,
          typename OriginalPermutationArrayHandleType,
          typename PermutationArrayHandleType_>
struct CellSetPermutationTraits<
  CellSetPermutation<CellSetPermutation<OriginalCellSet_, OriginalPermutationArrayHandleType>,
                     PermutationArrayHandleType_>>
{
  using PreviousCellSet = CellSetPermutation<OriginalCellSet_, OriginalPermutationArrayHandleType>;
  using PermutationArrayHandleType = viskores::cont::ArrayHandlePermutation<
    PermutationArrayHandleType_,
    typename CellSetPermutationTraits<PreviousCellSet>::PermutationArrayHandleType>;
  using OriginalCellSet = typename CellSetPermutationTraits<PreviousCellSet>::OriginalCellSet;
  using Superclass = CellSetPermutation<OriginalCellSet, PermutationArrayHandleType>;
};

template <typename VisitTopology,
          typename IncidentTopology,
          typename OriginalCellSetType,
          typename PermutationArrayHandleType>
struct CellSetPermutationConnectivityChooser;

template <typename OriginalCellSetType, typename PermutationArrayHandleType>
struct CellSetPermutationConnectivityChooser<viskores::TopologyElementTagCell,
                                             viskores::TopologyElementTagPoint,
                                             OriginalCellSetType,
                                             PermutationArrayHandleType>
{
  using ExecPortalType = typename PermutationArrayHandleType::ReadPortalType;
  using OrigExecObjectType =
    typename OriginalCellSetType::template ExecConnectivityType<viskores::TopologyElementTagCell,
                                                                viskores::TopologyElementTagPoint>;

  using ExecConnectivityType =
    viskores::exec::ConnectivityPermutedVisitCellsWithPoints<ExecPortalType, OrigExecObjectType>;
};

template <typename OriginalCellSetType, typename PermutationArrayHandleType>
struct CellSetPermutationConnectivityChooser<viskores::TopologyElementTagPoint,
                                             viskores::TopologyElementTagCell,
                                             OriginalCellSetType,
                                             PermutationArrayHandleType>
{
  using ConnectivityPortalType = typename viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType;
  using NumIndicesPortalType =
    typename viskores::cont::ArrayHandle<viskores::IdComponent>::ReadPortalType;
  using OffsetPortalType = typename viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType;

  using ExecConnectivityType =
    viskores::exec::ConnectivityPermutedVisitPointsWithCells<ConnectivityPortalType,
                                                             OffsetPortalType>;
};

} // internal

/// @brief Rearranges the cells of one cell set to create another cell set.
///
/// This restructuring of cells is not done by copying data to a new structure.
/// Rather, `CellSetPermutation` establishes a look-up from one cell structure to
/// another. Cells are permuted on the fly while algorithms are run.
///
/// A `CellSetPermutation` is established by providing a mapping array that for every
/// cell index provides the equivalent cell index in the cell set being permuted.
/// `CellSetPermutation` is most often used to mask out cells in a data set so that
/// algorithms will skip over those cells when running.
template <
  typename OriginalCellSetType_,
  typename PermutationArrayHandleType_ =
    viskores::cont::ArrayHandle<viskores::Id, VISKORES_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG>>
class CellSetPermutation : public CellSet
{
  VISKORES_IS_CELL_SET(OriginalCellSetType_);
  VISKORES_IS_ARRAY_HANDLE(PermutationArrayHandleType_);
  VISKORES_STATIC_ASSERT_MSG(
    (std::is_same<viskores::Id, typename PermutationArrayHandleType_::ValueType>::value),
    "Must use ArrayHandle with value type of Id for permutation array.");

public:
  using OriginalCellSetType = OriginalCellSetType_;
  using PermutationArrayHandleType = PermutationArrayHandleType_;

  /// @brief Create a `CellSetPermutation`.
  ///
  /// @param[in] validCellIds An array that defines the permutation. If index @a i
  ///   is value @a j, then the @a ith cell of this cell set will be the same as
  ///   the @a jth cell in the original @a cellset.
  /// @param[in] cellset The original cell set that this one is permuting.
  VISKORES_CONT CellSetPermutation(const PermutationArrayHandleType& validCellIds,
                                   const OriginalCellSetType& cellset)
    : CellSet()
    , ValidCellIds(validCellIds)
    , FullCellSet(cellset)
  {
  }

  VISKORES_CONT CellSetPermutation()
    : CellSet()
    , ValidCellIds()
    , FullCellSet()
  {
  }

  ~CellSetPermutation() override {}

  CellSetPermutation(const CellSetPermutation& src)
    : CellSet()
    , ValidCellIds(src.ValidCellIds)
    , FullCellSet(src.FullCellSet)
  {
  }


  CellSetPermutation& operator=(const CellSetPermutation& src)
  {
    this->ValidCellIds = src.ValidCellIds;
    this->FullCellSet = src.FullCellSet;
    return *this;
  }

  /// @brief Returns the original `CellSet` that this one is permuting.
  VISKORES_CONT
  const OriginalCellSetType& GetFullCellSet() const { return this->FullCellSet; }

  /// @brief Returns the array used to permute the cell indices.
  VISKORES_CONT
  const PermutationArrayHandleType& GetValidCellIds() const { return this->ValidCellIds; }

  VISKORES_CONT
  viskores::Id GetNumberOfCells() const override { return this->ValidCellIds.GetNumberOfValues(); }

  VISKORES_CONT
  viskores::Id GetNumberOfPoints() const override { return this->FullCellSet.GetNumberOfPoints(); }

  VISKORES_CONT
  viskores::Id GetNumberOfFaces() const override { return -1; }

  VISKORES_CONT
  viskores::Id GetNumberOfEdges() const override { return -1; }

  VISKORES_CONT
  void ReleaseResourcesExecution() override
  {
    this->ValidCellIds.ReleaseResourcesExecution();
    this->FullCellSet.ReleaseResourcesExecution();
    this->VisitPointsWithCells.ReleaseResourcesExecution();
  }

  VISKORES_CONT
  viskores::IdComponent GetNumberOfPointsInCell(viskores::Id cellIndex) const override
  {
    // Looping over GetNumberOfPointsInCell is a performance bug.
    return this->FullCellSet.GetNumberOfPointsInCell(
      this->ValidCellIds.ReadPortal().Get(cellIndex));
  }

  VISKORES_DEPRECATED(1.6,
                      "Calling GetCellShape(cellid) is a performance bug. Call ShapesReadPortal() "
                      "and loop over the Get.")
  viskores::UInt8 GetCellShape(viskores::Id id) const override
  {
    // Looping over GetCellShape is a performance bug.
    VISKORES_DEPRECATED_SUPPRESS_BEGIN
    return this->FullCellSet.GetCellShape(this->ValidCellIds.ReadPortal().Get(id));
    VISKORES_DEPRECATED_SUPPRESS_END
  }

  void GetCellPointIds(viskores::Id id, viskores::Id* ptids) const override
  {
    // Looping over GetCellPointsIdx is a performance bug.
    return this->FullCellSet.GetCellPointIds(this->ValidCellIds.ReadPortal().Get(id), ptids);
  }

  std::shared_ptr<CellSet> NewInstance() const override
  {
    return std::make_shared<CellSetPermutation>();
  }

  void DeepCopy(const CellSet* src) override
  {
    const auto* other = dynamic_cast<const CellSetPermutation*>(src);
    if (!other)
    {
      throw viskores::cont::ErrorBadType("CellSetPermutation::DeepCopy types don't match");
    }

    this->FullCellSet.DeepCopy(&(other->GetFullCellSet()));
    viskores::cont::ArrayCopy(other->GetValidCellIds(), this->ValidCellIds);
  }

  /// @brief Set the topology.
  ///
  /// @param[in] validCellIds An array that defines the permutation. If index @a i
  ///   is value @a j, then the @a ith cell of this cell set will be the same as
  ///   the @a jth cell in the original @a cellset.
  /// @param[in] cellset The original cell set that this one is permuting.
  VISKORES_CONT
  void Fill(const PermutationArrayHandleType& validCellIds, const OriginalCellSetType& cellset)
  {
    this->ValidCellIds = validCellIds;
    this->FullCellSet = cellset;
  }

  VISKORES_CONT viskores::Id GetSchedulingRange(viskores::TopologyElementTagCell) const
  {
    return this->ValidCellIds.GetNumberOfValues();
  }

  VISKORES_CONT viskores::Id GetSchedulingRange(viskores::TopologyElementTagPoint) const
  {
    return this->FullCellSet.GetNumberOfPoints();
  }

  template <typename VisitTopology, typename IncidentTopology>
  using ExecConnectivityType = typename internal::CellSetPermutationConnectivityChooser<
    VisitTopology,
    IncidentTopology,
    OriginalCellSetType,
    PermutationArrayHandleType>::ExecConnectivityType;

  /// @brief Prepares the data for a particular device and returns the execution object for it.
  ///
  /// @param device Specifies the device on which the cell set will ve available.
  /// @param visitTopology Specifies the "visit" topology element. This is the element
  /// that will be indexed in the resulting connectivity object. This is typically
  /// `viskores::TopologyElementTagPoint` or `viskores::TopologyElementTagCell`.
  /// @param incidentTopology Specifies the "incident" topology element. This is the element
  /// that will incident to the elements that are visited. This is typically
  /// `viskores::TopologyElementTagPoint` or `viskores::TopologyElementTagCell`.
  /// @param token Provides a `viskores::cont::Token` object that will define the span which
  /// the return execution object must be valid.
  ///
  /// @returns A connectivity object that can be used in the execution environment on the
  /// specified device.
  VISKORES_CONT
  ExecConnectivityType<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint>
  PrepareForInput(viskores::cont::DeviceAdapterId device,
                  viskores::TopologyElementTagCell visitTopology,
                  viskores::TopologyElementTagPoint incidentTopology,
                  viskores::cont::Token& token) const
  {
    using ConnectivityType =
      ExecConnectivityType<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint>;
    return ConnectivityType(
      this->ValidCellIds.PrepareForInput(device, token),
      this->FullCellSet.PrepareForInput(device, visitTopology, incidentTopology, token));
  }

  /// @copydoc PrepareForInput
  VISKORES_CONT
  ExecConnectivityType<viskores::TopologyElementTagPoint, viskores::TopologyElementTagCell>
  PrepareForInput(viskores::cont::DeviceAdapterId device,
                  viskores::TopologyElementTagPoint visitTopology,
                  viskores::TopologyElementTagCell incidentTopology,
                  viskores::cont::Token& token) const
  {
    (void)visitTopology;
    (void)incidentTopology;
    if (!this->VisitPointsWithCells.ElementsValid)
    {
      auto connTable = internal::RConnBuilderInput<CellSetPermutation>::Get(*this, device);
      internal::ComputeRConnTable(
        this->VisitPointsWithCells, connTable, this->GetNumberOfPoints(), device);
    }

    using ConnectivityType =
      ExecConnectivityType<viskores::TopologyElementTagPoint, viskores::TopologyElementTagCell>;
    return ConnectivityType(this->VisitPointsWithCells.Connectivity.PrepareForInput(device, token),
                            this->VisitPointsWithCells.Offsets.PrepareForInput(device, token));
  }

  VISKORES_CONT
  void PrintSummary(std::ostream& out) const override
  {
    out << "CellSetPermutation of: " << std::endl;
    this->FullCellSet.PrintSummary(out);
    out << "Permutation Array: " << std::endl;
    viskores::cont::printSummary_ArrayHandle(this->ValidCellIds, out);
  }

private:
  using VisitPointsWithCellsConnectivity = viskores::cont::internal::ConnectivityExplicitInternals<
    typename ArrayHandleConstant<viskores::UInt8>::StorageTag>;

  PermutationArrayHandleType ValidCellIds;
  OriginalCellSetType FullCellSet;
  mutable VisitPointsWithCellsConnectivity VisitPointsWithCells;
};

template <typename CellSetType,
          typename PermutationArrayHandleType1,
          typename PermutationArrayHandleType2>
class CellSetPermutation<CellSetPermutation<CellSetType, PermutationArrayHandleType1>,
                         PermutationArrayHandleType2>
  : public internal::CellSetPermutationTraits<
      CellSetPermutation<CellSetPermutation<CellSetType, PermutationArrayHandleType1>,
                         PermutationArrayHandleType2>>::Superclass
{
private:
  using Superclass = typename internal::CellSetPermutationTraits<CellSetPermutation>::Superclass;

public:
  VISKORES_CONT
  CellSetPermutation(const PermutationArrayHandleType2& validCellIds,
                     const CellSetPermutation<CellSetType, PermutationArrayHandleType1>& cellset)
    : Superclass(
        viskores::cont::make_ArrayHandlePermutation(validCellIds, cellset.GetValidCellIds()),
        cellset.GetFullCellSet())
  {
  }

  VISKORES_CONT
  CellSetPermutation()
    : Superclass()
  {
  }

  ~CellSetPermutation() override {}

  VISKORES_CONT
  void Fill(const PermutationArrayHandleType2& validCellIds,
            const CellSetPermutation<CellSetType, PermutationArrayHandleType1>& cellset)
  {
    this->ValidCellIds = make_ArrayHandlePermutation(validCellIds, cellset.GetValidCellIds());
    this->FullCellSet = cellset.GetFullCellSet();
  }

  std::shared_ptr<CellSet> NewInstance() const override
  {
    return std::make_shared<CellSetPermutation>();
  }
};

template <typename OriginalCellSet, typename PermutationArrayHandleType>
viskores::cont::CellSetPermutation<OriginalCellSet, PermutationArrayHandleType>
make_CellSetPermutation(const PermutationArrayHandleType& cellIndexMap,
                        const OriginalCellSet& cellSet)
{
  VISKORES_IS_CELL_SET(OriginalCellSet);
  VISKORES_IS_ARRAY_HANDLE(PermutationArrayHandleType);

  return viskores::cont::CellSetPermutation<OriginalCellSet, PermutationArrayHandleType>(
    cellIndexMap, cellSet);
}
}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <typename CSType, typename AHValidCellIds>
struct SerializableTypeString<viskores::cont::CellSetPermutation<CSType, AHValidCellIds>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "CS_Permutation<" + SerializableTypeString<CSType>::Get() + "," +
      SerializableTypeString<AHValidCellIds>::Get() + ">";
    return name;
  }
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename CSType, typename AHValidCellIds>
struct Serialization<viskores::cont::CellSetPermutation<CSType, AHValidCellIds>>
{
private:
  using Type = viskores::cont::CellSetPermutation<CSType, AHValidCellIds>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    viskoresdiy::save(bb, cs.GetFullCellSet());
    viskoresdiy::save(bb, cs.GetValidCellIds());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    CSType fullCS;
    viskoresdiy::load(bb, fullCS);
    AHValidCellIds validCellIds;
    viskoresdiy::load(bb, validCellIds);

    cs = make_CellSetPermutation(validCellIds, fullCS);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_CellSetPermutation_h
