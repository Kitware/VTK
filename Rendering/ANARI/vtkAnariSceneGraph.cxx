// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include <anari/anari_cpp/ext/std.h>
#define ANARI_EXTENSION_UTILITY_IMPL
#include <anari/frontend/anari_extension_utility.h>

#include "vtkAnariSceneGraph.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkAnariActorNode.h"
#include "vtkAnariCameraNode.h"
#include "vtkAnariDevice.h"
#include "vtkAnariLightNode.h"
#include "vtkAnariProfiling.h"
#include "vtkAnariVolumeNode.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationStringKey.h"
#include "vtkLight.h"
#include "vtkLogger.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"

#include <cmath>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

using namespace anari::std_types;

vtkInformationKeyMacro(vtkAnariSceneGraph, COMPOSITE_ON_GL, Integer);
vtkInformationKeyMacro(vtkAnariSceneGraph, ACCUMULATION_COUNT, Integer);

struct RendererChangeCallback : vtkCommand
{
  vtkTypeMacro(RendererChangeCallback, vtkCommand);

  static RendererChangeCallback* New() { return new RendererChangeCallback; }

  void Execute(
    vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventId), void* vtkNotUsed(callData))
  {
    this->AnariRendererModifiedTime->Modified();
  }

  vtkTimeStamp* AnariRendererModifiedTime{ nullptr };
};

class vtkAnariSceneGraphInternals
{
public:
  vtkAnariSceneGraphInternals(vtkAnariSceneGraph*);
  ~vtkAnariSceneGraphInternals();

  vtkAnariSceneGraph* Owner{ nullptr };

  std::vector<uint8_t> ColorBuffer;
  std::vector<float> DepthBuffer;

  int ImageX;
  int ImageY;

  bool CompositeOnGL{ false };
  bool OnlyUpdateWorld{ false };

  vtkSmartPointer<vtkAnariDevice> AnariDevice{ nullptr };
  anari::Renderer AnariRenderer{ nullptr };
  anari::World AnariWorld{ nullptr };
  anari::Frame AnariFrame{ nullptr };

  anari::Extensions AnariExtensions{};
  const char* const* AnariExtensionStrings{ nullptr };

  std::vector<anari::Surface> AnariSurfaces;
  std::vector<anari::Volume> AnariVolumes;
  std::vector<anari::Light> AnariLights;

  int MaxPropId = 0;
};

vtkAnariSceneGraphInternals::vtkAnariSceneGraphInternals(vtkAnariSceneGraph* owner)
  : Owner(owner)
{
}

vtkAnariSceneGraphInternals::~vtkAnariSceneGraphInternals()
{
  if (this->AnariDevice)
  {
    anari::Device d = this->AnariDevice->GetHandle();
    if (d)
    {
      anari::release(d, this->AnariWorld);
      anari::release(d, this->AnariRenderer);
      anari::release(d, this->AnariFrame);
      anari::release(d, d);
    }
  }
}

//============================================================================

vtkStandardNewMacro(vtkAnariSceneGraph);

//----------------------------------------------------------------------------
vtkAnariSceneGraph::vtkAnariSceneGraph()
{
  this->Internal = new vtkAnariSceneGraphInternals(this);
  InvalidateSceneStructure();
}

//----------------------------------------------------------------------------
vtkAnariSceneGraph::~vtkAnariSceneGraph()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::InitAnariFrame(vtkRenderer* ren)
{
  if (this->Internal->AnariFrame != nullptr)
  {
    return;
  }

  auto anariDevice = this->GetDeviceHandle();
  this->Internal->AnariFrame = anari::newObject<anari::Frame>(anariDevice);
  anari::setParameter(anariDevice, this->Internal->AnariFrame, "channel.color", ANARI_UFIXED8_VEC4);
  anari::setParameter(anariDevice, this->Internal->AnariFrame, "channel.depth", ANARI_FLOAT32);
  anari::commitParameters(anariDevice, this->Internal->AnariFrame);

  if (!ren->HasObserver(vtkCommand::ModifiedEvent))
  {
    vtkNew<RendererChangeCallback> cc;
    cc->AnariRendererModifiedTime = &this->AnariRendererModifiedTime;
    ren->AddObserver(vtkCommand::ModifiedEvent, cc);
    cc->Execute(nullptr, vtkCommand::ModifiedEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::SetupAnariRendererParameters(vtkRenderer* ren)
{
  if (this->AnariRendererModifiedTime <= this->AnariRendererUpdatedTime)
  {
    return;
  }

  auto anariDevice = this->GetDeviceHandle();
  auto anariRenderer = this->Internal->AnariRenderer;

  double* bg = ren->GetBackground();
  double bgAlpha = ren->GetBackgroundAlpha();

  if (!ren->GetGradientBackground())
  {
    float bgColor[4] = { static_cast<float>(bg[0]), static_cast<float>(bg[1]),
      static_cast<float>(bg[2]), static_cast<float>(bgAlpha) };

    anari::setParameter(anariDevice, anariRenderer, "background", bgColor);
  }
  else
  {
    double* topbg = ren->GetBackground2();
    constexpr int IMAGE_SIZE = 128;

    vtkNew<vtkColorTransferFunction> colorTF;
    colorTF->AddRGBPoint(0, bg[0], bg[1], bg[2]);
    colorTF->AddRGBPoint(IMAGE_SIZE, topbg[0], topbg[1], topbg[2]);

    auto gradientArray = anari::newArray2D(anariDevice, ANARI_FLOAT32_VEC4, 1, IMAGE_SIZE + 1);
    auto* gradientColors = anari::map<vec4>(anariDevice, gradientArray);

    for (int i = 0; i <= IMAGE_SIZE; i++)
    {
      double* color = colorTF->GetColor(i);

      gradientColors[i] = vec4{ static_cast<float>(color[0]), static_cast<float>(color[1]),
        static_cast<float>(color[2]), static_cast<float>(bgAlpha) };
    }

    anari::unmap(anariDevice, gradientArray);
    anari::setAndReleaseParameter(anariDevice, anariRenderer, "background", gradientArray);
    anari::commitParameters(anariDevice, anariRenderer);
  }

  anari::commitParameters(anariDevice, anariRenderer);

  this->AnariRendererUpdatedTime = this->AnariRendererModifiedTime;
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::InitAnariWorld()
{
  if (this->Internal->AnariWorld != nullptr)
  {
    return;
  }

  auto anariDevice = this->GetDeviceHandle();

  auto anariWorld = anari::newObject<anari::World>(anariDevice);
  this->Internal->AnariWorld = anariWorld;
  anari::setParameter(anariDevice, anariWorld, "name", "vtk_world");
  anari::commitParameters(anariDevice, anariWorld);

  auto anariFrame = this->Internal->AnariFrame;
  anari::setParameter(anariDevice, anariFrame, "world", this->Internal->AnariWorld);
  anari::commitParameters(anariDevice, anariFrame);
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::UpdateAnariFrameSize()
{
  const uvec2 frameSize = { static_cast<uint32_t>(this->Size[0]),
    static_cast<uint32_t>(this->Size[1]) };
  if ((uint32_t)this->Internal->ImageX == frameSize[0] &&
    (uint32_t)this->Internal->ImageY == frameSize[1])
  {
    return;
  }

  this->Internal->ImageX = frameSize[0];
  this->Internal->ImageY = frameSize[1];

  const size_t totalSize = this->Size[0] * this->Size[1];
  this->Internal->ColorBuffer.resize(totalSize * sizeof(float));
  this->Internal->DepthBuffer.resize(totalSize);

  auto anariDevice = this->GetDeviceHandle();
  auto anariFrame = this->Internal->AnariFrame;
  anari::setParameter(anariDevice, anariFrame, "size", frameSize);
  anari::commitParameters(anariDevice, anariFrame);
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::UpdateAnariLights()
{
  auto anariDevice = this->GetDeviceHandle();
  auto anariWorld = this->Internal->AnariWorld;
  const auto& lightState = this->Internal->AnariLights;

  if (!lightState.empty())
  {
    for (size_t i = 0; i < lightState.size(); i++)
    {
      std::string lightName = "vtk_light_" + std::to_string(i);
      anari::setParameter(anariDevice, lightState[i], "name", lightName.c_str());
      anari::commitParameters(anariDevice, lightState[i]);
    }

    anari::setParameterArray1D(
      anariDevice, anariWorld, "light", lightState.data(), lightState.size());
  }
  else
  {
    vtkWarningMacro(<< "No lights set on world.");
    anari::unsetParameter(anariDevice, anariWorld, "light");
  }

  anari::commitParameters(anariDevice, anariWorld);
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::UpdateAnariSurfaces()
{
  auto anariDevice = this->GetDeviceHandle();
  auto anariWorld = this->Internal->AnariWorld;
  const auto& surfaceState = this->Internal->AnariSurfaces;

  if (!surfaceState.empty())
  {
    for (size_t i = 0; i < surfaceState.size(); i++)
    {
      anari::commitParameters(anariDevice, surfaceState[i]);
    }

    anari::setParameterArray1D(
      anariDevice, anariWorld, "surface", surfaceState.data(), surfaceState.size());
  }
  else
  {
    anari::unsetParameter(anariDevice, anariWorld, "surface");
  }

  anari::commitParameters(anariDevice, anariWorld);
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::UpdateAnariVolumes()
{
  auto anariDevice = this->GetDeviceHandle();
  auto anariWorld = this->Internal->AnariWorld;
  const auto& volumeState = this->Internal->AnariVolumes;

  if (!volumeState.empty())
  {
    for (size_t i = 0; i < volumeState.size(); i++)
    {
      std::string volumeName = "vtk_volume_" + std::to_string(i);
      anari::setParameter(anariDevice, volumeState[i], "name", volumeName.c_str());
      anari::commitParameters(anariDevice, volumeState[i]);
    }

    anari::setParameterArray1D(
      anariDevice, anariWorld, "volume", volumeState.data(), volumeState.size());
  }
  else
  {
    anari::unsetParameter(anariDevice, anariWorld, "volume");
  }

  anari::commitParameters(anariDevice, anariWorld);
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::DebugOutputWorldBounds()
{
  auto anariDevice = this->GetDeviceHandle();
  auto anariWorld = this->Internal->AnariWorld;

  // Get world bounds
  float worldBounds[6];
  if (anariGetProperty(anariDevice, anariWorld, "bounds", ANARI_FLOAT32_BOX3, worldBounds,
        sizeof(worldBounds), ANARI_WAIT))
  {
    vtkDebugMacro(<< "[ANARI] World Bounds: "
                  << "{" << worldBounds[0] << ", " << worldBounds[1] << ", " << worldBounds[2]
                  << "}, "
                  << "{" << worldBounds[3] << ", " << worldBounds[4] << ", " << worldBounds[5]
                  << "}");
  }
  else
  {
    vtkWarningMacro(<< "[ANARI] World bounds not returned");
  }
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::ResetReservedPropIds()
{
  this->Internal->MaxPropId = 0;
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::CopyAnariFrameBufferData()
{
  int totalSize = this->Size[0] * this->Size[1];

  auto anariDevice = this->GetDeviceHandle();
  auto anariFrame = this->Internal->AnariFrame;

  float duration = 0.0f;
  anari::getProperty(anariDevice, anariFrame, "duration", duration, ANARI_NO_WAIT);

  vtkDebugMacro(<< "Rendered frame in " << duration * 1000.0f << " ms");

  // Color buffer
  auto renderedFrame = anari::map<uint32_t>(anariDevice, anariFrame, "channel.color");

  if (renderedFrame.data != nullptr)
  {
    int retTotalSize = renderedFrame.width * renderedFrame.height;
    totalSize = std::min(retTotalSize, totalSize);
    memcpy(this->Internal->ColorBuffer.data(), renderedFrame.data, totalSize * 4);
  }
  else
  {
    vtkWarningMacro(<< "Color buffer is null");
    memset(this->Internal->ColorBuffer.data(), 255, totalSize * 4);
  }

  anari::unmap(anariDevice, anariFrame, "channel.color");

  // Depth Buffer
  auto mappedDepthBuffer = anari::map<float>(anariDevice, anariFrame, "channel.depth");

  if (mappedDepthBuffer.data != nullptr)
  {
    vtkCamera* cam = vtkRenderer::SafeDownCast(this->Renderable)->GetActiveCamera();
    double* clipValues = cam->GetClippingRange();
    double clipMin = clipValues[0];
    double clipMax = clipValues[1];
    double clipDiv = 1.0 / (clipMax - clipMin);

    const float* depthBuffer = mappedDepthBuffer.data;   // s
    float* zBuffer = this->Internal->DepthBuffer.data(); // d

    for (int i = 0; i < totalSize; i++)
    {
      // *d = (*s < clipMin ? 1.0 : (*s - clipMin) * clipDiv);
      zBuffer[i] = (depthBuffer[i] < clipMin ? 1.0f : (depthBuffer[i] - clipMin) * clipDiv);
    }
  }
  else
  {
    vtkWarningMacro(<< "Depth buffer is null");
    memset(this->Internal->DepthBuffer.data(), 0, totalSize * sizeof(float));
  }

  anari::unmap(anariDevice, anariFrame, "channel.depth");
}

//----------------------------------------------------------------------------
#define RENDERER_NODE_PARAM_SET_DEFINITION(FCN, PARAM, TYPE)                                       \
  void vtkAnariSceneGraph::Set##FCN(vtkRenderer* r, TYPE v)                                        \
  {                                                                                                \
    if (!r)                                                                                        \
    {                                                                                              \
      return;                                                                                      \
    }                                                                                              \
                                                                                                   \
    vtkInformation* info = r->GetInformation();                                                    \
    info->Set(vtkAnariSceneGraph::PARAM(), v);                                                     \
  }

RENDERER_NODE_PARAM_SET_DEFINITION(AccumulationCount, ACCUMULATION_COUNT, int)
RENDERER_NODE_PARAM_SET_DEFINITION(CompositeOnGL, COMPOSITE_ON_GL, int)

//----------------------------------------------------------------------------
#define RENDERER_NODE_PARAM_GET_DEFINITION(FCN, PARAM, TYPE, DEFAULT_VALUE)                        \
  TYPE vtkAnariSceneGraph::Get##FCN(vtkRenderer* r)                                                \
  {                                                                                                \
    if (!r)                                                                                        \
    {                                                                                              \
      return DEFAULT_VALUE;                                                                        \
    }                                                                                              \
                                                                                                   \
    vtkInformation* info = r->GetInformation();                                                    \
                                                                                                   \
    if (info && info->Has(vtkAnariSceneGraph::PARAM()))                                            \
    {                                                                                              \
      return info->Get(vtkAnariSceneGraph::PARAM());                                               \
    }                                                                                              \
                                                                                                   \
    return DEFAULT_VALUE;                                                                          \
  }

RENDERER_NODE_PARAM_GET_DEFINITION(AccumulationCount, ACCUMULATION_COUNT, int, 1)
RENDERER_NODE_PARAM_GET_DEFINITION(CompositeOnGL, COMPOSITE_ON_GL, int, 0)

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::SetCamera(anari::Camera camera)
{
  auto d = this->GetDeviceHandle();
  auto f = this->Internal->AnariFrame;
  if (d && f)
  {
    anari::setParameter(d, f, "camera", camera);
    anari::commitParameters(d, f);
  }
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::AddLight(anari::Light light)
{
  if (light != nullptr)
  {
    this->Internal->AnariLights.emplace_back(light);
  }
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::AddSurface(anari::Surface surface)
{
  if (surface != nullptr)
  {
    this->Internal->AnariSurfaces.push_back(surface);
  }
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::AddVolume(anari::Volume volume)
{
  if (volume != nullptr)
  {
    this->Internal->AnariVolumes.emplace_back(volume);
  }
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::Traverse(int operation)
{
  if (operation == operation_type::render)
  {
    this->Apply(operation, true);
    if (this->AnariSceneConstructedMTime < this->AnariSceneStructureModifiedMTime)
    {
      this->Internal->AnariLights.clear();
      this->Internal->AnariVolumes.clear();
      this->Internal->AnariSurfaces.clear();
      for (auto val : this->Children)
      {
        val->Traverse(operation);
      }
      this->UpdateAnariLights();
      this->UpdateAnariSurfaces();
      this->UpdateAnariVolumes();
      this->AnariSceneConstructedMTime = this->AnariSceneStructureModifiedMTime;
    }
    this->Apply(operation, false);
  }
  else
  {
    this->Superclass::Traverse(operation);
  }
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::Invalidate(bool prepass)
{
  if (prepass)
  {
    this->RenderTime = 0;
  }
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::Build(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariSceneGraph::Build", vtkAnariProfiling::BLUE);

  if (prepass)
  {
    vtkRenderer* aren = vtkRenderer::SafeDownCast(this->Renderable);

    ResetReservedPropIds(); // Make sure the prop ids are reset before rendering

    // make sure we have a camera
    if (!(aren->IsActiveCameraCreated()))
    {
      aren->ResetCamera();
    }
  }

  this->Superclass::Build(prepass);
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::Render(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariSceneGraph::Render", vtkAnariProfiling::BLUE);

  vtkRenderer* ren = this->GetRenderer();
  if (this->Internal->OnlyUpdateWorld || prepass || !ren)
  {
    return;
  }

  this->Internal->CompositeOnGL = (this->GetCompositeOnGL(ren) != 0);
  this->SetupAnariRendererParameters(ren);
  this->UpdateAnariFrameSize();
#if 0
    this->DebugOutputWorldBounds();
#endif

  // Render frame
  auto anariDevice = this->GetDeviceHandle();
  auto anariFrame = this->Internal->AnariFrame;
  int accumulationCount = this->GetAccumulationCount(ren);
  for (int i = 0; i < accumulationCount; i++)
  {
    anari::render(anariDevice, anariFrame);
  }

  anari::wait(anariDevice, anariFrame);

  CopyAnariFrameBufferData();
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::WriteLayer(
  unsigned char* buffer, float* Z, int buffx, int buffy, int layer)
{
  vtkAnariProfiling startProfiling("vtkAnariSceneGraph::WriteLayer", vtkAnariProfiling::BLUE);
  unsigned char* colorBuffer = this->Internal->ColorBuffer.data();
  float* zBuffer = this->Internal->DepthBuffer.data();

  if (layer == 0)
  {
    for (int j = 0; j < buffy && j < this->Size[1]; j++)
    {
      unsigned char* iptr = colorBuffer + j * this->Size[0] * 4;
      float* zptr = zBuffer + j * this->Size[0];

      unsigned char* optr = buffer + j * buffx * 4;
      float* ozptr = Z + j * buffx;

      for (int i = 0; i < buffx && i < this->Size[0]; i++)
      {
        // copy 4 color channels
        *optr++ = *iptr++;
        *optr++ = *iptr++;
        *optr++ = *iptr++;
        *optr++ = *iptr++;

        // copy depth values
        *ozptr++ = *zptr++;
      }
    }
  }
  else
  {
    for (int j = 0; j < buffy && j < this->Size[1]; j++)
    {
      unsigned char* iptr = colorBuffer + j * this->Size[0] * 4;
      float* zptr = zBuffer + j * this->Size[0];

      unsigned char* optr = buffer + j * buffx * 4;
      float* ozptr = Z + j * buffx;

      for (int i = 0; i < buffx && i < this->Size[0]; i++)
      {
        if (*zptr < 1.0)
        {
          if (this->Internal->CompositeOnGL)
          {
            unsigned char a = (*(iptr + 3));
            float A = static_cast<float>(a) / 255.0f;

            for (int h = 0; h < 3; h++)
            {
              *optr = static_cast<uint8_t>(((float)*iptr) * (A) + ((float)*optr) * (1 - A));
              optr++;
              iptr++;
            }

            *optr++ = *iptr++;
          }
          else
          {
            *optr++ = *iptr++;
            *optr++ = *iptr++;
            *optr++ = *iptr++;
            *optr++ = *iptr++;
          }

          *ozptr = *zptr;
        }
        else
        {
          optr += 4;
          iptr += 4;
        }

        ozptr++;
        zptr++;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkAnariSceneGraph::SetUpdateWorldOnly(bool onlyUpdateWorld)
{
  this->Internal->OnlyUpdateWorld = onlyUpdateWorld;
}

//------------------------------------------------------------------------------
vtkRenderer* vtkAnariSceneGraph::GetRenderer()
{
  return vtkRenderer::SafeDownCast(this->GetRenderable());
}

//------------------------------------------------------------------------------
anari::Device vtkAnariSceneGraph::GetDeviceHandle() const
{
  return this->Internal->AnariDevice->GetHandle();
}

//------------------------------------------------------------------------------
anari::Renderer vtkAnariSceneGraph::GetRendererHandle() const
{
  return this->Internal->AnariRenderer;
}

//------------------------------------------------------------------------------
const anari::Extensions& vtkAnariSceneGraph::GetAnariDeviceExtensions() const
{
  return this->Internal->AnariExtensions;
}

//------------------------------------------------------------------------------
const char* const* vtkAnariSceneGraph::GetAnariDeviceExtensionStrings() const
{
  return this->Internal->AnariExtensionStrings;
}

//------------------------------------------------------------------------------
const unsigned char* vtkAnariSceneGraph::GetBuffer()
{
  return this->Internal->ColorBuffer.data();
}

//------------------------------------------------------------------------------
const float* vtkAnariSceneGraph::GetZBuffer()
{
  return this->Internal->DepthBuffer.data();
}

//------------------------------------------------------------------------------
void vtkAnariSceneGraph::InvalidateSceneStructure()
{
  this->AnariSceneStructureModifiedMTime.Modified();
}

//------------------------------------------------------------------------------
int vtkAnariSceneGraph::ReservePropId()
{
  return this->Internal->MaxPropId++;
}

//------------------------------------------------------------------------------
void vtkAnariSceneGraph::SetAnariDevice(
  vtkAnariDevice* ad, anari::Extensions e, const char* const* es)
{
  vtkRenderer* renderer = GetRenderer();
  if (!renderer)
  {
    vtkErrorMacro(<< "Null vtkRenderer in vtkAnariSceneGraph::SetAnariDevice()");
    return;
  }

  if (this->Internal->AnariDevice != nullptr)
  {
    vtkErrorMacro(<< "vtkAnariSceneGraph::SetAnariDevice() called too many times");
  }

  if (!ad)
  {
    vtkErrorMacro(<< "Trying to set null anari device");
    return;
  }

  anari::Device d = ad->GetHandle();
  this->IssuedWarnings.clear();
  anari::retain(d, d);
  this->Internal->AnariDevice = ad;
  this->Internal->AnariExtensions = e;
  this->Internal->AnariExtensionStrings = es;
  this->InitAnariFrame(renderer);
  this->InitAnariWorld();

  this->AnariRendererModifiedTime.Modified();
}

//------------------------------------------------------------------------------
void vtkAnariSceneGraph::SetAnariRenderer(anari::Renderer r)
{
  if (!this->Internal->AnariDevice)
  {
    return;
  }

  auto d = this->GetDeviceHandle();
  anari::retain(d, r);
  anari::release(d, this->Internal->AnariRenderer);
  this->Internal->AnariRenderer = r;

  if (!this->Internal->AnariFrame)
  {
    return;
  }

  auto f = this->Internal->AnariFrame;
  if (r)
    anari::setParameter(d, f, "renderer", r);
  else
    anari::unsetParameter(d, f, "renderer");
  anari::commitParameters(d, f);

  this->AnariRendererModifiedTime.Modified();
}

//------------------------------------------------------------------------------
void vtkAnariSceneGraph::WarningMacroOnce(
  vtkSmartPointer<vtkObject> caller, const std::string& warning)
{
  if (!this->Internal->AnariDevice || !caller)
  {
    return;
  }
  std::string classname(caller->GetClassName());
  auto warnFound = this->IssuedWarnings.find(classname);
  bool issueWarn = true;
  if (warnFound != this->IssuedWarnings.end())
  {
    auto warnvec = warnFound->second;
    for (auto iter = warnvec.begin(); iter != warnvec.end(); ++iter)
    {
      if ((*iter) == warning)
      {
        issueWarn = false;
        break;
      }
    }
  }
  if (issueWarn)
  {
    vtkSmartPointer<vtkRenderer> ren = this->GetRenderer();
    std::string library = this->Internal->AnariDevice->GetAnariLibraryName();
    std::string device = this->Internal->AnariDevice->GetAnariDeviceName();
    vtkWarningMacro(<< "ANARI back-end " << this << " " << library << ":" << device << " "
                    << warning);
    this->IssuedWarnings[classname].push_back(warning);
  }
}

VTK_ABI_NAMESPACE_END
