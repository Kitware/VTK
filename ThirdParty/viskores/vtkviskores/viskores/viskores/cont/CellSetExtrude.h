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
#ifndef viskores_cont_CellSetExtrude_h
#define viskores_cont_CellSetExtrude_h

#include <viskores/TopologyElementTag.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleXGCCoordinates.h>
#include <viskores/cont/CellSet.h>
#include <viskores/cont/Invoker.h>
#include <viskores/exec/ConnectivityExtrude.h>
#include <viskores/exec/arg/ThreadIndicesExtrude.h>

namespace viskores
{
namespace cont
{

namespace detail
{

template <typename VisitTopology, typename IncidentTopology>
struct CellSetExtrudeConnectivityChooser;

template <>
struct CellSetExtrudeConnectivityChooser<viskores::TopologyElementTagCell,
                                         viskores::TopologyElementTagPoint>
{
  using ExecConnectivityType = viskores::exec::ConnectivityExtrude;
};

template <>
struct CellSetExtrudeConnectivityChooser<viskores::TopologyElementTagPoint,
                                         viskores::TopologyElementTagCell>
{
  using ExecConnectivityType = viskores::exec::ReverseConnectivityExtrude;
};

} // namespace detail

/// @brief Defines a 3-dimensional extruded mesh representation.
///
/// `CellSetExtrude` takes takes a mesh defined in the XZ-plane and extrudes it along
/// the Y-axis. This plane is repeated in a series of steps and forms wedge cells
/// between them.
///
/// The extrusion can be linear or rotational (e.g., to form a torus).
class VISKORES_CONT_EXPORT CellSetExtrude : public CellSet
{
public:
  VISKORES_CONT CellSetExtrude();

  VISKORES_CONT CellSetExtrude(const viskores::cont::ArrayHandle<viskores::Int32>& conn,
                               viskores::Int32 numberOfPointsPerPlane,
                               viskores::Int32 numberOfPlanes,
                               const viskores::cont::ArrayHandle<viskores::Int32>& nextNode,
                               bool periodic);

  VISKORES_CONT CellSetExtrude(const CellSetExtrude& src);
  VISKORES_CONT CellSetExtrude(CellSetExtrude&& src) noexcept;

  VISKORES_CONT CellSetExtrude& operator=(const CellSetExtrude& src);
  VISKORES_CONT CellSetExtrude& operator=(CellSetExtrude&& src) noexcept;

  ~CellSetExtrude() override;

  viskores::Int32 GetNumberOfPlanes() const;

  viskores::Id GetNumberOfCells() const override;

  viskores::Id GetNumberOfPoints() const override;

  viskores::Id GetNumberOfFaces() const override;

  viskores::Id GetNumberOfEdges() const override;

  VISKORES_CONT viskores::Id2 GetSchedulingRange(viskores::TopologyElementTagCell) const;

  VISKORES_CONT viskores::Id2 GetSchedulingRange(viskores::TopologyElementTagPoint) const;

  viskores::UInt8 GetCellShape(viskores::Id id) const override;
  viskores::IdComponent GetNumberOfPointsInCell(viskores::Id id) const override;
  void GetCellPointIds(viskores::Id id, viskores::Id* ptids) const override;

  std::shared_ptr<CellSet> NewInstance() const override;
  void DeepCopy(const CellSet* src) override;

  void PrintSummary(std::ostream& out) const override;
  void ReleaseResourcesExecution() override;

  const viskores::cont::ArrayHandle<viskores::Int32>& GetConnectivityArray() const
  {
    return this->Connectivity;
  }

  viskores::Int32 GetNumberOfPointsPerPlane() const { return this->NumberOfPointsPerPlane; }

  const viskores::cont::ArrayHandle<viskores::Int32>& GetNextNodeArray() const
  {
    return this->NextNode;
  }

  bool GetIsPeriodic() const { return this->IsPeriodic; }

  template <viskores::IdComponent NumIndices>
  VISKORES_CONT void GetIndices(viskores::Id index,
                                viskores::Vec<viskores::Id, NumIndices>& ids) const;

  VISKORES_CONT void GetIndices(viskores::Id index,
                                viskores::cont::ArrayHandle<viskores::Id>& ids) const;

  template <typename VisitTopology, typename IncidentTopology>
  using ExecConnectivityType =
    typename detail::CellSetExtrudeConnectivityChooser<VisitTopology,
                                                       IncidentTopology>::ExecConnectivityType;

  viskores::exec::ConnectivityExtrude PrepareForInput(viskores::cont::DeviceAdapterId,
                                                      viskores::TopologyElementTagCell,
                                                      viskores::TopologyElementTagPoint,
                                                      viskores::cont::Token&) const;

  viskores::exec::ReverseConnectivityExtrude PrepareForInput(viskores::cont::DeviceAdapterId,
                                                             viskores::TopologyElementTagPoint,
                                                             viskores::TopologyElementTagCell,
                                                             viskores::cont::Token&) const;

private:
  void BuildReverseConnectivity();

  bool IsPeriodic;

  viskores::Int32 NumberOfPointsPerPlane;
  viskores::Int32 NumberOfCellsPerPlane;
  viskores::Int32 NumberOfPlanes;
  viskores::cont::ArrayHandle<viskores::Int32> Connectivity;
  viskores::cont::ArrayHandle<viskores::Int32> NextNode;

  bool ReverseConnectivityBuilt;
  viskores::cont::ArrayHandle<viskores::Int32> RConnectivity;
  viskores::cont::ArrayHandle<viskores::Int32> ROffsets;
  viskores::cont::ArrayHandle<viskores::Int32> RCounts;
  viskores::cont::ArrayHandle<viskores::Int32> PrevNode;
};

template <typename T>
CellSetExtrude make_CellSetExtrude(const viskores::cont::ArrayHandle<viskores::Int32>& conn,
                                   const viskores::cont::ArrayHandleXGCCoordinates<T>& coords,
                                   const viskores::cont::ArrayHandle<viskores::Int32>& nextNode,
                                   bool periodic = true)
{
  return CellSetExtrude{
    conn, coords.GetNumberOfPointsPerPlane(), coords.GetNumberOfPlanes(), nextNode, periodic
  };
}

template <typename T>
CellSetExtrude make_CellSetExtrude(const std::vector<viskores::Int32>& conn,
                                   const viskores::cont::ArrayHandleXGCCoordinates<T>& coords,
                                   const std::vector<viskores::Int32>& nextNode,
                                   bool periodic = true)
{
  return CellSetExtrude{ viskores::cont::make_ArrayHandle(conn, viskores::CopyFlag::On),
                         static_cast<viskores::Int32>(coords.GetNumberOfPointsPerPlane()),
                         static_cast<viskores::Int32>(coords.GetNumberOfPlanes()),
                         viskores::cont::make_ArrayHandle(nextNode, viskores::CopyFlag::On),
                         periodic };
}

template <typename T>
CellSetExtrude make_CellSetExtrude(std::vector<viskores::Int32>&& conn,
                                   const viskores::cont::ArrayHandleXGCCoordinates<T>& coords,
                                   std::vector<viskores::Int32>&& nextNode,
                                   bool periodic = true)
{
  return CellSetExtrude{ viskores::cont::make_ArrayHandleMove(std::move(conn)),
                         static_cast<viskores::Int32>(coords.GetNumberOfPointsPerPlane()),
                         static_cast<viskores::Int32>(coords.GetNumberOfPlanes()),
                         viskores::cont::make_ArrayHandleMove(std::move(nextNode)),
                         periodic };
}
}
} // viskores::cont


//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <>
struct SerializableTypeString<viskores::cont::CellSetExtrude>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "CS_Extrude";
    return name;
  }
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <>
struct Serialization<viskores::cont::CellSetExtrude>
{
private:
  using Type = viskores::cont::CellSetExtrude;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    viskoresdiy::save(bb, cs.GetNumberOfPointsPerPlane());
    viskoresdiy::save(bb, cs.GetNumberOfPlanes());
    viskoresdiy::save(bb, cs.GetIsPeriodic());
    viskoresdiy::save(bb, cs.GetConnectivityArray());
    viskoresdiy::save(bb, cs.GetNextNodeArray());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    viskores::Int32 numberOfPointsPerPlane;
    viskores::Int32 numberOfPlanes;
    bool isPeriodic;
    viskores::cont::ArrayHandle<viskores::Int32> conn;
    viskores::cont::ArrayHandle<viskores::Int32> nextNode;

    viskoresdiy::load(bb, numberOfPointsPerPlane);
    viskoresdiy::load(bb, numberOfPlanes);
    viskoresdiy::load(bb, isPeriodic);
    viskoresdiy::load(bb, conn);
    viskoresdiy::load(bb, nextNode);

    cs = Type{ conn, numberOfPointsPerPlane, numberOfPlanes, nextNode, isPeriodic };
  }
};

} // diy
/// @endcond SERIALIZATION

#endif // viskores_cont_CellSetExtrude.h
