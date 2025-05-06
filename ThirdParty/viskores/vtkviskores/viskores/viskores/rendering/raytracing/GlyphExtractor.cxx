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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/rendering/raytracing/GlyphExtractor.h>
#include <viskores/rendering/raytracing/RayTracingTypeDefs.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

namespace
{

class CountPoints : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  VISKORES_CONT
  CountPoints() {}
  typedef void ControlSignature(CellSetIn cellset, FieldOut);
  typedef void ExecutionSignature(CellShape, _2);

  template <typename ShapeType>
  VISKORES_EXEC void operator()(ShapeType shape, viskores::Id& points) const
  {
    points = (shape.Id == viskores::CELL_SHAPE_VERTEX) ? 1 : 0;
  }
}; // class CountPoints

class Pointify : public viskores::worklet::WorkletVisitCellsWithPoints
{

public:
  VISKORES_CONT
  Pointify() {}
  typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
  typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

  template <typename ShapeType, typename VecType, typename OutputPortal>
  VISKORES_EXEC void operator()(const viskores::Id& pointOffset,
                                ShapeType shape,
                                const VecType& viskoresNotUsed(cellIndices),
                                const viskores::Id& cellId,
                                OutputPortal& outputIndices) const
  {
    if (shape.Id == viskores::CELL_SHAPE_VERTEX)
    {
      outputIndices.Set(pointOffset, cellId);
    }
  }
}; //class Pointify

class GetFieldSize : public viskores::worklet::WorkletMapField
{
protected:
  // viskores::Float64 is used to handle field values that are very small or very large
  // and could loose precision if viskores::Float32 is used.
  viskores::Float64 MinSize;
  viskores::Float64 SizeDelta;
  viskores::Float64 MinValue;
  viskores::Float64 InverseDelta;

public:
  VISKORES_CONT
  GetFieldSize(viskores::Float64 minSize, viskores::Float64 maxSize, viskores::Range scalarRange)
    : MinSize(minSize)
    , SizeDelta(maxSize - minSize)
    , MinValue(scalarRange.Min)
  {
    viskores::Float64 delta = scalarRange.Max - scalarRange.Min;
    if (delta != 0.)
      InverseDelta = 1. / (delta);
    else
      InverseDelta = 0.; // just map scalar to 0;
  }

  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename ScalarPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& pointId,
                                viskores::Float32& size,
                                const ScalarPortalType& scalars) const
  {
    viskores::Float64 scalar = viskores::Float64(scalars.Get(pointId));
    viskores::Float64 t = (scalar - this->MinValue) * this->InverseDelta;
    size = static_cast<viskores::Float32>(this->MinSize + t * this->SizeDelta);
  }

}; //class GetFieldSize

} //namespace

GlyphExtractor::GlyphExtractor() = default;

void GlyphExtractor::ExtractCoordinates(const viskores::cont::CoordinateSystem& coords,
                                        const viskores::Float32 size)
{
  this->SetPointIdsFromCoords(coords);
  this->SetUniformSize(size);
}

void GlyphExtractor::ExtractCoordinates(const viskores::cont::CoordinateSystem& coords,
                                        const viskores::cont::Field& field,
                                        const viskores::Float32 minSize,
                                        const viskores::Float32 maxSize)
{
  this->SetPointIdsFromCoords(coords);
  this->SetVaryingSize(minSize, maxSize, field);
}

void GlyphExtractor::ExtractCells(const viskores::cont::UnknownCellSet& cells,
                                  const viskores::Float32 size)
{
  this->SetPointIdsFromCells(cells);
  this->SetUniformSize(size);
}
void GlyphExtractor::ExtractCells(const viskores::cont::UnknownCellSet& cells,
                                  const viskores::cont::Field& field,
                                  const viskores::Float32 minSize,
                                  const viskores::Float32 maxSize)
{
  this->SetPointIdsFromCells(cells);
  this->SetVaryingSize(minSize, maxSize, field);
}

void GlyphExtractor::SetUniformSize(const viskores::Float32 size)
{
  const viskores::Id numValues = this->PointIds.GetNumberOfValues();
  Sizes.AllocateAndFill(numValues, size);
}

void GlyphExtractor::SetPointIdsFromCoords(const viskores::cont::CoordinateSystem& coords)
{
  viskores::Id size = coords.GetNumberOfPoints();
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(size), this->PointIds);
}

void GlyphExtractor::SetPointIdsFromCells(const viskores::cont::UnknownCellSet& cells)
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
    viskores::worklet::DispatcherMapTopology<CountPoints>(CountPoints())
      .Invoke(cellsExplicit, points);

    viskores::Id totalPoints = 0;
    totalPoints = viskores::cont::Algorithm::Reduce(points, viskores::Id(0));

    viskores::cont::ArrayHandle<viskores::Id> cellOffsets;
    viskores::cont::Algorithm::ScanExclusive(points, cellOffsets);
    PointIds.Allocate(totalPoints);

    viskores::worklet::DispatcherMapTopology<Pointify>(Pointify())
      .Invoke(cellsExplicit, cellOffsets, this->PointIds);
  }
  else if (cells.CanConvert<SingleType>())
  {
    SingleType pointCells = cells.AsCellSet<SingleType>();
    viskores::UInt8 shape_id = pointCells.GetCellShape(0);
    if (shape_id == viskores::CELL_SHAPE_VERTEX)
    {
      viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(numCells), this->PointIds);
    }
  }
}

void GlyphExtractor::SetVaryingSize(const viskores::Float32 minSize,
                                    const viskores::Float32 maxSize,
                                    const viskores::cont::Field& field)
{
  viskores::cont::ArrayHandle<viskores::Range> rangeArray = field.GetRange();
  if (rangeArray.GetNumberOfValues() != 1)
  {
    throw viskores::cont::ErrorBadValue("Glyph Extractor: scalar field must have one component");
  }

  viskores::Range range = rangeArray.ReadPortal().Get(0);

  Sizes.Allocate(this->PointIds.GetNumberOfValues());
  viskores::worklet::DispatcherMapField<GetFieldSize>(GetFieldSize(minSize, maxSize, range))
    .Invoke(
      this->PointIds, this->Sizes, viskores::rendering::raytracing::GetScalarFieldArray(field));
}

viskores::cont::ArrayHandle<viskores::Id> GlyphExtractor::GetPointIds()
{
  return this->PointIds;
}

viskores::cont::ArrayHandle<viskores::Float32> GlyphExtractor::GetSizes()
{
  return this->Sizes;
}

viskores::Id GlyphExtractor::GetNumberOfGlyphs() const
{
  return this->PointIds.GetNumberOfValues();
}
}
}
} //namespace viskores::rendering::raytracing
