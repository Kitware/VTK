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

#ifndef viskores_rendering_Wireframer_h
#define viskores_rendering_Wireframer_h

#include <viskores/Assert.h>
#include <viskores/Math.h>
#include <viskores/Swap.h>
#include <viskores/Types.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/AtomicArray.h>
#include <viskores/rendering/MatrixHelpers.h>
#include <viskores/rendering/Triangulator.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace rendering
{
namespace
{

using ColorMapHandle = viskores::cont::ArrayHandle<viskores::Vec4f_32>;
using IndicesHandle = viskores::cont::ArrayHandle<viskores::Id2>;
using PackedFrameBufferHandle = viskores::cont::ArrayHandle<viskores::Int64>;

// Depth value of 1.0f
const viskores::Int64 ClearDepth = 0x3F800000;
// Packed frame buffer value with color set as black and depth as 1.0f
const viskores::Int64 ClearValue = 0x3F800000000000FF;

VISKORES_EXEC_CONT
viskores::Float32 IntegerPart(viskores::Float32 x)
{
  return viskores::Floor(x);
}

VISKORES_EXEC_CONT
viskores::Float32 FractionalPart(viskores::Float32 x)
{
  return x - viskores::Floor(x);
}

VISKORES_EXEC_CONT
viskores::Float32 ReverseFractionalPart(viskores::Float32 x)
{
  return 1.0f - FractionalPart(x);
}

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

struct CopyIntoFrameBuffer : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  VISKORES_CONT
  CopyIntoFrameBuffer() {}

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
}; //struct CopyIntoFrameBuffer

template <typename DeviceTag>
class EdgePlotter : public viskores::worklet::WorkletMapField
{
public:
  using AtomicPackedFrameBufferHandle = viskores::exec::AtomicArrayExecutionObject<viskores::Int64>;
  using AtomicPackedFrameBuffer = viskores::cont::AtomicArray<viskores::Int64>;

  using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  VISKORES_CONT
  EdgePlotter(const viskores::Matrix<viskores::Float32, 4, 4>& worldToProjection,
              viskores::Id width,
              viskores::Id height,
              viskores::Id subsetWidth,
              viskores::Id subsetHeight,
              viskores::Id xOffset,
              viskores::Id yOffset,
              bool assocPoints,
              const viskores::Range& fieldRange,
              const ColorMapHandle& colorMap,
              const AtomicPackedFrameBuffer& frameBuffer,
              const viskores::Range& clippingRange,
              viskores::cont::Token& token)
    : WorldToProjection(worldToProjection)
    , Width(width)
    , Height(height)
    , SubsetWidth(subsetWidth)
    , SubsetHeight(subsetHeight)
    , XOffset(xOffset)
    , YOffset(yOffset)
    , AssocPoints(assocPoints)
    , ColorMap(colorMap.PrepareForInput(DeviceTag(), token))
    , ColorMapSize(viskores::Float32(colorMap.GetNumberOfValues() - 1))
    , FrameBuffer(frameBuffer.PrepareForExecution(DeviceTag(), token))
    , FieldMin(viskores::Float32(fieldRange.Min))
  {
    viskores::Float32 fieldLength = viskores::Float32(fieldRange.Length());
    if (fieldLength == 0.f)
    {
      // constant color
      this->InverseFieldDelta = 0.f;
    }
    else
    {
      this->InverseFieldDelta = 1.0f / fieldLength;
    }
    this->Offset = viskores::Max(0.03f / viskores::Float32(clippingRange.Length()), 0.0001f);
  }

  template <typename CoordinatesPortalType, typename ScalarFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id2& edgeIndices,
                                const CoordinatesPortalType& coordsPortal,
                                const ScalarFieldPortalType& fieldPortal) const
  {
    viskores::Id point1Idx = edgeIndices[0];
    viskores::Id point2Idx = edgeIndices[1];

    viskores::Vec3f_32 point1 = coordsPortal.Get(edgeIndices[0]);
    viskores::Vec3f_32 point2 = coordsPortal.Get(edgeIndices[1]);

    TransformWorldToViewport(point1);
    TransformWorldToViewport(point2);

    viskores::Float32 x1 = viskores::Round(point1[0]);
    viskores::Float32 y1 = viskores::Round(point1[1]);
    viskores::Float32 z1 = point1[2];
    viskores::Float32 x2 = viskores::Round(point2[0]);
    viskores::Float32 y2 = viskores::Round(point2[1]);
    viskores::Float32 z2 = point2[2];
    // If the line is steep, i.e., the height is greater than the width, then
    // transpose the co-ordinates to prevent "holes" in the line. This ensures
    // that we pick the co-ordinate which grows at a lesser rate than the other.
    bool transposed = viskores::Abs(y2 - y1) > viskores::Abs(x2 - x1);
    if (transposed)
    {
      viskores::Swap(x1, y1);
      viskores::Swap(x2, y2);
    }

    // Ensure we are always going from left to right
    if (x1 > x2)
    {
      viskores::Swap(x1, x2);
      viskores::Swap(y1, y2);
      viskores::Swap(z1, z2);
    }

    viskores::Float32 dx = x2 - x1;
    viskores::Float32 dy = y2 - y1;
    if (dx == 0.0)
      dx = viskores::Epsilon32(); // Avoid FPE
    viskores::Float32 gradient = dy / dx;

    viskores::Float32 xEnd = viskores::Round(x1);
    viskores::Float32 yEnd = y1 + gradient * (xEnd - x1);
    viskores::Float32 xPxl1 = xEnd, yPxl1 = IntegerPart(yEnd);
    viskores::Float32 zPxl1 = viskores::Lerp(z1, z2, (xPxl1 - x1) / dx);
    viskores::Float64 point1Field = fieldPortal.Get(point1Idx);
    viskores::Float64 point2Field;
    if (AssocPoints)
    {
      point2Field = fieldPortal.Get(point2Idx);
    }
    else
    {
      // cell associated field has a solid line color
      point2Field = point1Field;
    }

    // Plot first endpoint
    viskores::Vec4f_32 color = GetColor(point1Field);
    if (transposed)
    {
      Plot(yPxl1, xPxl1, zPxl1, color, 1.0f);
    }
    else
    {
      Plot(xPxl1, yPxl1, zPxl1, color, 1.0f);
    }

    viskores::Float32 interY = yEnd + gradient;
    xEnd = viskores::Round(x2);
    yEnd = y2 + gradient * (xEnd - x2);
    viskores::Float32 xPxl2 = xEnd, yPxl2 = IntegerPart(yEnd);
    viskores::Float32 zPxl2 = viskores::Lerp(z1, z2, (xPxl2 - x1) / dx);

    // Plot second endpoint
    color = GetColor(point2Field);
    if (transposed)
    {
      Plot(yPxl2, xPxl2, zPxl2, color, 1.0f);
    }
    else
    {
      Plot(xPxl2, yPxl2, zPxl2, color, 1.0f);
    }

    // Plot rest of the line
    if (transposed)
    {
      for (viskores::Float32 x = xPxl1 + 1; x <= xPxl2 - 1; ++x)
      {
        viskores::Float32 t = IntegerPart(interY);
        viskores::Float32 factor = (x - x1) / dx;
        viskores::Float32 depth = viskores::Lerp(zPxl1, zPxl2, factor);
        viskores::Float64 fieldValue = viskores::Lerp(point1Field, point2Field, factor);
        color = GetColor(fieldValue);
        Plot(t, x, depth, color, ReverseFractionalPart(interY));
        Plot(t + 1, x, depth, color, FractionalPart(interY));
        interY += gradient;
      }
    }
    else
    {
      for (viskores::Float32 x = xPxl1 + 1; x <= xPxl2 - 1; ++x)
      {
        viskores::Float32 t = IntegerPart(interY);
        viskores::Float32 factor = (x - x1) / dx;
        viskores::Float32 depth = viskores::Lerp(zPxl1, zPxl2, factor);
        viskores::Float64 fieldValue = viskores::Lerp(point1Field, point2Field, factor);
        color = GetColor(fieldValue);
        Plot(x, t, depth, color, ReverseFractionalPart(interY));
        Plot(x, t + 1, depth, color, FractionalPart(interY));
        interY += gradient;
      }
    }
  }

private:
  using ColorMapPortalConst = typename ColorMapHandle::ReadPortalType;

  VISKORES_EXEC
  void TransformWorldToViewport(viskores::Vec3f_32& point) const
  {
    viskores::Vec4f_32 temp(point[0], point[1], point[2], 1.0f);
    temp = viskores::MatrixMultiply(WorldToProjection, temp);
    for (viskores::IdComponent i = 0; i < 3; ++i)
    {
      point[i] = temp[i] / temp[3];
    }
    // Scale to canvas width and height
    point[0] =
      (point[0] * 0.5f + 0.5f) * viskores::Float32(SubsetWidth) + viskores::Float32(XOffset);
    point[1] =
      (point[1] * 0.5f + 0.5f) * viskores::Float32(SubsetHeight) + viskores::Float32(YOffset);
    // Convert from -1/+1 to 0/+1 range
    point[2] = point[2] * 0.5f + 0.5f;
    // Offset the point to a bit towards the camera. This is to ensure that the front faces of
    // the wireframe wins the z-depth check against the surface render, and is in addition to the
    // existing camera space offset.
    point[2] -= Offset;
  }

  VISKORES_EXEC viskores::Vec4f_32 GetColor(viskores::Float64 fieldValue) const
  {
    viskores::Int32 colorIdx = viskores::Int32((viskores::Float32(fieldValue) - FieldMin) *
                                               this->ColorMapSize * this->InverseFieldDelta);
    colorIdx = viskores::Min(viskores::Int32(this->ColorMap.GetNumberOfValues() - 1),
                             viskores::Max(0, colorIdx));
    return this->ColorMap.Get(colorIdx);
  }

  VISKORES_EXEC
  void Plot(viskores::Float32 x,
            viskores::Float32 y,
            viskores::Float32 depth,
            const viskores::Vec4f_32& color,
            viskores::Float32 intensity) const
  {
    viskores::Id xi = static_cast<viskores::Id>(x), yi = static_cast<viskores::Id>(y);
    if (xi < 0 || xi >= Width || yi < 0 || yi >= Height)
    {
      return;
    }
    viskores::Id index = yi * Width + xi;
    PackedValue current, next;
    current.Raw = ClearValue;
    next.Floats.Depth = depth;
    viskores::Vec4f_32 blendedColor;
    viskores::Vec4f_32 srcColor;
    do
    {
      UnpackColor(current.Ints.Color, srcColor);
      viskores::Float32 inverseIntensity = (1.0f - intensity);
      viskores::Float32 alpha = srcColor[3] * inverseIntensity;
      blendedColor[0] = color[0] * intensity + srcColor[0] * alpha;
      blendedColor[1] = color[1] * intensity + srcColor[1] * alpha;
      blendedColor[2] = color[2] * intensity + srcColor[2] * alpha;
      blendedColor[3] = alpha + intensity;
      next.Ints.Color = PackColor(blendedColor);
      FrameBuffer.CompareExchange(index, &current.Raw, next.Raw);
    } while (current.Floats.Depth > next.Floats.Depth);
  }

  viskores::Matrix<viskores::Float32, 4, 4> WorldToProjection;
  viskores::Id Width;
  viskores::Id Height;
  viskores::Id SubsetWidth;
  viskores::Id SubsetHeight;
  viskores::Id XOffset;
  viskores::Id YOffset;
  bool AssocPoints;
  ColorMapPortalConst ColorMap;
  viskores::Float32 ColorMapSize;
  AtomicPackedFrameBufferHandle FrameBuffer;
  viskores::Float32 FieldMin;
  viskores::Float32 InverseFieldDelta;
  viskores::Float32 Offset;
};

struct BufferConverter : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  BufferConverter() {}

  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, WorkIndex);

  template <typename DepthBufferPortalType, typename ColorBufferPortalType>
  VISKORES_EXEC void operator()(const viskores::Int64& packedValue,
                                DepthBufferPortalType& depthBuffer,
                                ColorBufferPortalType& colorBuffer,
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
};

} // namespace

class Wireframer
{
public:
  VISKORES_CONT
  Wireframer(viskores::rendering::Canvas* canvas, bool showInternalZones, bool isOverlay)
    : Canvas(canvas)
    , ShowInternalZones(showInternalZones)
    , IsOverlay(isOverlay)
  {
  }

  VISKORES_CONT
  void SetCamera(const viskores::rendering::Camera& camera) { this->Camera = camera; }

  VISKORES_CONT
  void SetColorMap(const ColorMapHandle& colorMap) { this->ColorMap = colorMap; }

  VISKORES_CONT
  void SetSolidDepthBuffer(const viskores::cont::ArrayHandle<viskores::Float32> depthBuffer)
  {
    this->SolidDepthBuffer = depthBuffer;
  }

  VISKORES_CONT
  void SetData(const viskores::cont::CoordinateSystem& coords,
               const IndicesHandle& endPointIndices,
               const viskores::cont::Field& field,
               const viskores::Range& fieldRange)
  {
    this->Bounds = coords.GetBounds();
    this->Coordinates = coords;
    this->PointIndices = endPointIndices;
    this->ScalarField = field;
    this->ScalarFieldRange = fieldRange;
  }

  VISKORES_CONT
  void Render()
  {
    RenderWithDeviceFunctor functor(this);
    viskores::cont::TryExecute(functor);
  }

private:
  template <typename DeviceTag>
  VISKORES_CONT void RenderWithDevice(DeviceTag)
  {

    // The wireframe should appear on top of any prerendered data, and hide away the internal
    // zones if `ShowInternalZones` is set to false. Since the prerendered data (or the solid
    // depth buffer) could cause z-fighting with the wireframe, we will offset all the edges in Z
    // by a small amount, proportional to distance between the near and far camera planes, in the
    // camera space.
    viskores::Range clippingRange = Camera.GetClippingRange();
    viskores::Float64 offset1 = (clippingRange.Max - clippingRange.Min) / 1.0e4;
    viskores::Float64 offset2 = clippingRange.Min / 2.0;
    viskores::Float32 offset = static_cast<viskores::Float32>(viskores::Min(offset1, offset2));
    viskores::Matrix<viskores::Float32, 4, 4> modelMatrix;
    viskores::MatrixIdentity(modelMatrix);
    modelMatrix[2][3] = offset;
    viskores::Matrix<viskores::Float32, 4, 4> worldToCamera =
      viskores::MatrixMultiply(modelMatrix, Camera.CreateViewMatrix());
    viskores::Matrix<viskores::Float32, 4, 4> WorldToProjection = viskores::MatrixMultiply(
      Camera.CreateProjectionMatrix(Canvas->GetWidth(), Canvas->GetHeight()), worldToCamera);

    viskores::Id width = static_cast<viskores::Id>(Canvas->GetWidth());
    viskores::Id height = static_cast<viskores::Id>(Canvas->GetHeight());
    viskores::Id pixelCount = width * height;

    if (this->ShowInternalZones && !this->IsOverlay)
    {
      viskores::cont::ArrayHandleConstant<viskores::Int64> clear(ClearValue, pixelCount);
      viskores::cont::Algorithm::Copy(clear, this->FrameBuffer);
    }
    else
    {
      VISKORES_ASSERT(this->SolidDepthBuffer.GetNumberOfValues() == pixelCount);
      CopyIntoFrameBuffer bufferCopy;
      viskores::worklet::DispatcherMapField<CopyIntoFrameBuffer>(bufferCopy)
        .Invoke(this->Canvas->GetColorBuffer(), this->SolidDepthBuffer, this->FrameBuffer);
    }
    //
    // detect a 2D camera and set the correct viewport.
    // The View port specifies what the region of the screen
    // to draw to which baiscally modifies the width and the
    // height of the "canvas"
    //
    viskores::Id xOffset = 0;
    viskores::Id yOffset = 0;
    viskores::Id subsetWidth = width;
    viskores::Id subsetHeight = height;

    bool ortho2d = Camera.GetMode() == viskores::rendering::Camera::Mode::TwoD;
    if (ortho2d)
    {
      viskores::Float32 vl, vr, vb, vt;
      Camera.GetRealViewport(width, height, vl, vr, vb, vt);
      viskores::Float32 _x = static_cast<viskores::Float32>(width) * (1.f + vl) / 2.f;
      viskores::Float32 _y = static_cast<viskores::Float32>(height) * (1.f + vb) / 2.f;
      viskores::Float32 _w = static_cast<viskores::Float32>(width) * (vr - vl) / 2.f;
      viskores::Float32 _h = static_cast<viskores::Float32>(height) * (vt - vb) / 2.f;

      subsetWidth = static_cast<viskores::Id>(_w);
      subsetHeight = static_cast<viskores::Id>(_h);
      yOffset = static_cast<viskores::Id>(_y);
      xOffset = static_cast<viskores::Id>(_x);
    }

    const bool isSupportedField = ScalarField.IsCellField() || ScalarField.IsPointField();
    if (!isSupportedField)
    {
      throw viskores::cont::ErrorBadValue("Field not associated with cell set or points");
    }
    const bool isAssocPoints = ScalarField.IsPointField();

    {
      viskores::cont::Token token;
      EdgePlotter<DeviceTag> plotter(WorldToProjection,
                                     width,
                                     height,
                                     subsetWidth,
                                     subsetHeight,
                                     xOffset,
                                     yOffset,
                                     isAssocPoints,
                                     ScalarFieldRange,
                                     ColorMap,
                                     FrameBuffer,
                                     Camera.GetClippingRange(),
                                     token);
      viskores::worklet::DispatcherMapField<EdgePlotter<DeviceTag>> plotterDispatcher(plotter);
      plotterDispatcher.SetDevice(DeviceTag());
      plotterDispatcher.Invoke(PointIndices,
                               Coordinates,
                               viskores::rendering::raytracing::GetScalarFieldArray(ScalarField));
    }

    BufferConverter converter;
    viskores::worklet::DispatcherMapField<BufferConverter> converterDispatcher(converter);
    converterDispatcher.SetDevice(DeviceTag());
    converterDispatcher.Invoke(FrameBuffer, Canvas->GetDepthBuffer(), Canvas->GetColorBuffer());
  }

  VISKORES_CONT
  struct RenderWithDeviceFunctor
  {
    Wireframer* Renderer;

    RenderWithDeviceFunctor(Wireframer* renderer)
      : Renderer(renderer)
    {
    }

    template <typename DeviceTag>
    VISKORES_CONT bool operator()(DeviceTag)
    {
      VISKORES_IS_DEVICE_ADAPTER_TAG(DeviceTag);
      Renderer->RenderWithDevice(DeviceTag());
      return true;
    }
  };

  viskores::Bounds Bounds;
  viskores::rendering::Camera Camera;
  viskores::rendering::Canvas* Canvas;
  bool ShowInternalZones;
  bool IsOverlay;
  ColorMapHandle ColorMap;
  viskores::cont::CoordinateSystem Coordinates;
  IndicesHandle PointIndices;
  viskores::cont::Field ScalarField;
  viskores::Range ScalarFieldRange;
  viskores::cont::ArrayHandle<viskores::Float32> SolidDepthBuffer;
  PackedFrameBufferHandle FrameBuffer;
}; // class Wireframer
}
} //namespace viskores::rendering

#endif //viskores_rendering_Wireframer_h
