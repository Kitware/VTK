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

#include <viskores/rendering/MapperGlyphVector.h>

#include <viskores/cont/Timer.h>

#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/GlyphExtractorVector.h>
#include <viskores/rendering/raytracing/GlyphIntersectorVector.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/RayTracer.h>
#include <viskores/rendering/raytracing/SphereExtractor.h>
#include <viskores/rendering/raytracing/SphereIntersector.h>

namespace viskores
{
namespace rendering
{

MapperGlyphVector::MapperGlyphVector()
  : MapperGlyphBase()
  , GlyphType(viskores::rendering::GlyphType::Arrow)
{
}

MapperGlyphVector::~MapperGlyphVector() {}

viskores::rendering::GlyphType MapperGlyphVector::GetGlyphType() const
{
  return this->GlyphType;
}

void MapperGlyphVector::SetGlyphType(viskores::rendering::GlyphType glyphType)
{
  if (!(glyphType == viskores::rendering::GlyphType::Arrow))
  {
    throw viskores::cont::ErrorBadValue("MapperGlyphVector: bad glyph type");
  }

  this->GlyphType = glyphType;
}

void MapperGlyphVector::RenderCellsImpl(
  const viskores::cont::UnknownCellSet& cellset,
  const viskores::cont::CoordinateSystem& coords,
  const viskores::cont::Field& field,
  const viskores::cont::ColorTable& viskoresNotUsed(colorTable),
  const viskores::rendering::Camera& camera,
  const viskores::Range& viskoresNotUsed(fieldRange),
  const viskores::cont::Field& viskoresNotUsed(ghostField))
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();

  viskores::rendering::raytracing::RayTracer tracer;
  tracer.Clear();

  logger->OpenLogEntry("mapper_glyph_vector");
  viskores::cont::Timer tot_timer;
  tot_timer.Start();
  viskores::cont::Timer timer;

  viskores::Bounds coordBounds = coords.GetBounds();
  viskores::Float32 baseSize = this->BaseSize;
  // The weird formulation of this condition is to handle NaN correctly.
  if (!(baseSize > 0))
  {
    // set a default size
    viskores::Float64 lx = coordBounds.X.Length();
    viskores::Float64 ly = coordBounds.Y.Length();
    viskores::Float64 lz = coordBounds.Z.Length();
    viskores::Float64 mag = viskores::Sqrt(lx * lx + ly * ly + lz * lz);
    // same as used in vtk ospray
    constexpr viskores::Float64 heuristic = 500.;
    baseSize = static_cast<viskores::Float32>(mag / heuristic);
  }

  viskores::rendering::raytracing::GlyphExtractorVector glyphExtractor;

  viskores::cont::DataSet processedDataSet = this->FilterPoints(cellset, coords, field);
  viskores::cont::UnknownCellSet processedCellSet = processedDataSet.GetCellSet();
  viskores::cont::CoordinateSystem processedCoords = processedDataSet.GetCoordinateSystem();
  viskores::cont::Field processedField = processedDataSet.GetField(field.GetName());

  if (this->ScaleByValue)
  {
    viskores::Float32 minSize = baseSize - baseSize * this->ScaleDelta;
    viskores::Float32 maxSize = baseSize + baseSize * this->ScaleDelta;
    if (this->Association == viskores::cont::Field::Association::Points)
    {
      glyphExtractor.ExtractCoordinates(processedCoords, processedField, minSize, maxSize);
    }
    else // this->Association == viskores::cont::Field::Association::Cells
    {
      glyphExtractor.ExtractCells(processedCellSet, processedField, minSize, maxSize);
    }
  }
  else
  {
    if (this->Association == viskores::cont::Field::Association::Points)
    {
      glyphExtractor.ExtractCoordinates(processedCoords, processedField, baseSize);
    }
    else // this->Association == viskores::cont::Field::Association::Cells
    {
      glyphExtractor.ExtractCells(processedCellSet, processedField, baseSize);
    }
  }

  viskores::Bounds shapeBounds;
  if (glyphExtractor.GetNumberOfGlyphs() > 0)
  {
    auto glyphIntersector = std::make_shared<raytracing::GlyphIntersectorVector>(this->GlyphType);
    if (this->GlyphType == viskores::rendering::GlyphType::Arrow)
    {
      viskores::Float32 arrowBodyRadius = 0.08f * baseSize;
      viskores::Float32 arrowHeadRadius = 0.16f * baseSize;
      glyphIntersector->SetArrowRadii(arrowBodyRadius, arrowHeadRadius);
    }
    glyphIntersector->SetData(
      processedCoords, glyphExtractor.GetPointIds(), glyphExtractor.GetSizes());

    tracer.AddShapeIntersector(glyphIntersector);
    shapeBounds.Include(glyphIntersector->GetShapeBounds());
  }

  //
  // Create rays
  //
  viskores::Int32 width = (viskores::Int32)this->Canvas->GetWidth();
  viskores::Int32 height = (viskores::Int32)this->Canvas->GetHeight();

  viskores::rendering::raytracing::Camera RayCamera;
  viskores::rendering::raytracing::Ray<viskores::Float32> Rays;

  RayCamera.SetParameters(camera, width, height);

  RayCamera.CreateRays(Rays, shapeBounds);
  Rays.Buffers.at(0).InitConst(0.f);
  raytracing::RayOperations::MapCanvasToRays(Rays, camera, *this->Canvas);

  auto magnitudeField = glyphExtractor.GetMagnitudeField();
  auto magnitudeFieldRange = magnitudeField.GetRange().ReadPortal().Get(0);
  tracer.SetField(magnitudeField, magnitudeFieldRange);
  tracer.GetCamera() = RayCamera;
  tracer.SetColorMap(this->ColorMap);
  tracer.Render(Rays);

  timer.Start();
  this->Canvas->WriteToCanvas(Rays, Rays.Buffers.at(0).Buffer, camera);

  if (this->CompositeBackground)
  {
    this->Canvas->BlendBackground();
  }

  viskores::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);
}

viskores::rendering::Mapper* MapperGlyphVector::NewCopy() const
{
  return new viskores::rendering::MapperGlyphVector(*this);
}
}
} // viskores::rendering
