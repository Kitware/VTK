// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUImageMapper.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatch.txx"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTexturedActor2D.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewport.h"
#include "vtkWebGPUPolyDataMapper2D.h"
#include "vtkWebGPURenderTextureDeviceResource.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"
#include "vtkWebGPUTexture.h"

#include <cmath>
#include <limits>
#include <type_traits>

VTK_ABI_NAMESPACE_BEGIN

// the bit-shift must be done after the comparison to zero
// because bit-shift is undefined behaviour for negative numbers
#define vtkClampIntToUnsignedChar(x, y, shift)                                                     \
  do                                                                                               \
  {                                                                                                \
    val = (y);                                                                                     \
    if (val < 0)                                                                                   \
    {                                                                                              \
      val = 0;                                                                                     \
    }                                                                                              \
    val >>= (shift);                                                                               \
    if (val > 255)                                                                                 \
    {                                                                                              \
      val = 255;                                                                                   \
    }                                                                                              \
    (x) = static_cast<unsigned char>(val);                                                         \
  } while (false)

namespace
{
const float RENDER_QUAD_UVS[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

inline unsigned char ClampToUnsignedChar(double val)
{
  val = std::clamp(val, 0.0, 255.0);
  return static_cast<unsigned char>(std::lround(val));
}

struct ColorTextureFallbackProcessor
{
  template <typename ArrayT>
  void operator()(ArrayT* colors, vtkWebGPUImageMapper* mapper, vtkImageData* image, double shift,
    double scale, vtkImageData* output)
  {
    const int& inMin0 = mapper->DisplayExtent[0];
    const int& inMax0 = mapper->DisplayExtent[1];
    const int& inMin1 = mapper->DisplayExtent[2];
    const int& inMax1 = mapper->DisplayExtent[3];

    const int width = inMax0 - inMin0 + 1;
    const int height = inMax1 - inMin1 + 1;

    vtkIdType tempIncs[3];
    image->GetIncrements(tempIncs);
    const auto& inInc1 = tempIncs[1];

    const int bpp = image->GetPointData()->GetScalars()->GetNumberOfComponents();
    if (bpp < 1)
    {
      vtkErrorWithObjectMacro(
        mapper, << "Number of components is less than 1, cannot create texture.");
      return;
    }
    // reformat data into unsigned char
    vtkNew<vtkUnsignedCharArray> uCharPixels;
    uCharPixels->SetNumberOfComponents(4);
    uCharPixels->Allocate(width * height * 4);
    auto offset = bpp *
      image->GetScalarIndex(
        mapper->DisplayExtent[0], mapper->DisplayExtent[2], mapper->DisplayExtent[4]);
    int i;
    int j = height;
    vtkIdType srcIndex = offset;
    vtkIdType nextSrcIndex = srcIndex;
    const auto colorsRange = vtk::DataArrayValueRange<1>(colors);
    unsigned char valueAsUChar = 0;
    while (--j >= 0)
    {
      i = width;
      srcIndex = nextSrcIndex;
      switch (bpp)
      {
        case 1:
          while (--i >= 0)
          {
            const auto value = colorsRange[srcIndex++];
            valueAsUChar = ClampToUnsignedChar((value + shift) * scale);
            uCharPixels->InsertNextValue(valueAsUChar);
            uCharPixels->InsertNextValue(valueAsUChar);
            uCharPixels->InsertNextValue(valueAsUChar);
            uCharPixels->InsertNextValue(255);
          }
          break;

        case 2:
          while (--i >= 0)
          {
            valueAsUChar = ClampToUnsignedChar((colorsRange[srcIndex++] + shift) * scale);
            uCharPixels->InsertNextValue(valueAsUChar);
            const auto redValueAsUChar = valueAsUChar; // repeated for blue channel
            valueAsUChar = ClampToUnsignedChar((colorsRange[srcIndex++] + shift) * scale);
            uCharPixels->InsertNextValue(valueAsUChar);
            uCharPixels->InsertNextValue(redValueAsUChar);
            uCharPixels->InsertNextValue(255);
          }
          break;

        case 3:
          while (--i >= 0)
          {
            valueAsUChar = ClampToUnsignedChar((colorsRange[srcIndex++] + shift) * scale);
            uCharPixels->InsertNextValue(valueAsUChar);
            valueAsUChar = ClampToUnsignedChar((colorsRange[srcIndex++] + shift) * scale);
            uCharPixels->InsertNextValue(valueAsUChar);
            valueAsUChar = ClampToUnsignedChar((colorsRange[srcIndex++] + shift) * scale);
            uCharPixels->InsertNextValue(valueAsUChar);
            uCharPixels->InsertNextValue(255);
          }
          break;

        default:
          while (--i >= 0)
          {
            valueAsUChar = ClampToUnsignedChar((colorsRange[srcIndex++] + shift) * scale);
            uCharPixels->InsertNextValue(valueAsUChar);
            valueAsUChar = ClampToUnsignedChar((colorsRange[srcIndex++] + shift) * scale);
            uCharPixels->InsertNextValue(valueAsUChar);
            valueAsUChar = ClampToUnsignedChar((colorsRange[srcIndex++] + shift) * scale);
            uCharPixels->InsertNextValue(valueAsUChar);
            valueAsUChar = ClampToUnsignedChar((colorsRange[srcIndex++] + shift) * scale);
            uCharPixels->InsertNextValue(valueAsUChar);
            srcIndex += bpp - 4;
          }
          break;
      }
      nextSrcIndex += inInc1;
    }
    output->SetExtent(0, width - 1, 0, height - 1, 0, 0);
    output->GetPointData()->SetScalars(uCharPixels);
  }
};

// only for ArrayT::ValueType == char, unsigned char, signed char, short, unsigned short
struct ColorTextureShiftScaleProcessor
{
  template <typename ArrayT>
  void operator()(ArrayT* colors, vtkWebGPUImageMapper* mapper, vtkImageData* image, double shift,
    double scale, vtkImageData* output)
  {
    using ValueT = vtk::GetAPIType<ArrayT>;
    static_assert(std::is_same_v<ValueT, char> || std::is_same_v<ValueT, unsigned char> ||
      std::is_same_v<ValueT, signed char> || std::is_same_v<ValueT, short> ||
      std::is_same_v<ValueT, unsigned short>);
    const int& inMin0 = mapper->DisplayExtent[0];
    const int& inMax0 = mapper->DisplayExtent[1];
    const int& inMin1 = mapper->DisplayExtent[2];
    const int& inMax1 = mapper->DisplayExtent[3];

    const int width = inMax0 - inMin0 + 1;
    const int height = inMax1 - inMin1 + 1;

    // Find the number of bits to use for the fraction:
    // continue increasing the bits until there is an overflow
    // in the worst case, then decrease by 1.
    // The "*2.0" and "*1.0" ensure that the comparison is done
    // with double-precision math.
    int bitShift = 0;
    double absScale = std::abs(scale);

    while ((static_cast<long>(1 << bitShift) * absScale) * 2.0 *
        std::numeric_limits<unsigned short>::max() <
      std::numeric_limits<int>::max() * 1.0)
    {
      bitShift++;
    }
    bitShift--;
    bitShift = std::max(bitShift, 0);

    long sscale = static_cast<long>(scale * (1 << bitShift));
    long sshift = static_cast<long>(sscale * shift);
    /* should do proper rounding, as follows:
    long sscale = (long) floor(scale*(1 << bitShift) + 0.5);
    long sshift = (long) floor((scale*shift + 0.5)*(1 << bitShift));
    */
    vtkIdType tempIncs[3];
    image->GetIncrements(tempIncs);
    const auto& inInc1 = tempIncs[1];

    const int bpp = image->GetPointData()->GetScalars()->GetNumberOfComponents();
    if (bpp < 1)
    {
      vtkErrorWithObjectMacro(
        mapper, << "Number of components is less than 1, cannot create texture.");
      return;
    }

    vtkNew<vtkUnsignedCharArray> uCharPixels;
    uCharPixels->SetNumberOfComponents(4);
    uCharPixels->Allocate(width * height * 4);
    auto offset = bpp *
      image->GetScalarIndex(
        mapper->DisplayExtent[0], mapper->DisplayExtent[2], mapper->DisplayExtent[4]);
    int i;
    int j = height;
    vtkIdType srcIndex = offset;
    vtkIdType nextSrcIndex = srcIndex;
    const auto colorsRange = vtk::DataArrayValueRange<1>(colors);
    long val;
    unsigned char valueAsUChar = 0;
    while (--j >= 0)
    {
      i = width;
      srcIndex = nextSrcIndex;
      switch (bpp)
      {
        case 1:
          while (--i >= 0)
          {
            vtkClampIntToUnsignedChar(
              valueAsUChar, (colorsRange[srcIndex++] * sscale + sshift), bitShift);
            uCharPixels->InsertNextValue(valueAsUChar);
            uCharPixels->InsertNextValue(valueAsUChar);
            uCharPixels->InsertNextValue(valueAsUChar);
            uCharPixels->InsertNextValue(255);
          }
          break;

        case 2:
          while (--i >= 0)
          {
            vtkClampIntToUnsignedChar(
              valueAsUChar, (colorsRange[srcIndex++] * sscale + sshift), bitShift);
            uCharPixels->InsertNextValue(valueAsUChar);
            const auto redValueAsUChar = valueAsUChar; // repeated for blue channel
            vtkClampIntToUnsignedChar(
              valueAsUChar, (colorsRange[srcIndex++] * sscale + sshift), bitShift);
            uCharPixels->InsertNextValue(valueAsUChar);
            uCharPixels->InsertNextValue(redValueAsUChar);
            uCharPixels->InsertNextValue(255);
          }
          break;

        case 3:
          while (--i >= 0)
          {
            vtkClampIntToUnsignedChar(
              valueAsUChar, (colorsRange[srcIndex++] * sscale + sshift), bitShift);
            uCharPixels->InsertNextValue(valueAsUChar);
            vtkClampIntToUnsignedChar(
              valueAsUChar, (colorsRange[srcIndex++] * sscale + sshift), bitShift);
            uCharPixels->InsertNextValue(valueAsUChar);
            vtkClampIntToUnsignedChar(
              valueAsUChar, (colorsRange[srcIndex++] * sscale + sshift), bitShift);
            uCharPixels->InsertNextValue(valueAsUChar);
            uCharPixels->InsertNextValue(255);
          }
          break;

        default:
          while (--i >= 0)
          {
            vtkClampIntToUnsignedChar(
              valueAsUChar, (colorsRange[srcIndex++] * sscale + sshift), bitShift);
            uCharPixels->InsertNextValue(valueAsUChar);
            vtkClampIntToUnsignedChar(
              valueAsUChar, (colorsRange[srcIndex++] * sscale + sshift), bitShift);
            uCharPixels->InsertNextValue(valueAsUChar);
            vtkClampIntToUnsignedChar(
              valueAsUChar, (colorsRange[srcIndex++] * sscale + sshift), bitShift);
            uCharPixels->InsertNextValue(valueAsUChar);
            vtkClampIntToUnsignedChar(
              valueAsUChar, (colorsRange[srcIndex++] * sscale + sshift), bitShift);
            uCharPixels->InsertNextValue(valueAsUChar);
            srcIndex += bpp - 4;
          }
          break;
      }
      nextSrcIndex += inInc1;
    }
    output->SetExtent(0, width - 1, 0, height - 1, 0, 0);
    output->GetPointData()->SetScalars(uCharPixels);
  }
};

// only for ArrayT::ValueType == char, unsigned char, signed char
struct ColorTextureSimpleProcessor
{
  template <typename ArrayT>
  void operator()(
    ArrayT* colors, vtkWebGPUImageMapper* mapper, vtkImageData* image, vtkImageData* output)
  {
    using ValueT = vtk::GetAPIType<ArrayT>;
    static_assert(std::is_same_v<ValueT, char> || std::is_same_v<ValueT, unsigned char> ||
      std::is_same_v<ValueT, signed char>);
    const int& inMin0 = mapper->DisplayExtent[0];
    const int& inMax0 = mapper->DisplayExtent[1];
    const int& inMin1 = mapper->DisplayExtent[2];
    const int& inMax1 = mapper->DisplayExtent[3];

    const int width = inMax0 - inMin0 + 1;
    const int height = inMax1 - inMin1 + 1;

    vtkIdType tempIncs[3];
    image->GetIncrements(tempIncs);
    const auto& inInc1 = tempIncs[1];

    const int bpp = image->GetPointData()->GetScalars()->GetNumberOfComponents();
    if (bpp < 1)
    {
      vtkErrorWithObjectMacro(
        mapper, << "Number of components is less than 1, cannot create texture.");
      return;
    }
    vtkNew<vtkUnsignedCharArray> uCharPixels;
    uCharPixels->SetNumberOfComponents(4);
    uCharPixels->Allocate(width * height * 4);
    auto offset = bpp *
      image->GetScalarIndex(
        mapper->DisplayExtent[0], mapper->DisplayExtent[2], mapper->DisplayExtent[4]);
    int i;
    int j = height;
    vtkIdType srcIndex = offset;
    vtkIdType nextSrcIndex = srcIndex;
    const auto colorsRange = vtk::DataArrayValueRange<1>(colors);
    while (--j >= 0)
    {
      i = width;
      srcIndex = nextSrcIndex;
      switch (bpp)
      {
        case 1:
          while (--i >= 0)
          {
            const auto value = colorsRange[srcIndex++];
            uCharPixels->InsertNextValue(value);
            uCharPixels->InsertNextValue(value);
            uCharPixels->InsertNextValue(value);
            uCharPixels->InsertNextValue(255);
          }
          break;

        case 2:
          while (--i >= 0)
          {
            const auto value = colorsRange[srcIndex++];
            uCharPixels->InsertNextValue(value);
            uCharPixels->InsertNextValue(value);
            uCharPixels->InsertNextValue(value);
            const auto alphaValue = colorsRange[srcIndex++];
            uCharPixels->InsertNextValue(alphaValue);
          }
          break;

        case 3:
          while (--i >= 0)
          {
            uCharPixels->InsertNextValue(colorsRange[srcIndex++]);
            uCharPixels->InsertNextValue(colorsRange[srcIndex++]);
            uCharPixels->InsertNextValue(colorsRange[srcIndex++]);
            uCharPixels->InsertNextValue(255);
          }
          break;

        default:
          while (--i >= 0)
          {
            uCharPixels->InsertNextValue(colorsRange[srcIndex++]);
            uCharPixels->InsertNextValue(colorsRange[srcIndex++]);
            uCharPixels->InsertNextValue(colorsRange[srcIndex++]);
            uCharPixels->InsertNextValue(colorsRange[srcIndex++]);
            srcIndex += bpp - 4;
          }
          break;
      }
      nextSrcIndex += inInc1;
    }
    output->SetExtent(0, width - 1, 0, height - 1, 0, 0);
    output->GetPointData()->SetScalars(uCharPixels);
  }
};
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUImageMapper);

//------------------------------------------------------------------------------
vtkWebGPUImageMapper::vtkWebGPUImageMapper()
{
  vtkNew<vtkWebGPUPolyDataMapper2D> mapper;
  vtkNew<vtkPolyData> renderQuad;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(4);
  renderQuad->SetPoints(points);

  vtkNew<vtkCellArray> tris;
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(1);
  tris->InsertCellPoint(2);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(3);
  renderQuad->SetPolys(tris);

  vtkNew<vtkTrivialProducer> prod;
  prod->SetOutput(renderQuad);

  // Set some properties.
  mapper->SetInputConnection(prod->GetOutputPort());
  this->ProxyActor->SetMapper(mapper);

  vtkNew<vtkWebGPUTexture> texture;
  texture->RepeatOff();
  this->ProxyActor->SetTexture(texture);

  vtkNew<vtkFloatArray> uvs;
  uvs->SetNumberOfComponents(2);
  uvs->SetArray(const_cast<float*>(RENDER_QUAD_UVS), 8, /*save=*/1); // do not delete
  renderQuad->GetPointData()->SetTCoords(uvs);
}

//------------------------------------------------------------------------------
vtkWebGPUImageMapper::~vtkWebGPUImageMapper() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPUImageMapper::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkWebGPUImageMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebGPUImageMapper::RenderOverlay(vtkViewport* viewport, vtkActor2D* actor)
{
  this->RenderStart(viewport, actor);
}

//------------------------------------------------------------------------------
void vtkWebGPUImageMapper::RenderData(vtkViewport* viewport, vtkImageData* image, vtkActor2D* actor)
{
  // Get the position of the image actor
  int* actorPos = actor->GetActualPositionCoordinate()->GetComputedViewportValue(viewport);
  // negative positions will already be clipped to viewport
  actorPos[0] += this->PositionAdjustment[0];
  actorPos[1] += this->PositionAdjustment[1];

  this->ProxyActor->SetPosition(actorPos[0], actorPos[1]);
  this->ProxyActor->SetPosition2(actor->GetPosition2());
  this->ProxyActor->SetProperty(actor->GetProperty());
  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(viewport);
  if (!wgpuRenderer)
  {
    vtkErrorMacro(<< "viewport is not a vtkWebGPURenderer");
    return;
  }
  switch (wgpuRenderer->GetRenderStage())
  {
    case vtkWebGPURenderer::RenderStageEnum::SyncDeviceResources:
      this->CreateTextureFromImage(image, wgpuRenderer);
      break;
    default:
      break;
  }

  int dimensions[3];
  this->ProxyActor->GetTexture()->GetInput()->GetDimensions(dimensions);
  int* proxyActorPos =
    this->ProxyActor->GetActualPositionCoordinate()->GetComputedViewportValue(wgpuRenderer);
  int* proxyActorPos2 =
    this->ProxyActor->GetActualPosition2Coordinate()->GetComputedViewportValue(wgpuRenderer);

  float xscale = 1.0;
  float yscale = 1.0;
  if (this->GetRenderToRectangle())
  {
    int rectwidth = (proxyActorPos2[0] - proxyActorPos[0]) + 1;
    int rectheight = (proxyActorPos2[1] - proxyActorPos[1]) + 1;
    xscale = static_cast<float>(rectwidth) / dimensions[0];
    yscale = static_cast<float>(rectheight) / dimensions[1];
  }

  vtkPolyData* pd = vtkPolyDataMapper2D::SafeDownCast(this->ProxyActor->GetMapper())->GetInput();
  vtkPoints* points = pd->GetPoints();
  points->SetPoint(0, 0.0, 0.0, 0);
  points->SetPoint(1, dimensions[0] * xscale, 0.0, 0);
  points->SetPoint(2, dimensions[0] * xscale, dimensions[1] * yscale, 0);
  points->SetPoint(3, 0.0, dimensions[1] * yscale, 0);
  points->GetData()->Modified();

  this->ProxyActor->RenderOverlay(wgpuRenderer);
}

//------------------------------------------------------------------------------
void vtkWebGPUImageMapper::ReleaseGraphicsResources(vtkWindow* window)
{
  this->ProxyActor->ReleaseGraphicsResources(window);
}

//------------------------------------------------------------------------------
void vtkWebGPUImageMapper::CreateTextureFromImage(
  vtkImageData* input, vtkWebGPURenderer* wgpuRenderer)
{
  if (!wgpuRenderer)
  {
    vtkErrorMacro(<< "wgpuRenderer is null");
    return;
  }
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(wgpuRenderer->GetVTKWindow());
  if (!wgpuRenderWindow)
  {
    vtkErrorMacro(<< "no WebGPU render window set for viewport");
    return;
  }
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
  if (!wgpuConfiguration)
  {
    vtkErrorMacro(<< "WGPU Configuration is null!");
    return;
  }
  if (!input->GetPointData() || !input->GetPointData()->GetScalars())
  {
    return;
  }
  auto scalars = input->GetPointData()->GetScalars();
  vtkNew<vtkImageData> output;
  const auto shift = this->GetColorShift();
  const auto scale = this->GetColorScale();
  if (shift == 0 && scale == 1)
  {
    using DispatchT =
      vtkArrayDispatch::DispatchByValueType<vtkTypeList::Create<char, signed char, unsigned char>>;
    ColorTextureSimpleProcessor colorTextureSimpleProcessor;
    if (!DispatchT::Execute(scalars, colorTextureSimpleProcessor, this, input, output))
    {
      using ShortDispatchT =
        vtkArrayDispatch::DispatchByValueType<vtkTypeList::Create<short, unsigned short>>;
      ColorTextureShiftScaleProcessor colorTextureShiftScaleProcessor;
      if (!ShortDispatchT::Execute(
            scalars, colorTextureShiftScaleProcessor, this, input, shift, scale, output))
      {
        using FallbackDispatchT = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
        ColorTextureFallbackProcessor fallbackColorProcessor;
        if (!FallbackDispatchT::Execute(
              scalars, fallbackColorProcessor, this, input, shift, scale, output))
        {
          fallbackColorProcessor(
            scalars, this, input, shift, scale, output); // try without dispatch
        }
      }
    }
  }
  else
  {
    using ShortDispatchT = vtkArrayDispatch::DispatchByValueType<
      vtkTypeList::Create<char, signed char, unsigned char, short, unsigned short>>;
    ColorTextureShiftScaleProcessor colorTextureShiftScaleProcessor;
    if (!ShortDispatchT::Execute(
          scalars, colorTextureShiftScaleProcessor, this, input, shift, scale, output))
    {
      using FallbackDispatchT = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
      ColorTextureFallbackProcessor fallbackColorProcessor;
      if (!FallbackDispatchT::Execute(
            scalars, fallbackColorProcessor, this, input, shift, scale, output))
      {
        fallbackColorProcessor(scalars, this, input, shift, scale, output); // try without dispatch
      }
    }
  }
  this->ProxyActor->GetTexture()->SetInputData(output);
}

VTK_ABI_NAMESPACE_END
