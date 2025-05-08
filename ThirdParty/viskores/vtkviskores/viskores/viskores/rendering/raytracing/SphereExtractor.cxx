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

#include <viskores/rendering/raytracing/SphereExtractor.h>

#include <viskores/rendering/raytracing/RayTracingTypeDefs.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

class CountPoints : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  VISKORES_CONT
  CountPoints() {}
  typedef void ControlSignature(CellSetIn cellset, FieldOut);
  typedef void ExecutionSignature(CellShape, _2);

  VISKORES_EXEC
  void operator()(viskores::CellShapeTagGeneric shapeType, viskores::Id& points) const
  {
    if (shapeType.Id == viskores::CELL_SHAPE_VERTEX)
      points = 1;
    else
      points = 0;
  }

  VISKORES_EXEC
  void operator()(viskores::CellShapeTagHexahedron viskoresNotUsed(shapeType),
                  viskores::Id& points) const
  {
    points = 0;
  }

  VISKORES_EXEC
  void operator()(viskores::CellShapeTagQuad viskoresNotUsed(shapeType), viskores::Id& points) const
  {
    points = 0;
  }
  VISKORES_EXEC
  void operator()(viskores::CellShapeTagWedge viskoresNotUsed(shapeType),
                  viskores::Id& points) const
  {
    points = 0;
  }

}; // ClassCountPoints

class Pointify : public viskores::worklet::WorkletVisitCellsWithPoints
{

public:
  VISKORES_CONT
  Pointify() {}
  typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
  typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

  template <typename VecType, typename OutputPortal>
  VISKORES_EXEC void operator()(const viskores::Id& viskoresNotUsed(pointOffset),
                                viskores::CellShapeTagQuad viskoresNotUsed(shapeType),
                                const VecType& viskoresNotUsed(cellIndices),
                                const viskores::Id& viskoresNotUsed(cellId),
                                OutputPortal& viskoresNotUsed(outputIndices)) const
  {
  }
  template <typename VecType, typename OutputPortal>
  VISKORES_EXEC void operator()(const viskores::Id& viskoresNotUsed(pointOffset),
                                viskores::CellShapeTagWedge viskoresNotUsed(shapeType),
                                const VecType& viskoresNotUsed(cellIndices),
                                const viskores::Id& viskoresNotUsed(cellId),
                                OutputPortal& viskoresNotUsed(outputIndices)) const
  {
  }

  template <typename VecType, typename OutputPortal>
  VISKORES_EXEC void operator()(const viskores::Id& viskoresNotUsed(pointOffset),
                                viskores::CellShapeTagHexahedron viskoresNotUsed(shapeType),
                                const VecType& viskoresNotUsed(cellIndices),
                                const viskores::Id& viskoresNotUsed(cellId),
                                OutputPortal& viskoresNotUsed(outputIndices)) const
  {
  }

  template <typename VecType, typename OutputPortal>
  VISKORES_EXEC void operator()(const viskores::Id& pointOffset,
                                viskores::CellShapeTagGeneric shapeType,
                                const VecType& viskoresNotUsed(cellIndices),
                                const viskores::Id& cellId,
                                OutputPortal& outputIndices) const
  {

    if (shapeType.Id == viskores::CELL_SHAPE_VERTEX)
    {
      outputIndices.Set(pointOffset, cellId);
    }
  }
}; //class pointify

class Iterator : public viskores::worklet::WorkletMapField
{

public:
  VISKORES_CONT
  Iterator() {}
  typedef void ControlSignature(FieldOut);
  typedef void ExecutionSignature(_1, WorkIndex);
  VISKORES_EXEC
  void operator()(viskores::Id& index, const viskores::Id& idx) const { index = idx; }
}; //class Iterator

class FieldRadius : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Float32 MinRadius;
  viskores::Float32 RadiusDelta;
  viskores::Float32 MinValue;
  viskores::Float32 InverseDelta;

public:
  VISKORES_CONT
  FieldRadius(const viskores::Float32 minRadius,
              const viskores::Float32 maxRadius,
              const viskores::Range scalarRange)
    : MinRadius(minRadius)
    , RadiusDelta(maxRadius - minRadius)
    , MinValue(static_cast<viskores::Float32>(scalarRange.Min))
  {
    viskores::Float32 delta = static_cast<viskores::Float32>(scalarRange.Max - scalarRange.Min);
    if (delta != 0.f)
      InverseDelta = 1.f / (delta);
    else
      InverseDelta = 0.f; // just map scalar to 0;
  }

  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename ScalarPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& pointId,
                                viskores::Float32& radius,
                                const ScalarPortalType& scalars) const
  {
    viskores::Float32 scalar = static_cast<viskores::Float32>(scalars.Get(pointId));
    viskores::Float32 t = (scalar - MinValue) * InverseDelta;
    radius = MinRadius + t * RadiusDelta;
  }

}; //class FieldRadius

} //namespace detail

void SphereExtractor::ExtractCoordinates(const viskores::cont::CoordinateSystem& coords,
                                         const viskores::Float32 radius)
{
  this->SetPointIdsFromCoords(coords);
  this->SetUniformRadius(radius);
}

void SphereExtractor::ExtractCoordinates(const viskores::cont::CoordinateSystem& coords,
                                         const viskores::cont::Field& field,
                                         const viskores::Float32 minRadius,
                                         const viskores::Float32 maxRadius)
{
  this->SetPointIdsFromCoords(coords);
  this->SetVaryingRadius(minRadius, maxRadius, field);
}

void SphereExtractor::ExtractCells(const viskores::cont::UnknownCellSet& cells,
                                   const viskores::Float32 radius)
{
  this->SetPointIdsFromCells(cells);
  this->SetUniformRadius(radius);
}
void SphereExtractor::ExtractCells(const viskores::cont::UnknownCellSet& cells,
                                   const viskores::cont::Field& field,
                                   const viskores::Float32 minRadius,
                                   const viskores::Float32 maxRadius)
{
  this->SetPointIdsFromCells(cells);
  this->SetVaryingRadius(minRadius, maxRadius, field);
}

void SphereExtractor::SetUniformRadius(const viskores::Float32 radius)
{
  const viskores::Id size = this->PointIds.GetNumberOfValues();
  Radii.Allocate(size);

  viskores::cont::ArrayHandleConstant<viskores::Float32> radiusHandle(radius, size);
  viskores::cont::Algorithm::Copy(radiusHandle, Radii);
}

void SphereExtractor::SetPointIdsFromCoords(const viskores::cont::CoordinateSystem& coords)
{
  viskores::Id size = coords.GetNumberOfPoints();
  this->PointIds.Allocate(size);
  viskores::worklet::DispatcherMapField<detail::Iterator>(detail::Iterator())
    .Invoke(this->PointIds);
}

void SphereExtractor::SetPointIdsFromCells(const viskores::cont::UnknownCellSet& cells)
{
  using SingleType = viskores::cont::CellSetSingleType<>;
  viskores::Id numCells = cells.GetNumberOfCells();
  if (numCells == 0)
  {
    return;
  }
  //
  // look for points in the cell set
  //
  if (cells.CanConvert<viskores::cont::CellSetExplicit<>>())
  {
    auto cellsExplicit = cells.AsCellSet<viskores::cont::CellSetExplicit<>>();

    viskores::cont::ArrayHandle<viskores::Id> points;
    viskores::worklet::DispatcherMapTopology<detail::CountPoints>(detail::CountPoints())
      .Invoke(cellsExplicit, points);

    viskores::Id totalPoints = 0;
    totalPoints = viskores::cont::Algorithm::Reduce(points, viskores::Id(0));

    viskores::cont::ArrayHandle<viskores::Id> cellOffsets;
    viskores::cont::Algorithm::ScanExclusive(points, cellOffsets);
    PointIds.Allocate(totalPoints);

    viskores::worklet::DispatcherMapTopology<detail::Pointify>(detail::Pointify())
      .Invoke(cellsExplicit, cellOffsets, this->PointIds);
  }
  else if (cells.CanConvert<SingleType>())
  {
    SingleType pointCells = cells.AsCellSet<SingleType>();
    viskores::UInt8 shape_id = pointCells.GetCellShape(0);
    if (shape_id == viskores::CELL_SHAPE_VERTEX)
    {
      this->PointIds.Allocate(numCells);
      viskores::worklet::DispatcherMapField<detail::Iterator>(detail::Iterator())
        .Invoke(this->PointIds);
    }
  }
}

void SphereExtractor::SetVaryingRadius(const viskores::Float32 minRadius,
                                       const viskores::Float32 maxRadius,
                                       const viskores::cont::Field& field)
{
  viskores::cont::ArrayHandle<viskores::Range> rangeArray = field.GetRange();
  if (rangeArray.GetNumberOfValues() != 1)
  {
    throw viskores::cont::ErrorBadValue("Sphere Extractor: scalar field must have one component");
  }

  viskores::Range range = rangeArray.ReadPortal().Get(0);

  Radii.Allocate(this->PointIds.GetNumberOfValues());
  viskores::worklet::DispatcherMapField<detail::FieldRadius>(
    detail::FieldRadius(minRadius, maxRadius, range))
    .Invoke(
      this->PointIds, this->Radii, viskores::rendering::raytracing::GetScalarFieldArray(field));
}

viskores::cont::ArrayHandle<viskores::Id> SphereExtractor::GetPointIds()
{
  return this->PointIds;
}

viskores::cont::ArrayHandle<viskores::Float32> SphereExtractor::GetRadii()
{
  return this->Radii;
}

viskores::Id SphereExtractor::GetNumberOfSpheres() const
{
  return this->PointIds.GetNumberOfValues();
}
}
}
} //namespace viskores::rendering::raytracing
