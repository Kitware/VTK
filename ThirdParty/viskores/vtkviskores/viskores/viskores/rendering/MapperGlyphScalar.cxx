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

#include <viskores/rendering/MapperGlyphScalar.h>

#include <viskores/cont/Timer.h>

#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/GlyphExtractor.h>
#include <viskores/rendering/raytracing/GlyphIntersector.h>
#include <viskores/rendering/raytracing/Logger.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/RayTracer.h>

namespace viskores
{
namespace rendering
{
namespace
{
// Packed frame buffer value with color set as black and depth as 1.0f
constexpr viskores::Int64 ClearValue = 0x3F800000000000FF;

union PackedValue
{
  struct PackedFloats
  {
    viskores::Float32 Color;
    viskores::Float32 Depth;
  } Floats;
  struct PackedInts
  {
    viskores::UInt32 Color;
    viskores::UInt32 Depth;
  } Ints;
  viskores::Int64 Raw;
}; // union PackedValue

VISKORES_EXEC_CONT
viskores::UInt32 ScaleColorComponent(viskores::Float32 c)
{
  viskores::Int32 t = viskores::Int32(c * 256.0f);
  return viskores::UInt32(t < 0 ? 0 : (t > 255 ? 255 : t));
}

VISKORES_EXEC_CONT
viskores::UInt32 PackColor(viskores::Float32 r,
                           viskores::Float32 g,
                           viskores::Float32 b,
                           viskores::Float32 a);

VISKORES_EXEC_CONT
viskores::UInt32 PackColor(const viskores::Vec4f_32& color)
{
  return PackColor(color[0], color[1], color[2], color[3]);
}

VISKORES_EXEC_CONT
viskores::UInt32 PackColor(viskores::Float32 r,
                           viskores::Float32 g,
                           viskores::Float32 b,
                           viskores::Float32 a)
{
  viskores::UInt32 packed = (ScaleColorComponent(r) << 24);
  packed |= (ScaleColorComponent(g) << 16);
  packed |= (ScaleColorComponent(b) << 8);
  packed |= ScaleColorComponent(a);
  return packed;
}

VISKORES_EXEC_CONT
void UnpackColor(viskores::UInt32 color,
                 viskores::Float32& r,
                 viskores::Float32& g,
                 viskores::Float32& b,
                 viskores::Float32& a);

VISKORES_EXEC_CONT
void UnpackColor(viskores::UInt32 packedColor, viskores::Vec4f_32& color)
{
  UnpackColor(packedColor, color[0], color[1], color[2], color[3]);
}

VISKORES_EXEC_CONT
void UnpackColor(viskores::UInt32 color,
                 viskores::Float32& r,
                 viskores::Float32& g,
                 viskores::Float32& b,
                 viskores::Float32& a)
{
  r = viskores::Float32((color & 0xFF000000) >> 24) / 255.0f;
  g = viskores::Float32((color & 0x00FF0000) >> 16) / 255.0f;
  b = viskores::Float32((color & 0x0000FF00) >> 8) / 255.0f;
  a = viskores::Float32((color & 0x000000FF)) / 255.0f;
}

class PackIntoFrameBuffer : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  VISKORES_CONT
  PackIntoFrameBuffer() {}

  VISKORES_EXEC
  void operator()(const viskores::Vec4f_32& color,
                  const viskores::Float32& depth,
                  viskores::Int64& outValue) const
  {
    PackedValue packed;
    packed.Ints.Color = PackColor(color);
    packed.Floats.Depth = depth;
    outValue = packed.Raw;
  }
}; //class PackIntoFrameBuffer

class UnpackFromFrameBuffer : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  UnpackFromFrameBuffer() {}

  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, WorkIndex);

  template <typename ColorBufferPortal, typename DepthBufferPortal>
  VISKORES_EXEC void operator()(const viskores::Int64& packedValue,
                                ColorBufferPortal& colorBuffer,
                                DepthBufferPortal& depthBuffer,
                                const viskores::Id& index) const
  {
    PackedValue packed;
    packed.Raw = packedValue;
    float depth = packed.Floats.Depth;
    if (depth <= depthBuffer.Get(index))
    {
      viskores::Vec4f_32 color;
      UnpackColor(packed.Ints.Color, color);
      colorBuffer.Set(index, color);
      depthBuffer.Set(index, depth);
    }
  }
}; // class UnpackFromFrameBuffer

class GetNormalizedScalars : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn pointIds, FieldOut normalizedScalars, WholeArrayIn field);
  using ExecutionSignature = void(_1, _2, _3);

  VISKORES_CONT GetNormalizedScalars(viskores::Float32 minScalar, viskores::Float32 maxScalar)
    : MinScalar(minScalar)
  {
    if (minScalar >= maxScalar)
    {
      this->InverseScalarDelta = 0.0f;
    }
    else
    {
      this->InverseScalarDelta = 1.0f / (maxScalar - minScalar);
    }
  }

  template <typename FieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& pointId,
                                viskores::Float32& normalizedScalar,
                                const FieldPortalType& field) const
  {
    normalizedScalar = static_cast<viskores::Float32>(field.Get(pointId));
    normalizedScalar = (normalizedScalar - this->MinScalar) * this->InverseScalarDelta;
  }

private:
  viskores::Float32 MinScalar;
  viskores::Float32 InverseScalarDelta;
}; // class GetNormalizedScalars

class BillboardGlyphPlotter : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn pointIds,
                                FieldIn sizes,
                                FieldIn normalizedScalar,
                                WholeArrayIn coords,
                                WholeArrayIn colorMap,
                                AtomicArrayInOut frameBuffer);

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);

  VISKORES_CONT
  BillboardGlyphPlotter(const viskores::Matrix<viskores::Float32, 4, 4>& worldToProjection,
                        viskores::Id width,
                        viskores::Id height,
                        viskores::Float32 projectionOffset)
    : WorldToProjection(worldToProjection)
    , Width(width)
    , Height(height)
    , ProjectionOffset(projectionOffset)
  {
  }

  template <typename CoordinatesPortal, typename ColorMapPortal, typename FrameBuffer>
  VISKORES_EXEC void operator()(const viskores::Id& pointId,
                                const viskores::Float32& size,
                                const viskores::Float32& normalizedScalar,
                                const CoordinatesPortal& coordsPortal,
                                const ColorMapPortal& colorMap,
                                FrameBuffer& frameBuffer) const
  {
    viskores::Vec3f_32 point = static_cast<viskores::Vec3f_32>(coordsPortal.Get(pointId));
    point = this->TransformWorldToViewport(point);
    viskores::Vec4f_32 color = this->GetColor(normalizedScalar, colorMap);

    viskores::Float32 halfSize = size / 2.0f;
    viskores::Id x1 = static_cast<viskores::Id>(viskores::Round(point[0] - halfSize));
    viskores::Id x2 = static_cast<viskores::Id>(viskores::Round(point[0] + halfSize));
    viskores::Id y1 = static_cast<viskores::Id>(viskores::Round(point[1] - halfSize));
    viskores::Id y2 = static_cast<viskores::Id>(viskores::Round(point[1] + halfSize));
    viskores::Float32 depth = point[2];

    for (viskores::Id x = x1; x <= x2; ++x)
    {
      for (viskores::Id y = y1; y <= y2; ++y)
      {
        this->SetColor(x, y, depth, color, frameBuffer);
      }
    }
  }

private:
  VISKORES_EXEC
  viskores::Vec3f_32 TransformWorldToViewport(const viskores::Vec3f_32& point) const
  {
    viskores::Vec4f_32 temp(point[0], point[1], point[2], 1.0f);
    viskores::Vec3f_32 result;
    temp = viskores::MatrixMultiply(this->WorldToProjection, temp);
    for (viskores::IdComponent i = 0; i < 3; ++i)
    {
      result[i] = temp[i] / temp[3];
    }
    result[0] = (result[0] * 0.5f + 0.5f) * viskores::Float32(this->Width);
    result[1] = (result[1] * 0.5f + 0.5f) * viskores::Float32(this->Height);
    result[2] = result[2] * 0.5f + 0.5f;
    // Offset the point to a bit towards the camera. This is to ensure that the front faces of
    // the wireframe wins the z-depth check against the surface render, and is in addition to the
    // existing camera space offset.
    result[2] -= this->ProjectionOffset;
    return result;
  }

  template <typename ColorMapPortal>
  VISKORES_EXEC viskores::Vec4f_32 GetColor(viskores::Float32 normalizedScalar,
                                            const ColorMapPortal& colorMap) const
  {
    viskores::Id colorMapSize = colorMap.GetNumberOfValues() - 1;
    viskores::Id colorIdx = static_cast<viskores::Id>(normalizedScalar * colorMapSize);
    colorIdx = viskores::Min(colorMapSize, viskores::Max(viskores::Id(0), colorIdx));
    return colorMap.Get(colorIdx);
  }

  template <typename FrameBuffer>
  VISKORES_EXEC void SetColor(viskores::Id x,
                              viskores::Id y,
                              viskores::Float32 depth,
                              const viskores::Vec4f_32& color,
                              FrameBuffer& frameBuffer) const
  {
    if (x < 0 || x >= this->Width || y < 0 || y >= this->Height)
    {
      return;
    }

    viskores::Id index = y * this->Width + x;
    PackedValue current, next;
    current.Raw = ClearValue;
    next.Floats.Depth = depth;

    viskores::Vec4f_32 currentColor;
    do
    {
      UnpackColor(current.Ints.Color, currentColor);
      next.Ints.Color = PackColor(color);
      frameBuffer.CompareExchange(index, &current.Raw, next.Raw);
    } while (current.Floats.Depth > next.Floats.Depth);
  }

  const viskores::Matrix<viskores::Float32, 4, 4> WorldToProjection;
  const viskores::Id Width;
  const viskores::Id Height;
  const viskores::Float32 ProjectionOffset;
}; // class BillboardGlyphPlotter

}

MapperGlyphScalar::MapperGlyphScalar()
  : MapperGlyphBase()
  , GlyphType(viskores::rendering::GlyphType::Sphere)
{
}

MapperGlyphScalar::~MapperGlyphScalar() {}

viskores::rendering::GlyphType MapperGlyphScalar::GetGlyphType() const
{
  return this->GlyphType;
}

void MapperGlyphScalar::SetGlyphType(viskores::rendering::GlyphType glyphType)
{
  if (!(glyphType == viskores::rendering::GlyphType::Axes ||
        glyphType == viskores::rendering::GlyphType::Cube ||
        glyphType == viskores::rendering::GlyphType::Quad ||
        glyphType == viskores::rendering::GlyphType::Sphere))
  {
    throw viskores::cont::ErrorBadValue("MapperGlyphScalar: bad glyph type");
  }

  this->GlyphType = glyphType;
}

void MapperGlyphScalar::RenderCellsImpl(
  const viskores::cont::UnknownCellSet& cellset,
  const viskores::cont::CoordinateSystem& coords,
  const viskores::cont::Field& scalarField,
  const viskores::cont::ColorTable& viskoresNotUsed(colorTable),
  const viskores::rendering::Camera& camera,
  const viskores::Range& scalarRange,
  const viskores::cont::Field& viskoresNotUsed(ghostField))
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();

  viskores::rendering::raytracing::RayTracer tracer;
  tracer.Clear();

  logger->OpenLogEntry("mapper_glyph_scalar");
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
    if (this->GlyphType == viskores::rendering::GlyphType::Quad)
    {
      baseSize = 20.0f;
    }
    else
    {
      // same as used in vtk ospray
      constexpr viskores::Float64 heuristic = 500.;
      baseSize = static_cast<viskores::Float32>(mag / heuristic);
    }
  }

  viskores::rendering::raytracing::GlyphExtractor glyphExtractor;

  viskores::cont::DataSet processedDataSet = this->FilterPoints(cellset, coords, scalarField);
  viskores::cont::UnknownCellSet processedCellSet = processedDataSet.GetCellSet();
  viskores::cont::CoordinateSystem processedCoords = processedDataSet.GetCoordinateSystem();
  viskores::cont::Field processedField = processedDataSet.GetField(scalarField.GetName());

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
      glyphExtractor.ExtractCoordinates(processedCoords, baseSize);
    }
    else // this->Association == viskores::cont::Field::Association::Cells
    {
      glyphExtractor.ExtractCells(processedCellSet, baseSize);
    }
  }

  if (this->GlyphType == viskores::rendering::GlyphType::Quad)
  {
    viskores::cont::ArrayHandle<viskores::Id> pointIds = glyphExtractor.GetPointIds();
    viskores::cont::ArrayHandle<viskores::Float32> sizes = glyphExtractor.GetSizes();

    viskores::cont::ArrayHandle<viskores::Float32> normalizedScalars;
    viskores::Float32 rangeMin = static_cast<viskores::Float32>(scalarRange.Min);
    viskores::Float32 rangeMax = static_cast<viskores::Float32>(scalarRange.Max);
    viskores::cont::Invoker invoker;
    invoker(GetNormalizedScalars{ rangeMin, rangeMax },
            pointIds,
            normalizedScalars,
            viskores::rendering::raytracing::GetScalarFieldArray(scalarField));

    viskores::cont::ArrayHandle<viskores::Int64> frameBuffer;
    invoker(PackIntoFrameBuffer{},
            this->Canvas->GetColorBuffer(),
            this->Canvas->GetDepthBuffer(),
            frameBuffer);

    viskores::Range clippingRange = camera.GetClippingRange();
    viskores::Float64 offset1 = (clippingRange.Max - clippingRange.Min) / 1.0e4;
    viskores::Float64 offset2 = clippingRange.Min / 2.0;
    viskores::Float32 offset = static_cast<viskores::Float32>(viskores::Min(offset1, offset2));
    viskores::Matrix<viskores::Float32, 4, 4> modelMatrix;
    viskores::MatrixIdentity(modelMatrix);
    modelMatrix[2][3] = offset;
    viskores::Matrix<viskores::Float32, 4, 4> worldToCamera =
      viskores::MatrixMultiply(modelMatrix, camera.CreateViewMatrix());
    viskores::Matrix<viskores::Float32, 4, 4> worldToProjection = viskores::MatrixMultiply(
      camera.CreateProjectionMatrix(this->Canvas->GetWidth(), this->Canvas->GetHeight()),
      worldToCamera);
    viskores::Float32 projectionOffset = viskores::Max(
      0.03f / static_cast<viskores::Float32>(camera.GetClippingRange().Length()), 1e-4f);
    invoker(
      BillboardGlyphPlotter{
        worldToProjection, this->Canvas->GetWidth(), this->Canvas->GetHeight(), projectionOffset },
      pointIds,
      sizes,
      normalizedScalars,
      coords,
      this->ColorMap,
      frameBuffer);

    timer.Start();
    invoker(UnpackFromFrameBuffer{},
            frameBuffer,
            this->Canvas->GetColorBuffer(),
            this->Canvas->GetDepthBuffer());
  }
  else
  {
    viskores::Bounds shapeBounds;
    if (glyphExtractor.GetNumberOfGlyphs() > 0)
    {
      auto glyphIntersector = std::make_shared<raytracing::GlyphIntersector>(this->GlyphType);
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

    tracer.SetField(processedField, scalarRange);
    tracer.GetCamera() = RayCamera;
    tracer.SetColorMap(this->ColorMap);
    tracer.Render(Rays);

    timer.Start();
    this->Canvas->WriteToCanvas(Rays, Rays.Buffers.at(0).Buffer, camera);
  }

  if (this->CompositeBackground)
  {
    this->Canvas->BlendBackground();
  }
  viskores::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);
}

viskores::rendering::Mapper* MapperGlyphScalar::NewCopy() const
{
  return new viskores::rendering::MapperGlyphScalar(*this);
}
}
} // viskores::rendering
