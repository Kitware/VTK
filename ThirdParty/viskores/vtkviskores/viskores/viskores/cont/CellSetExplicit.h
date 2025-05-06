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
#ifndef viskores_cont_CellSetExplicit_h
#define viskores_cont_CellSetExplicit_h

#include <viskores/CellShape.h>
#include <viskores/TopologyElementTag.h>
#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleOffsetsToNumComponents.h>
#include <viskores/cont/CellSet.h>
#include <viskores/cont/UnknownArrayHandle.h>
#include <viskores/cont/internal/ConnectivityExplicitInternals.h>
#include <viskores/exec/ConnectivityExplicit.h>

#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{

namespace detail
{

template <typename CellSetType, typename VisitTopology, typename IncidentTopology>
struct CellSetExplicitConnectivityChooser
{
  using ConnectivityType = viskores::cont::internal::ConnectivityExplicitInternals<>;
};

// The connectivity generally used for the visit-points-with-cells connectivity.
// This type of connectivity does not have variable shape types, and since it is
// never really provided externally we can use the defaults for the other arrays.
using DefaultVisitPointsWithCellsConnectivityExplicit =
  viskores::cont::internal::ConnectivityExplicitInternals<
    typename ArrayHandleConstant<viskores::UInt8>::StorageTag>;

VISKORES_CONT_EXPORT void BuildReverseConnectivity(
  const viskores::cont::UnknownArrayHandle& connections,
  const viskores::cont::UnknownArrayHandle& offsets,
  viskores::Id numberOfPoints,
  viskores::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit& visitPointsWithCells,
  viskores::cont::DeviceAdapterId device);

} // namespace detail

#ifndef VISKORES_DEFAULT_SHAPES_STORAGE_TAG
#define VISKORES_DEFAULT_SHAPES_STORAGE_TAG VISKORES_DEFAULT_STORAGE_TAG
#endif

#ifndef VISKORES_DEFAULT_CONNECTIVITY_STORAGE_TAG
#define VISKORES_DEFAULT_CONNECTIVITY_STORAGE_TAG VISKORES_DEFAULT_STORAGE_TAG
#endif

#ifndef VISKORES_DEFAULT_OFFSETS_STORAGE_TAG
#define VISKORES_DEFAULT_OFFSETS_STORAGE_TAG VISKORES_DEFAULT_STORAGE_TAG
#endif

/// @brief Defines an irregular collection of cells.
///
/// The cells can be of different types and connected in arbitrary ways.
/// This is done by explicitly providing for each cell a sequence of points that defines the cell.
template <typename ShapesStorageTag = VISKORES_DEFAULT_SHAPES_STORAGE_TAG,
          typename ConnectivityStorageTag = VISKORES_DEFAULT_CONNECTIVITY_STORAGE_TAG,
          typename OffsetsStorageTag = VISKORES_DEFAULT_OFFSETS_STORAGE_TAG>
class VISKORES_ALWAYS_EXPORT CellSetExplicit : public CellSet
{
  using Thisclass = CellSetExplicit<ShapesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>;

  template <typename VisitTopology, typename IncidentTopology>
  struct ConnectivityChooser
  {
  private:
    using Chooser = typename detail::
      CellSetExplicitConnectivityChooser<Thisclass, VisitTopology, IncidentTopology>;

  public:
    using ConnectivityType = typename Chooser::ConnectivityType;
    using ShapesArrayType = typename ConnectivityType::ShapesArrayType;
    using ConnectivityArrayType = typename ConnectivityType::ConnectivityArrayType;
    using OffsetsArrayType = typename ConnectivityType::OffsetsArrayType;

    using NumIndicesArrayType = viskores::cont::ArrayHandleOffsetsToNumComponents<OffsetsArrayType>;

    using ExecConnectivityType =
      viskores::exec::ConnectivityExplicit<typename ShapesArrayType::ReadPortalType,
                                           typename ConnectivityArrayType::ReadPortalType,
                                           typename OffsetsArrayType::ReadPortalType>;
  };

  using ConnTypes =
    ConnectivityChooser<viskores::TopologyElementTagCell, viskores::TopologyElementTagPoint>;
  using RConnTypes =
    ConnectivityChooser<viskores::TopologyElementTagPoint, viskores::TopologyElementTagCell>;

  using CellPointIdsType = typename ConnTypes::ConnectivityType;
  using PointCellIdsType = typename RConnTypes::ConnectivityType;

public:
  using SchedulingRangeType = viskores::Id;

  using ShapesArrayType = typename CellPointIdsType::ShapesArrayType;
  using ConnectivityArrayType = typename CellPointIdsType::ConnectivityArrayType;
  using OffsetsArrayType = typename CellPointIdsType::OffsetsArrayType;
  using NumIndicesArrayType = typename ConnTypes::NumIndicesArrayType;

  VISKORES_CONT CellSetExplicit();
  VISKORES_CONT CellSetExplicit(const Thisclass& src);
  VISKORES_CONT CellSetExplicit(Thisclass&& src) noexcept;

  VISKORES_CONT Thisclass& operator=(const Thisclass& src);
  VISKORES_CONT Thisclass& operator=(Thisclass&& src) noexcept;

  VISKORES_CONT ~CellSetExplicit() override;

  VISKORES_CONT viskores::Id GetNumberOfCells() const override;
  VISKORES_CONT viskores::Id GetNumberOfPoints() const override;
  VISKORES_CONT viskores::Id GetNumberOfFaces() const override;
  VISKORES_CONT viskores::Id GetNumberOfEdges() const override;
  VISKORES_CONT void PrintSummary(std::ostream& out) const override;

  VISKORES_CONT void ReleaseResourcesExecution() override;

  VISKORES_CONT std::shared_ptr<CellSet> NewInstance() const override;
  VISKORES_CONT void DeepCopy(const CellSet* src) override;

  VISKORES_CONT viskores::Id GetSchedulingRange(viskores::TopologyElementTagCell) const;
  VISKORES_CONT viskores::Id GetSchedulingRange(viskores::TopologyElementTagPoint) const;

  VISKORES_CONT viskores::IdComponent GetNumberOfPointsInCell(viskores::Id cellid) const override;
  VISKORES_CONT void GetCellPointIds(viskores::Id id, viskores::Id* ptids) const override;

  /// Returns an array portal that can be used to get the shape id of each cell.
  /// Using the array portal returned from this method to get many shape ids is likely
  /// significantly faster than calling `GetCellShape()` for each cell.
  VISKORES_CONT
  typename viskores::cont::ArrayHandle<viskores::UInt8, ShapesStorageTag>::ReadPortalType
  ShapesReadPortal() const;

  VISKORES_CONT viskores::UInt8 GetCellShape(viskores::Id cellid) const override;

  /// Retrieves the indices of the points incident to the given cell.
  /// If the provided `viskores::Vec` does not have enough components, the result will be truncated.
  template <viskores::IdComponent NumIndices>
  VISKORES_CONT void GetIndices(viskores::Id index,
                                viskores::Vec<viskores::Id, NumIndices>& ids) const;

  /// Retrieves the indices of the points incident to the given cell.
  VISKORES_CONT void GetIndices(viskores::Id index,
                                viskores::cont::ArrayHandle<viskores::Id>& ids) const;

  /// @brief Start adding cells one at a time.
  ///
  /// After this method is called, `AddCell` is called repeatedly to add each cell.
  /// Once all cells are added, call `CompleteAddingCells`.
  VISKORES_CONT void PrepareToAddCells(viskores::Id numCells, viskores::Id connectivityMaxLen);

  /// @brief Add a cell.
  ///
  /// This can only be called after `AddCell`.
  template <typename IdVecType>
  VISKORES_CONT void AddCell(viskores::UInt8 cellType,
                             viskores::IdComponent numVertices,
                             const IdVecType& ids);

  /// @brief Finish adding cells one at a time.
  VISKORES_CONT void CompleteAddingCells(viskores::Id numPoints);

  /// @brief Set all the cells of the mesh.
  ///
  /// This method can be used to fill the memory from another system without
  /// copying data.
  VISKORES_CONT
  void Fill(viskores::Id numPoints,
            const viskores::cont::ArrayHandle<viskores::UInt8, ShapesStorageTag>& cellTypes,
            const viskores::cont::ArrayHandle<viskores::Id, ConnectivityStorageTag>& connectivity,
            const viskores::cont::ArrayHandle<viskores::Id, OffsetsStorageTag>& offsets);

  template <typename VisitTopology, typename IncidentTopology>
  using ExecConnectivityType =
    typename ConnectivityChooser<VisitTopology, IncidentTopology>::ExecConnectivityType;

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
  template <typename VisitTopology, typename IncidentTopology>
  VISKORES_CONT ExecConnectivityType<VisitTopology, IncidentTopology> PrepareForInput(
    viskores::cont::DeviceAdapterId device,
    VisitTopology visitTopology,
    IncidentTopology incidentTopology,
    viskores::cont::Token& token) const;

  /// Returns the `viskores::cont::ArrayHandle` holding the shape information.
  /// The shapes array corresponding to `viskores::TopologyElementTagCell` for the `VisitTopology`
  /// and `viskores::TopologyElementTagPoint` for the `IncidentTopology` is the same as that provided
  /// when filling the explicit cell set.
  /// `ExplicitCellSet` is capable of providing the inverse connections (cells incident on
  /// each point) on request.
  template <typename VisitTopology, typename IncidentTopology>
  VISKORES_CONT const typename ConnectivityChooser<VisitTopology,
                                                   IncidentTopology>::ShapesArrayType&
    GetShapesArray(VisitTopology, IncidentTopology) const;

  /// Returns the `viskores::cont::ArrayHandle` containing the connectivity information.
  /// Returns the `viskores::cont::ArrayHandle` holding the shape information.
  /// The incident array corresponding to `viskores::TopologyElementTagCell` for the `VisitTopology`
  /// and `viskores::TopologyElementTagPoint` for the `IncidentTopology` is the same as that provided
  /// when filling the explicit cell set.
  /// `ExplicitCellSet` is capable of providing the inverse connections (cells incident on
  /// each point) on request.
  template <typename VisitTopology, typename IncidentTopology>
  VISKORES_CONT const typename ConnectivityChooser<VisitTopology,
                                                   IncidentTopology>::ConnectivityArrayType&
    GetConnectivityArray(VisitTopology, IncidentTopology) const;

  /// Returns the `viskores::cont::ArrayHandle` containing the offsets into theconnectivity information.
  /// Returns the `viskores::cont::ArrayHandle` holding the offset information.
  /// The offset array corresponding to `viskores::TopologyElementTagCell` for the `VisitTopology`
  /// and `viskores::TopologyElementTagPoint` for the `IncidentTopology` is the same as that provided
  /// when filling the explicit cell set.
  /// `ExplicitCellSet` is capable of providing the inverse connections (cells incident on
  /// each point) on request.
  template <typename VisitTopology, typename IncidentTopology>
  VISKORES_CONT const typename ConnectivityChooser<VisitTopology,
                                                   IncidentTopology>::OffsetsArrayType&
    GetOffsetsArray(VisitTopology, IncidentTopology) const;

  template <typename VisitTopology, typename IncidentTopology>
  VISKORES_CONT typename ConnectivityChooser<VisitTopology, IncidentTopology>::NumIndicesArrayType
    GetNumIndicesArray(VisitTopology, IncidentTopology) const;

  /// Returns whether the `CellSetExplicit` has information for the given visit and incident
  /// topology elements. If the connectivity is not available, it will be automatically created
  /// if requested, but that will take time.
  template <typename VisitTopology, typename IncidentTopology>
  VISKORES_CONT bool HasConnectivity(VisitTopology visit, IncidentTopology incident) const
  {
    return this->HasConnectivityImpl(visit, incident);
  }

  // Can be used to reset a connectivity table, mostly useful for benchmarking.
  template <typename VisitTopology, typename IncidentTopology>
  VISKORES_CONT void ResetConnectivity(VisitTopology visit, IncidentTopology incident)
  {
    this->ResetConnectivityImpl(visit, incident);
  }

protected:
  VISKORES_CONT void BuildConnectivity(viskores::cont::DeviceAdapterId,
                                       viskores::TopologyElementTagCell,
                                       viskores::TopologyElementTagPoint) const
  {
    VISKORES_ASSERT(this->Data->CellPointIds.ElementsValid);
    // no-op
  }

  VISKORES_CONT void BuildConnectivity(viskores::cont::DeviceAdapterId device,
                                       viskores::TopologyElementTagPoint,
                                       viskores::TopologyElementTagCell) const
  {
    detail::BuildReverseConnectivity(this->Data->CellPointIds.Connectivity,
                                     this->Data->CellPointIds.Offsets,
                                     this->Data->NumberOfPoints,
                                     this->Data->PointCellIds,
                                     device);
  }

  VISKORES_CONT bool HasConnectivityImpl(viskores::TopologyElementTagCell,
                                         viskores::TopologyElementTagPoint) const
  {
    return this->Data->CellPointIds.ElementsValid;
  }

  VISKORES_CONT bool HasConnectivityImpl(viskores::TopologyElementTagPoint,
                                         viskores::TopologyElementTagCell) const
  {
    return this->Data->PointCellIds.ElementsValid;
  }

  VISKORES_CONT void ResetConnectivityImpl(viskores::TopologyElementTagCell,
                                           viskores::TopologyElementTagPoint)
  {
    // Reset entire cell set
    this->Data->CellPointIds = CellPointIdsType{};
    this->Data->PointCellIds = PointCellIdsType{};
    this->Data->ConnectivityAdded = -1;
    this->Data->NumberOfCellsAdded = -1;
    this->Data->NumberOfPoints = 0;
  }

  VISKORES_CONT void ResetConnectivityImpl(viskores::TopologyElementTagPoint,
                                           viskores::TopologyElementTagCell)
  {
    this->Data->PointCellIds = PointCellIdsType{};
  }

  // Store internals in a shared pointer so shallow copies stay consistent.
  // See #2268.
  struct Internals
  {
    CellPointIdsType CellPointIds;
    PointCellIdsType PointCellIds;

    // These are used in the AddCell and related methods to incrementally add
    // cells. They need to be protected as subclasses of CellSetExplicit
    // need to set these values when implementing Fill()
    viskores::Id ConnectivityAdded;
    viskores::Id NumberOfCellsAdded;
    viskores::Id NumberOfPoints;

    VISKORES_CONT
    Internals()
      : ConnectivityAdded(-1)
      , NumberOfCellsAdded(-1)
      , NumberOfPoints(0)
    {
    }
  };

  std::shared_ptr<Internals> Data;

private:
  VISKORES_CONT
  const CellPointIdsType& GetConnectivity(viskores::TopologyElementTagCell,
                                          viskores::TopologyElementTagPoint) const
  {
    return this->Data->CellPointIds;
  }

  VISKORES_CONT
  const CellPointIdsType& GetConnectivity(viskores::TopologyElementTagCell,
                                          viskores::TopologyElementTagPoint)
  {
    return this->Data->CellPointIds;
  }

  VISKORES_CONT
  const PointCellIdsType& GetConnectivity(viskores::TopologyElementTagPoint,
                                          viskores::TopologyElementTagCell) const
  {
    return this->Data->PointCellIds;
  }

  VISKORES_CONT
  const PointCellIdsType& GetConnectivity(viskores::TopologyElementTagPoint,
                                          viskores::TopologyElementTagCell)
  {
    return this->Data->PointCellIds;
  }
};

namespace detail
{

template <typename Storage1, typename Storage2, typename Storage3>
struct CellSetExplicitConnectivityChooser<
  viskores::cont::CellSetExplicit<Storage1, Storage2, Storage3>,
  viskores::TopologyElementTagCell,
  viskores::TopologyElementTagPoint>
{
  using ConnectivityType =
    viskores::cont::internal::ConnectivityExplicitInternals<Storage1, Storage2, Storage3>;
};

template <typename CellSetType>
struct CellSetExplicitConnectivityChooser<CellSetType,
                                          viskores::TopologyElementTagPoint,
                                          viskores::TopologyElementTagCell>
{
  //only specify the shape type as it will be constant as everything
  //is a vertex. otherwise use the defaults.
  using ConnectivityType = viskores::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit;
};

} // namespace detail

/// \cond
/// Make doxygen ignore this section
#ifndef viskores_cont_CellSetExplicit_cxx
extern template class VISKORES_CONT_TEMPLATE_EXPORT CellSetExplicit<>; // default
extern template class VISKORES_CONT_TEMPLATE_EXPORT CellSetExplicit<
  typename viskores::cont::ArrayHandleConstant<viskores::UInt8>::StorageTag,
  VISKORES_DEFAULT_CONNECTIVITY_STORAGE_TAG,
  typename viskores::cont::ArrayHandleCounting<viskores::Id>::StorageTag>; // CellSetSingleType base
#endif
/// \endcond
}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <typename SST, typename CST, typename OST>
struct SerializableTypeString<viskores::cont::CellSetExplicit<SST, CST, OST>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "CS_Explicit<" +
      SerializableTypeString<viskores::cont::ArrayHandle<viskores::UInt8, SST>>::Get() + "_ST," +
      SerializableTypeString<viskores::cont::ArrayHandle<viskores::Id, CST>>::Get() + "_ST," +
      SerializableTypeString<viskores::cont::ArrayHandle<viskores::Id, OST>>::Get() + "_ST>";

    return name;
  }
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename SST, typename CST, typename OST>
struct Serialization<viskores::cont::CellSetExplicit<SST, CST, OST>>
{
private:
  using Type = viskores::cont::CellSetExplicit<SST, CST, OST>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    viskoresdiy::save(bb, cs.GetNumberOfPoints());
    viskoresdiy::save(
      bb,
      cs.GetShapesArray(viskores::TopologyElementTagCell{}, viskores::TopologyElementTagPoint{}));
    viskoresdiy::save(bb,
                      cs.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                              viskores::TopologyElementTagPoint{}));
    viskoresdiy::save(
      bb,
      cs.GetOffsetsArray(viskores::TopologyElementTagCell{}, viskores::TopologyElementTagPoint{}));
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    viskores::Id numberOfPoints = 0;
    viskoresdiy::load(bb, numberOfPoints);
    viskores::cont::ArrayHandle<viskores::UInt8, SST> shapes;
    viskoresdiy::load(bb, shapes);
    viskores::cont::ArrayHandle<viskores::Id, CST> connectivity;
    viskoresdiy::load(bb, connectivity);
    viskores::cont::ArrayHandle<viskores::Id, OST> offsets;
    viskoresdiy::load(bb, offsets);

    cs = Type{};
    cs.Fill(numberOfPoints, shapes, connectivity, offsets);
  }
};

} // diy
/// @endcond SERIALIZATION

#ifndef viskores_cont_CellSetExplicit_hxx
#include <viskores/cont/CellSetExplicit.hxx>
#endif //viskores_cont_CellSetExplicit_hxx

#endif //viskores_cont_CellSetExplicit_h
