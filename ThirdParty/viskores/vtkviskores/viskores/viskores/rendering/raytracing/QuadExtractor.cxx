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

#include <viskores/rendering/raytracing/QuadExtractor.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/rendering/Quadralizer.h>
#include <viskores/rendering/raytracing/Worklets.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

class CountQuads : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  VISKORES_CONT
  CountQuads() {}
  typedef void ControlSignature(CellSetIn cellset, FieldOut);
  typedef void ExecutionSignature(CellShape, _2);

  VISKORES_EXEC
  void operator()(viskores::CellShapeTagGeneric shapeType, viskores::Id& quads) const
  {
    if (shapeType.Id == viskores::CELL_SHAPE_QUAD)
      quads = 1;
    else
      quads = 0;
  }

  VISKORES_EXEC
  void operator()(viskores::CellShapeTagHexahedron viskoresNotUsed(shapeType),
                  viskores::Id& quads) const
  {
    quads = 6;
  }

  VISKORES_EXEC
  void operator()(viskores::CellShapeTagQuad viskoresNotUsed(shapeType), viskores::Id& points) const
  {
    points = 1;
  }
  VISKORES_EXEC
  void operator()(viskores::CellShapeTagWedge viskoresNotUsed(shapeType),
                  viskores::Id& points) const
  {
    points = 0;
  }

}; // ClassCountquads

class Pointify : public viskores::worklet::WorkletVisitCellsWithPoints
{

public:
  VISKORES_CONT
  Pointify() {}
  typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
  typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

  template <typename VecType, typename OutputPortal>
  VISKORES_EXEC void cell2quad(viskores::Id& offset,
                               const VecType& cellIndices,
                               const viskores::Id& cellId,
                               const viskores::Id Id0,
                               const viskores::Id Id1,
                               const viskores::Id Id2,
                               const viskores::Id Id3,
                               OutputPortal& outputIndices) const
  {
    viskores::Vec<viskores::Id, 5> quad;
    quad[0] = cellId;
    quad[1] = static_cast<viskores::Id>(cellIndices[viskores::IdComponent(Id0)]);
    quad[2] = static_cast<viskores::Id>(cellIndices[viskores::IdComponent(Id1)]);
    quad[3] = static_cast<viskores::Id>(cellIndices[viskores::IdComponent(Id2)]);
    quad[4] = static_cast<viskores::Id>(cellIndices[viskores::IdComponent(Id3)]);
    outputIndices.Set(offset++, quad);
  }

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
  VISKORES_EXEC void operator()(const viskores::Id& pointOffset,
                                viskores::CellShapeTagHexahedron viskoresNotUsed(shapeType),
                                const VecType& cellIndices,
                                const viskores::Id& cellId,
                                OutputPortal& outputIndices) const

  {
    viskores::Id offset = pointOffset;
    cell2quad(offset, cellIndices, cellId, 0, 1, 5, 4, outputIndices);
    cell2quad(offset, cellIndices, cellId, 1, 2, 6, 5, outputIndices);
    cell2quad(offset, cellIndices, cellId, 3, 7, 6, 2, outputIndices);
    cell2quad(offset, cellIndices, cellId, 0, 4, 7, 3, outputIndices);
    cell2quad(offset, cellIndices, cellId, 0, 3, 2, 1, outputIndices);
    cell2quad(offset, cellIndices, cellId, 4, 5, 6, 7, outputIndices);
  }

  template <typename VecType, typename OutputPortal>
  VISKORES_EXEC void operator()(const viskores::Id& pointOffset,
                                viskores::CellShapeTagGeneric shapeType,
                                const VecType& cellIndices,
                                const viskores::Id& cellId,
                                OutputPortal& outputIndices) const
  {

    if (shapeType.Id == viskores::CELL_SHAPE_QUAD)
    {
      viskores::Vec<viskores::Id, 5> quad;
      quad[0] = cellId;
      quad[1] = cellIndices[0];
      quad[2] = cellIndices[1];
      quad[3] = cellIndices[2];
      quad[4] = cellIndices[3];
      outputIndices.Set(pointOffset, quad);
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
  void operator()(viskores::Id2& index, const viskores::Id2& idx) const { index = idx; }
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

void QuadExtractor::ExtractCells(const viskores::cont::UnknownCellSet& cells)
{
  viskores::Id numOfQuads;
  viskores::rendering::Quadralizer quadrizer;
  quadrizer.Run(cells, this->QuadIds, numOfQuads);

  //this->SetPointIdsFromCells(cells);
}


void QuadExtractor::SetQuadIdsFromCells(const viskores::cont::UnknownCellSet& cells)
{
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
    viskores::worklet::DispatcherMapTopology<detail::CountQuads>(detail::CountQuads())
      .Invoke(cellsExplicit, points);

    viskores::Id total = 0;
    total = viskores::cont::Algorithm::Reduce(points, viskores::Id(0));

    viskores::cont::ArrayHandle<viskores::Id> cellOffsets;
    viskores::cont::Algorithm::ScanExclusive(points, cellOffsets);
    QuadIds.Allocate(total);

    viskores::worklet::DispatcherMapTopology<detail::Pointify>(detail::Pointify())
      .Invoke(cellsExplicit, cellOffsets, this->QuadIds);
  }
}


viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 5>> QuadExtractor::GetQuadIds()
{
  return this->QuadIds;
}


viskores::Id QuadExtractor::GetNumberOfQuads() const
{
  return this->QuadIds.GetNumberOfValues();
}
}
}
} //namespace viskores::rendering::raytracing
