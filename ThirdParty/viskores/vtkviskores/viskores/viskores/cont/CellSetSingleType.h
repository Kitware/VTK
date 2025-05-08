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
#ifndef viskores_cont_CellSetSingleType_h
#define viskores_cont_CellSetSingleType_h

#include <viskores/CellShape.h>
#include <viskores/CellTraits.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/CellSet.h>
#include <viskores/cont/CellSetExplicit.h>

#include <map>
#include <utility>

namespace viskores
{
namespace cont
{

/// @brief An explicit cell set with all cells of the same shape.
///
/// `CellSetSingleType` is an explicit cell set constrained to contain cells that
/// all have the same shape and all have the same number of points. So, for example
/// if you are creating a surface that you know will contain only triangles,
/// `CellSetSingleType` is a good representation for these data.
///
/// Using `CellSetSingleType` saves memory because the array of cell shapes and the
/// array of point counts no longer need to be stored. `CellSetSingleType` also allows
/// Viskores to skip some processing and other storage required for general explicit cell
/// sets.
template <typename ConnectivityStorageTag = VISKORES_DEFAULT_CONNECTIVITY_STORAGE_TAG>
class VISKORES_ALWAYS_EXPORT CellSetSingleType
  : public viskores::cont::CellSetExplicit<
      typename viskores::cont::ArrayHandleConstant<viskores::UInt8>::StorageTag, //ShapesStorageTag
      ConnectivityStorageTag,
      typename viskores::cont::ArrayHandleCounting<viskores::Id>::StorageTag //OffsetsStorageTag
      >
{
  using Thisclass = viskores::cont::CellSetSingleType<ConnectivityStorageTag>;
  using Superclass = viskores::cont::CellSetExplicit<
    typename viskores::cont::ArrayHandleConstant<viskores::UInt8>::StorageTag,
    ConnectivityStorageTag,
    typename viskores::cont::ArrayHandleCounting<viskores::Id>::StorageTag>;

public:
  VISKORES_CONT
  CellSetSingleType()
    : Superclass()
    , ExpectedNumberOfCellsAdded(-1)
    , CellShapeAsId(CellShapeTagEmpty::Id)
    , NumberOfPointsPerCell(0)
  {
  }

  VISKORES_CONT
  CellSetSingleType(const Thisclass& src)
    : Superclass(src)
    , ExpectedNumberOfCellsAdded(-1)
    , CellShapeAsId(src.CellShapeAsId)
    , NumberOfPointsPerCell(src.NumberOfPointsPerCell)
  {
  }

  VISKORES_CONT
  CellSetSingleType(Thisclass&& src) noexcept
    : Superclass(std::forward<Superclass>(src))
    , ExpectedNumberOfCellsAdded(-1)
    , CellShapeAsId(src.CellShapeAsId)
    , NumberOfPointsPerCell(src.NumberOfPointsPerCell)
  {
  }


  VISKORES_CONT
  Thisclass& operator=(const Thisclass& src)
  {
    this->Superclass::operator=(src);
    this->CellShapeAsId = src.CellShapeAsId;
    this->NumberOfPointsPerCell = src.NumberOfPointsPerCell;
    return *this;
  }

  VISKORES_CONT
  Thisclass& operator=(Thisclass&& src) noexcept
  {
    this->Superclass::operator=(std::forward<Superclass>(src));
    this->CellShapeAsId = src.CellShapeAsId;
    this->NumberOfPointsPerCell = src.NumberOfPointsPerCell;
    return *this;
  }

  ~CellSetSingleType() override {}

  /// @brief Start adding cells one at a time.
  ///
  /// After this method is called, `AddCell` is called repeatedly to add each cell.
  /// Once all cells are added, call `CompleteAddingCells`.
  VISKORES_CONT void PrepareToAddCells(viskores::Id numCells, viskores::Id connectivityMaxLen)
  {
    this->CellShapeAsId = viskores::CELL_SHAPE_EMPTY;

    this->Data->CellPointIds.Connectivity.Allocate(connectivityMaxLen);

    this->Data->NumberOfCellsAdded = 0;
    this->Data->ConnectivityAdded = 0;
    this->ExpectedNumberOfCellsAdded = numCells;
  }

  /// @brief Add a cell.
  ///
  /// This can only be called after `AddCell`.
  template <typename IdVecType>
  VISKORES_CONT void AddCell(viskores::UInt8 shapeId,
                             viskores::IdComponent numVertices,
                             const IdVecType& ids)
  {
    using Traits = viskores::VecTraits<IdVecType>;
    VISKORES_STATIC_ASSERT_MSG((std::is_same<typename Traits::ComponentType, viskores::Id>::value),
                               "CellSetSingleType::AddCell requires viskores::Id for indices.");

    if (Traits::GetNumberOfComponents(ids) < numVertices)
    {
      throw viskores::cont::ErrorBadValue(
        "Not enough indices given to CellSetSingleType::AddCell.");
    }

    if (this->Data->ConnectivityAdded + numVertices >
        this->Data->CellPointIds.Connectivity.GetNumberOfValues())
    {
      throw viskores::cont::ErrorBadValue(
        "Connectivity increased past estimated maximum connectivity.");
    }

    if (this->CellShapeAsId == viskores::CELL_SHAPE_EMPTY)
    {
      if (shapeId == viskores::CELL_SHAPE_EMPTY)
      {
        throw viskores::cont::ErrorBadValue("Cannot create cells of type empty.");
      }
      this->CellShapeAsId = shapeId;
      this->CheckNumberOfPointsPerCell(numVertices);
      this->NumberOfPointsPerCell = numVertices;
    }
    else
    {
      if (shapeId != this->GetCellShape(0))
      {
        throw viskores::cont::ErrorBadValue("Cannot have differing shapes in CellSetSingleType.");
      }
      if (numVertices != this->NumberOfPointsPerCell)
      {
        throw viskores::cont::ErrorBadValue(
          "Inconsistent number of points in cells for CellSetSingleType.");
      }
    }
    auto conn = this->Data->CellPointIds.Connectivity.WritePortal();
    for (viskores::IdComponent iVert = 0; iVert < numVertices; ++iVert)
    {
      conn.Set(this->Data->ConnectivityAdded + iVert, Traits::GetComponent(ids, iVert));
    }
    this->Data->NumberOfCellsAdded++;
    this->Data->ConnectivityAdded += numVertices;
  }

  /// @brief Finish adding cells one at a time.
  VISKORES_CONT void CompleteAddingCells(viskores::Id numPoints)
  {
    this->Data->NumberOfPoints = numPoints;
    this->Data->CellPointIds.Connectivity.Allocate(this->Data->ConnectivityAdded,
                                                   viskores::CopyFlag::On);

    viskores::Id numCells = this->Data->NumberOfCellsAdded;

    this->Data->CellPointIds.Shapes =
      viskores::cont::make_ArrayHandleConstant(this->GetCellShape(0), numCells);
    this->Data->CellPointIds.Offsets = viskores::cont::make_ArrayHandleCounting(
      viskores::Id(0), static_cast<viskores::Id>(this->NumberOfPointsPerCell), numCells);

    this->Data->CellPointIds.ElementsValid = true;

    if (this->ExpectedNumberOfCellsAdded != this->GetNumberOfCells())
    {
      throw viskores::cont::ErrorBadValue("Did not add the expected number of cells.");
    }

    this->Data->NumberOfCellsAdded = -1;
    this->Data->ConnectivityAdded = -1;
    this->ExpectedNumberOfCellsAdded = -1;
  }

  /// @brief Set all the cells of the mesh.
  ///
  /// This method can be used to fill the memory from another system without
  /// copying data.
  VISKORES_CONT void Fill(
    viskores::Id numPoints,
    viskores::UInt8 shapeId,
    viskores::IdComponent numberOfPointsPerCell,
    const viskores::cont::ArrayHandle<viskores::Id, ConnectivityStorageTag>& connectivity)
  {
    this->Data->NumberOfPoints = numPoints;
    this->CellShapeAsId = shapeId;
    this->CheckNumberOfPointsPerCell(numberOfPointsPerCell);

    const viskores::Id numCells = connectivity.GetNumberOfValues() / numberOfPointsPerCell;
    VISKORES_ASSERT((connectivity.GetNumberOfValues() % numberOfPointsPerCell) == 0);

    this->Data->CellPointIds.Shapes = viskores::cont::make_ArrayHandleConstant(shapeId, numCells);

    this->Data->CellPointIds.Offsets = viskores::cont::make_ArrayHandleCounting(
      viskores::Id(0), static_cast<viskores::Id>(numberOfPointsPerCell), numCells + 1);

    this->Data->CellPointIds.Connectivity = connectivity;

    this->Data->CellPointIds.ElementsValid = true;

    this->ResetConnectivity(TopologyElementTagPoint{}, TopologyElementTagCell{});
  }

  VISKORES_CONT
  viskores::Id GetCellShapeAsId() const { return this->CellShapeAsId; }

  VISKORES_DEPRECATED_SUPPRESS_BEGIN
  VISKORES_CONT
  viskores::UInt8 GetCellShape(viskores::Id viskoresNotUsed(cellIndex)) const override
  {
    return static_cast<viskores::UInt8>(this->CellShapeAsId);
  }
  VISKORES_DEPRECATED_SUPPRESS_END

  VISKORES_CONT
  std::shared_ptr<CellSet> NewInstance() const override
  {
    return std::make_shared<CellSetSingleType>();
  }

  VISKORES_CONT
  void DeepCopy(const CellSet* src) override
  {
    const auto* other = dynamic_cast<const CellSetSingleType*>(src);
    if (!other)
    {
      throw viskores::cont::ErrorBadType("CellSetSingleType::DeepCopy types don't match");
    }

    this->Superclass::DeepCopy(other);
    this->CellShapeAsId = other->CellShapeAsId;
    this->NumberOfPointsPerCell = other->NumberOfPointsPerCell;
  }

  void PrintSummary(std::ostream& out) const override
  {
    out << "   CellSetSingleType: Type=" << this->CellShapeAsId << std::endl;
    out << "   CellPointIds:" << std::endl;
    this->Data->CellPointIds.PrintSummary(out);
    out << "   PointCellIds:" << std::endl;
    this->Data->PointCellIds.PrintSummary(out);
  }

private:
  template <typename CellShapeTag>
  void CheckNumberOfPointsPerCell(CellShapeTag,
                                  viskores::CellTraitsTagSizeFixed,
                                  viskores::IdComponent numVertices) const
  {
    if (numVertices != viskores::CellTraits<CellShapeTag>::NUM_POINTS)
    {
      throw viskores::cont::ErrorBadValue("Passed invalid number of points for cell shape.");
    }
  }

  template <typename CellShapeTag>
  void CheckNumberOfPointsPerCell(CellShapeTag,
                                  viskores::CellTraitsTagSizeVariable,
                                  viskores::IdComponent viskoresNotUsed(numVertices)) const
  {
    // Technically, a shape with a variable number of points probably has a
    // minimum number of points, but we are not being sophisticated enough to
    // check that. Instead, just pass the check by returning without error.
  }

  void CheckNumberOfPointsPerCell(viskores::IdComponent numVertices) const
  {
    switch (this->CellShapeAsId)
    {
      viskoresGenericCellShapeMacro(this->CheckNumberOfPointsPerCell(
        CellShapeTag(), viskores::CellTraits<CellShapeTag>::IsSizeFixed(), numVertices));
      default:
        throw viskores::cont::ErrorBadValue("CellSetSingleType unable to determine the cell type");
    }
  }

  viskores::Id ExpectedNumberOfCellsAdded;
  viskores::Id CellShapeAsId;
  viskores::IdComponent NumberOfPointsPerCell;
};
}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <typename ConnectivityST>
struct SerializableTypeString<viskores::cont::CellSetSingleType<ConnectivityST>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "CS_Single<" +
      SerializableTypeString<viskores::cont::ArrayHandle<viskores::Id, ConnectivityST>>::Get() +
      "_ST>";

    return name;
  }
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename ConnectivityST>
struct Serialization<viskores::cont::CellSetSingleType<ConnectivityST>>
{
private:
  using Type = viskores::cont::CellSetSingleType<ConnectivityST>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    viskoresdiy::save(bb, cs.GetNumberOfPoints());
    viskoresdiy::save(bb, cs.GetCellShape(0));
    viskoresdiy::save(bb, cs.GetNumberOfPointsInCell(0));
    viskoresdiy::save(bb,
                      cs.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                              viskores::TopologyElementTagPoint{}));
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    viskores::Id numberOfPoints = 0;
    viskoresdiy::load(bb, numberOfPoints);
    viskores::UInt8 shape;
    viskoresdiy::load(bb, shape);
    viskores::IdComponent count;
    viskoresdiy::load(bb, count);
    viskores::cont::ArrayHandle<viskores::Id, ConnectivityST> connectivity;
    viskoresdiy::load(bb, connectivity);

    cs = Type{};
    cs.Fill(numberOfPoints, shape, count, connectivity);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_CellSetSingleType_h
