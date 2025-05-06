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
#ifndef viskores_cont_CellSetStructured_h
#define viskores_cont_CellSetStructured_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/TopologyElementTag.h>
#include <viskores/cont/CellSet.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/exec/ConnectivityStructured.h>
#include <viskores/internal/ConnectivityStructuredInternals.h>

namespace viskores
{
namespace cont
{

/// @brief Defines a 1-, 2-, or 3-dimensional structured grid of points.
///
/// The structured cells form lines, quadrilaterals, or hexahedra for 1-, 2-, or 3-dimensions,
/// respectively, to connect th epoints.
/// The topology is specified by simply providing the dimensions, which is the number of points
/// in the i, j, and k directions of the grid of points.
template <viskores::IdComponent DIMENSION>
class VISKORES_ALWAYS_EXPORT CellSetStructured final : public CellSet
{
private:
  using Thisclass = viskores::cont::CellSetStructured<DIMENSION>;
  using InternalsType = viskores::internal::ConnectivityStructuredInternals<DIMENSION>;

public:
  static const viskores::IdComponent Dimension = DIMENSION;

  using SchedulingRangeType = typename InternalsType::SchedulingRangeType;

  /// @brief Get the number of cells in the topology.
  viskores::Id GetNumberOfCells() const override { return this->Structure.GetNumberOfCells(); }

  /// @brief Get the number of points in the topology.
  viskores::Id GetNumberOfPoints() const override { return this->Structure.GetNumberOfPoints(); }

  viskores::Id GetNumberOfFaces() const override { return -1; }

  viskores::Id GetNumberOfEdges() const override { return -1; }

  // Since the entire topology is defined by by three integers, nothing to do here.
  void ReleaseResourcesExecution() override {}

  /// @brief Set the dimensions of the structured array of points.
  void SetPointDimensions(SchedulingRangeType dimensions)
  {
    this->Structure.SetPointDimensions(dimensions);
  }

  void SetGlobalPointDimensions(SchedulingRangeType dimensions)
  {
    this->Structure.SetGlobalPointDimensions(dimensions);
  }

  void SetGlobalPointIndexStart(SchedulingRangeType start)
  {
    this->Structure.SetGlobalPointIndexStart(start);
  }

  /// Get the dimensions of the points.
  SchedulingRangeType GetPointDimensions() const { return this->Structure.GetPointDimensions(); }

  SchedulingRangeType GetGlobalPointDimensions() const
  {
    return this->Structure.GetGlobalPointDimensions();
  }

  /// Get the dimensions of the cells.
  /// This is typically one less than the dimensions of the points.
  SchedulingRangeType GetCellDimensions() const { return this->Structure.GetCellDimensions(); }

  SchedulingRangeType GetGlobalCellDimensions() const
  {
    return this->Structure.GetGlobalCellDimensions();
  }

  SchedulingRangeType GetGlobalPointIndexStart() const
  {
    return this->Structure.GetGlobalPointIndexStart();
  }

  viskores::IdComponent GetNumberOfPointsInCell(
    viskores::Id viskoresNotUsed(cellIndex) = 0) const override
  {
    return this->Structure.GetNumberOfPointsInCell();
  }

  viskores::UInt8 GetCellShape(viskores::Id viskoresNotUsed(cellIndex) = 0) const override
  {
    return static_cast<viskores::UInt8>(this->Structure.GetCellShape());
  }

  void GetCellPointIds(viskores::Id id, viskores::Id* ptids) const override
  {
    auto asVec = this->Structure.GetPointsOfCell(id);
    for (viskores::IdComponent i = 0; i < InternalsType::NUM_POINTS_IN_CELL; ++i)
    {
      ptids[i] = asVec[i];
    }
  }

  std::shared_ptr<CellSet> NewInstance() const override
  {
    return std::make_shared<CellSetStructured>();
  }

  void DeepCopy(const CellSet* src) override
  {
    const auto* other = dynamic_cast<const CellSetStructured*>(src);
    if (!other)
    {
      throw viskores::cont::ErrorBadType("CellSetStructured::DeepCopy types don't match");
    }

    this->Structure = other->Structure;
  }

  template <typename TopologyElement>
  SchedulingRangeType GetSchedulingRange(TopologyElement) const
  {
    VISKORES_IS_TOPOLOGY_ELEMENT_TAG(TopologyElement);
    return this->Structure.GetSchedulingRange(TopologyElement());
  }

  template <typename VisitTopology, typename IncidentTopology>
  using ExecConnectivityType =
    viskores::exec::ConnectivityStructured<VisitTopology, IncidentTopology, Dimension>;

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
  ExecConnectivityType<VisitTopology, IncidentTopology> PrepareForInput(
    viskores::cont::DeviceAdapterId device,
    VisitTopology visitTopology,
    IncidentTopology incidentTopology,
    viskores::cont::Token& token) const
  {
    (void)device;
    (void)visitTopology;
    (void)incidentTopology;
    (void)token;
    return ExecConnectivityType<VisitTopology, IncidentTopology>(this->Structure);
  }

  void PrintSummary(std::ostream& out) const override
  {
    out << "  StructuredCellSet:\n";
    this->Structure.PrintSummary(out);
  }

  // Cannot use the default implementation of the destructor because the CUDA compiler
  // will attempt to create it for device, and then it will fail when it tries to call
  // the destructor of the superclass.
  ~CellSetStructured() override {}

  CellSetStructured() = default;

  CellSetStructured(const CellSetStructured& src)
    : CellSet()
    , Structure(src.Structure)
  {
  }

  CellSetStructured& operator=(const CellSetStructured& src)
  {
    this->Structure = src.Structure;
    return *this;
  }

  CellSetStructured(CellSetStructured&& src) noexcept
    : CellSet()
    , Structure(std::move(src.Structure))
  {
  }

  CellSetStructured& operator=(CellSetStructured&& src) noexcept
  {
    this->Structure = std::move(src.Structure);
    return *this;
  }

private:
  InternalsType Structure;
};

#ifndef viskores_cont_CellSetStructured_cxx
extern template class VISKORES_CONT_TEMPLATE_EXPORT CellSetStructured<1>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT CellSetStructured<2>;
extern template class VISKORES_CONT_TEMPLATE_EXPORT CellSetStructured<3>;
#endif
}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <viskores::IdComponent DIMENSION>
struct SerializableTypeString<viskores::cont::CellSetStructured<DIMENSION>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "CS_Structured<" + std::to_string(DIMENSION) + ">";
    return name;
  }
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <viskores::IdComponent DIMENSION>
struct Serialization<viskores::cont::CellSetStructured<DIMENSION>>
{
private:
  using Type = viskores::cont::CellSetStructured<DIMENSION>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    viskoresdiy::save(bb, cs.GetPointDimensions());
    viskoresdiy::save(bb, cs.GetGlobalPointDimensions());
    viskoresdiy::save(bb, cs.GetGlobalPointIndexStart());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    typename Type::SchedulingRangeType dims, gdims, start;
    viskoresdiy::load(bb, dims);
    viskoresdiy::load(bb, gdims);
    viskoresdiy::load(bb, start);

    cs = Type{};
    cs.SetPointDimensions(dims);
    cs.SetGlobalPointDimensions(gdims);
    cs.SetGlobalPointIndexStart(start);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_CellSetStructured_h
