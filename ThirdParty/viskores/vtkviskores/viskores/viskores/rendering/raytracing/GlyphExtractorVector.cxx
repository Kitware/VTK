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

#include <viskores/VectorAnalysis.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/rendering/raytracing/GlyphExtractorVector.h>
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

struct MinFunctor
{
  template <typename VecType>
  VISKORES_EXEC VecType operator()(const VecType& x, const VecType& y) const
  {
    return (viskores::MagnitudeSquared(y) < viskores::MagnitudeSquared(x)) ? y : x;
  }
};

struct MaxFunctor
{
  template <typename VecType>
  VISKORES_EXEC VecType operator()(const VecType& x, const VecType& y) const
  {
    return (viskores::MagnitudeSquared(x) < viskores::MagnitudeSquared(y)) ? y : x;
  }
};

class GetFieldSize : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Float64 MinSize;
  viskores::Float64 SizeDelta;
  viskores::Float64 MinValueMagnitude;
  viskores::Float64 InverseDelta;

public:
  VISKORES_CONT
  GetFieldSize(viskores::Float64 minSize,
               viskores::Float64 maxSize,
               viskores::Vec3f_64 minValue,
               viskores::Vec3f_64 maxValue)
    : MinSize(minSize)
    , SizeDelta(maxSize - minSize)
  {
    MinValueMagnitude = viskores::Magnitude(minValue);
    viskores::Float64 minMag = viskores::Magnitude(minValue);
    viskores::Float64 maxMag = viskores::Magnitude(maxValue);
    viskores::Float64 delta = maxMag - minMag;
    if (delta != 0.)
      InverseDelta = 1. / (delta);
    else
      InverseDelta = 0.; // just map scalar to 0;
  }

  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename FieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& pointId,
                                viskores::Vec3f_32& size,
                                const FieldPortalType& field) const
  {
    using ValueType = typename FieldPortalType::ValueType;

    ValueType fieldVal = field.Get(pointId);
    viskores::Float64 fieldValMag = viskores::Magnitude(fieldVal);
    viskores::Normalize(fieldVal);
    viskores::Float64 t = (fieldValMag - MinValueMagnitude) * InverseDelta;
    viskores::Float64 sizeMag = MinSize + t * SizeDelta;
    viskores::Vec3f_64 tempSize = fieldVal * sizeMag;

    size[0] = static_cast<viskores::Float32>(tempSize[0]);
    size[1] = static_cast<viskores::Float32>(tempSize[1]);
    size[2] = static_cast<viskores::Float32>(tempSize[2]);
  }

}; //class GetFieldSize

class FieldMagnitude : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  FieldMagnitude() {}

  typedef void ControlSignature(FieldIn, WholeArrayIn, WholeArrayInOut);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename FieldPortalType, typename MagnitudeFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& pointId,
                                const FieldPortalType& field,
                                MagnitudeFieldPortalType& magnitudeField) const
  {
    using FieldValueType = typename FieldPortalType::ValueType;

    FieldValueType fieldVal = field.Get(pointId);
    viskores::Float32 fieldValMag = static_cast<viskores::Float32>(viskores::Magnitude(fieldVal));
    magnitudeField.Set(pointId, fieldValMag);
  }
}; //class FieldMagnitude

class UniformFieldMagnitude : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3);

  VISKORES_CONT
  UniformFieldMagnitude(viskores::Float32 uniformMagnitude)
    : UniformMagnitude(uniformMagnitude)
  {
  }

  template <typename FieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& pointId,
                                viskores::Vec3f_32& size,
                                const FieldPortalType& field) const
  {
    viskores::Vec3f_32 fieldVal = static_cast<viskores::Vec3f_32>(field.Get(pointId));
    size = viskores::Normal(fieldVal) * this->UniformMagnitude;
  }

  viskores::Float32 UniformMagnitude;
}; //class UniformFieldMagnitude

} //namespace

GlyphExtractorVector::GlyphExtractorVector() = default;

void GlyphExtractorVector::ExtractCoordinates(const viskores::cont::CoordinateSystem& coords,
                                              const viskores::cont::Field& field,
                                              const viskores::Float32 size)
{
  this->SetPointIdsFromCoords(coords);
  this->SetUniformSize(size, field);
}

void GlyphExtractorVector::ExtractCoordinates(const viskores::cont::CoordinateSystem& coords,
                                              const viskores::cont::Field& field,
                                              const viskores::Float32 minSize,
                                              const viskores::Float32 maxSize)
{
  this->SetPointIdsFromCoords(coords);
  this->SetVaryingSize(minSize, maxSize, field);
}

void GlyphExtractorVector::ExtractCells(const viskores::cont::UnknownCellSet& cells,
                                        const viskores::cont::Field& field,
                                        const viskores::Float32 size)
{
  this->SetPointIdsFromCells(cells);
  this->SetUniformSize(size, field);
}
void GlyphExtractorVector::ExtractCells(const viskores::cont::UnknownCellSet& cells,
                                        const viskores::cont::Field& field,
                                        const viskores::Float32 minSize,
                                        const viskores::Float32 maxSize)
{
  this->SetPointIdsFromCells(cells);
  this->SetVaryingSize(minSize, maxSize, field);
}

void GlyphExtractorVector::SetUniformSize(const viskores::Float32 size,
                                          const viskores::cont::Field& field)
{
  this->ExtractMagnitudeField(field);

  this->Sizes.Allocate(this->PointIds.GetNumberOfValues());
  viskores::cont::Invoker invoker;
  invoker(UniformFieldMagnitude(size),
          this->PointIds,
          this->Sizes,
          viskores::rendering::raytracing::GetVec3FieldArray(field));
}

void GlyphExtractorVector::ExtractMagnitudeField(const viskores::cont::Field& field)
{
  viskores::cont::ArrayHandle<viskores::Float32> magnitudeArray;
  magnitudeArray.Allocate(this->PointIds.GetNumberOfValues());
  viskores::worklet::DispatcherMapField<FieldMagnitude>(FieldMagnitude())
    .Invoke(
      this->PointIds, viskores::rendering::raytracing::GetVec3FieldArray(field), magnitudeArray);
  this->MagnitudeField = viskores::cont::Field(field);
  this->MagnitudeField.SetData(magnitudeArray);
}

void GlyphExtractorVector::SetPointIdsFromCoords(const viskores::cont::CoordinateSystem& coords)
{
  viskores::Id size = coords.GetNumberOfPoints();
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(size), this->PointIds);
}

void GlyphExtractorVector::SetPointIdsFromCells(const viskores::cont::UnknownCellSet& cells)
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

void GlyphExtractorVector::SetVaryingSize(const viskores::Float32 minSize,
                                          const viskores::Float32 maxSize,
                                          const viskores::cont::Field& field)
{
  viskores::cont::ArrayHandle<viskores::Range> rangeArray = field.GetRange();
  if (rangeArray.GetNumberOfValues() != 3)
  {
    throw viskores::cont::ErrorBadValue(
      "Glyph Extractor Vector: vector field must have three components");
  }

  using Vec3f_32Handle = viskores::cont::ArrayHandle<viskores::Vec3f_32>;
  using Vec3f_64Handle = viskores::cont::ArrayHandle<viskores::Vec3f_64>;
  viskores::cont::UnknownArrayHandle fieldUnknownHandle = field.GetData();
  viskores::Vec3f_32 minFieldValue, maxFieldValue;

  if (fieldUnknownHandle.CanConvert<Vec3f_64Handle>())
  {
    Vec3f_64Handle fieldArray;
    field.GetData().AsArrayHandle(fieldArray);
    viskores::Vec3f_64 initVal = viskores::cont::ArrayGetValue(0, fieldArray);
    minFieldValue = static_cast<viskores::Vec3f_32>(
      viskores::cont::Algorithm::Reduce(fieldArray, initVal, MinFunctor()));
    maxFieldValue = static_cast<viskores::Vec3f_32>(
      viskores::cont::Algorithm::Reduce(fieldArray, initVal, MaxFunctor()));
  }
  else
  {
    Vec3f_32Handle fieldArray;
    field.GetData().AsArrayHandle(fieldArray);
    viskores::Vec3f_32 initVal = viskores::cont::ArrayGetValue(0, fieldArray);
    minFieldValue = static_cast<viskores::Vec3f_32>(
      viskores::cont::Algorithm::Reduce(fieldArray, initVal, MinFunctor()));
    maxFieldValue = static_cast<viskores::Vec3f_32>(
      viskores::cont::Algorithm::Reduce(fieldArray, initVal, MaxFunctor()));
  }

  this->ExtractMagnitudeField(field);

  this->Sizes.Allocate(this->PointIds.GetNumberOfValues());
  viskores::worklet::DispatcherMapField<GetFieldSize>(
    GetFieldSize(minSize, maxSize, minFieldValue, maxFieldValue))
    .Invoke(this->PointIds, this->Sizes, viskores::rendering::raytracing::GetVec3FieldArray(field));
}

viskores::cont::ArrayHandle<viskores::Id> GlyphExtractorVector::GetPointIds()
{
  return this->PointIds;
}

viskores::cont::ArrayHandle<viskores::Vec<viskores::Float32, 3>> GlyphExtractorVector::GetSizes()
{
  return this->Sizes;
}

viskores::cont::Field GlyphExtractorVector::GetMagnitudeField()
{
  return this->MagnitudeField;
}

viskores::Id GlyphExtractorVector::GetNumberOfGlyphs() const
{
  return this->PointIds.GetNumberOfValues();
}
}
}
} //namespace viskores::rendering::raytracing
