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

#include <viskores/rendering/MapperGlyphBase.h>

#include <viskores/filter/entity_extraction/MaskPoints.h>
#include <viskores/rendering/CanvasRayTracer.h>

namespace viskores
{
namespace rendering
{

MapperGlyphBase::MapperGlyphBase() {}

MapperGlyphBase::~MapperGlyphBase() {}

void MapperGlyphBase::SetCanvas(viskores::rendering::Canvas* canvas)
{
  viskores::rendering::CanvasRayTracer* canvasRT =
    dynamic_cast<viskores::rendering::CanvasRayTracer*>(canvas);
  if (canvasRT == nullptr)
  {
    throw viskores::cont::ErrorBadValue(
      "MapperGlyphBase: bad canvas type. Must be CanvasRayTracer");
  }

  this->Canvas = canvasRT;
}

viskores::rendering::Canvas* MapperGlyphBase::GetCanvas() const
{
  return this->Canvas;
}

viskores::cont::Field::Association MapperGlyphBase::GetAssociation() const
{
  return this->Association;
}

void MapperGlyphBase::SetAssociation(viskores::cont::Field::Association association)
{
  switch (association)
  {
    case viskores::cont::Field::Association::Cells:
    case viskores::cont::Field::Association::Points:
      this->Association = association;
      break;
    default:
      throw viskores::cont::ErrorBadValue("Invalid glyph association.");
  }
}

bool MapperGlyphBase::GetUseCells() const
{
  return this->Association == viskores::cont::Field::Association::Cells;
}

void MapperGlyphBase::SetUseCells()
{
  this->SetAssociation(viskores::cont::Field::Association::Cells);
}

bool MapperGlyphBase::GetUsePoints() const
{
  return this->Association == viskores::cont::Field::Association::Points;
}

void MapperGlyphBase::SetUsePoints()
{
  this->SetAssociation(viskores::cont::Field::Association::Points);
}

bool MapperGlyphBase::GetUseNodes() const
{
  return this->GetUsePoints();
}

void MapperGlyphBase::SetUseNodes()
{
  this->SetUsePoints();
}

viskores::Float32 MapperGlyphBase::GetBaseSize() const
{
  return this->BaseSize;
}

void MapperGlyphBase::SetBaseSize(viskores::Float32 size)
{
  if (size <= 0.f)
  {
    throw viskores::cont::ErrorBadValue("MapperGlyphBase: base size must be positive");
  }
  this->BaseSize = size;
}

bool MapperGlyphBase::GetScaleByValue() const
{
  return this->ScaleByValue;
}

void MapperGlyphBase::SetScaleByValue(bool on)
{
  this->ScaleByValue = on;
}

viskores::Float32 MapperGlyphBase::GetScaleDelta() const
{
  return this->ScaleDelta;
}

void MapperGlyphBase::SetScaleDelta(viskores::Float32 delta)
{
  if (delta < 0.0f)
  {
    throw viskores::cont::ErrorBadValue("MapperGlyphBase: scale delta must be non-negative");
  }

  this->ScaleDelta = delta;
}

bool MapperGlyphBase::GetUseStride() const
{
  return this->UseStride;
}

void MapperGlyphBase::SetUseStride(bool on)
{
  this->UseStride = on;
}

viskores::Id MapperGlyphBase::GetStride() const
{
  return this->Stride;
}

void MapperGlyphBase::SetStride(viskores::Id stride)
{
  if (stride < 1)
  {
    throw viskores::cont::ErrorBadValue("MapperGlyphBase: stride must be positive");
  }
  this->Stride = stride;
}

void MapperGlyphBase::SetCompositeBackground(bool on)
{
  this->CompositeBackground = on;
}

viskores::cont::DataSet MapperGlyphBase::FilterPoints(
  const viskores::cont::UnknownCellSet& cellSet,
  const viskores::cont::CoordinateSystem& coords,
  const viskores::cont::Field& field) const
{
  viskores::cont::DataSet result;
  result.SetCellSet(cellSet);
  result.AddCoordinateSystem(coords);
  result.AddField(field);

  if (this->UseStride)
  {
    viskores::filter::entity_extraction::MaskPoints pointMasker;
    pointMasker.SetCompactPoints(true);
    pointMasker.SetStride(this->Stride);
    result = pointMasker.Execute(result);
  }

  return result;
}

}
} // namespace viskores::rendering
