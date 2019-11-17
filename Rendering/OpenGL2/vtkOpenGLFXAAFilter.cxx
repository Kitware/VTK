/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLFXAAFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLFXAAFilter.h"

#include "vtk_glew.h"

#include "vtkFXAAOptions.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderTimer.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTimerLog.h"
#include "vtkTypeTraits.h"

#include <algorithm>
#include <cassert>

// Our fragment shader:
#include "vtkFXAAFilterFS.h"

// Define to perform/dump benchmarking info:
//#define FXAA_BENCHMARK

vtkStandardNewMacro(vtkOpenGLFXAAFilter);

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RelativeContrastThreshold: " << this->RelativeContrastThreshold << "\n";
  os << indent << "HardContrastThreshold: " << this->HardContrastThreshold << "\n";
  os << indent << "SubpixelBlendLimit: " << this->SubpixelBlendLimit << "\n";
  os << indent << "SubpixelContrastThreshold: " << this->SubpixelContrastThreshold << "\n";
  os << indent << "EndpointSearchIterations: " << this->EndpointSearchIterations << "\n";
  os << indent << "UseHighQualityEndpoints: " << this->UseHighQualityEndpoints << "\n";

  os << indent << "DebugOptionValue: ";
  switch (this->DebugOptionValue)
  {
    default:
    case vtkFXAAOptions::FXAA_NO_DEBUG:
      os << "FXAA_NO_DEBUG\n";
      break;
    case vtkFXAAOptions::FXAA_DEBUG_SUBPIXEL_ALIASING:
      os << "FXAA_DEBUG_SUBPIXEL_ALIASING\n";
      break;
    case vtkFXAAOptions::FXAA_DEBUG_EDGE_DIRECTION:
      os << "FXAA_DEBUG_EDGE_DIRECTION\n";
      break;
    case vtkFXAAOptions::FXAA_DEBUG_EDGE_NUM_STEPS:
      os << "FXAA_DEBUG_EDGE_NUM_STEPS\n";
      break;
    case vtkFXAAOptions::FXAA_DEBUG_EDGE_DISTANCE:
      os << "FXAA_DEBUG_EDGE_DISTANCE\n";
      break;
    case vtkFXAAOptions::FXAA_DEBUG_EDGE_SAMPLE_OFFSET:
      os << "FXAA_DEBUG_EDGE_SAMPLE_OFFSET\n";
      break;
    case vtkFXAAOptions::FXAA_DEBUG_ONLY_SUBPIX_AA:
      os << "FXAA_DEBUG_ONLY_SUBPIX_AA\n";
      break;
    case vtkFXAAOptions::FXAA_DEBUG_ONLY_EDGE_AA:
      os << "FXAA_DEBUG_ONLY_EDGE_AA\n";
      break;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::Execute(vtkOpenGLRenderer* ren)
{
  assert(ren);
  this->Renderer = ren;

  this->StartTimeQuery(this->PreparationTimer);
  this->Prepare();
  this->LoadInput();
  this->EndTimeQuery(this->PreparationTimer);

  this->StartTimeQuery(this->FXAATimer);
  this->ApplyFilter();
  this->EndTimeQuery(this->FXAATimer);

  this->Finalize();
  this->PrintBenchmark();

  this->Renderer = nullptr;
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::ReleaseGraphicsResources()
{
  this->FreeGLObjects();
  this->PreparationTimer->ReleaseGraphicsResources();
  this->FXAATimer->ReleaseGraphicsResources();
  if (this->QHelper)
  {
    delete this->QHelper;
    this->QHelper = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::UpdateConfiguration(vtkFXAAOptions* opts)
{
  // Use the setters -- some of these options will trigger a shader rebuild
  // when they change, and the setters hold the logic for determining this.
  this->SetRelativeContrastThreshold(opts->GetRelativeContrastThreshold());
  this->SetHardContrastThreshold(opts->GetHardContrastThreshold());
  this->SetSubpixelBlendLimit(opts->GetSubpixelBlendLimit());
  this->SetSubpixelContrastThreshold(opts->GetSubpixelContrastThreshold());
  this->SetEndpointSearchIterations(opts->GetEndpointSearchIterations());
  this->SetUseHighQualityEndpoints(opts->GetUseHighQualityEndpoints());
  this->SetDebugOptionValue(opts->GetDebugOptionValue());
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::SetUseHighQualityEndpoints(bool val)
{
  if (this->UseHighQualityEndpoints != val)
  {
    this->NeedToRebuildShader = true;
    this->Modified();
    this->UseHighQualityEndpoints = val;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::SetDebugOptionValue(vtkFXAAOptions::DebugOption opt)
{
  if (this->DebugOptionValue != opt)
  {
    this->NeedToRebuildShader = true;
    this->Modified();
    this->DebugOptionValue = opt;
  }
}

//------------------------------------------------------------------------------
vtkOpenGLFXAAFilter::vtkOpenGLFXAAFilter()
  : BlendState(false)
  , DepthTestState(false)
  , PreparationTimer(new vtkOpenGLRenderTimer)
  , FXAATimer(new vtkOpenGLRenderTimer)
  , RelativeContrastThreshold(1.f / 8.f)
  , HardContrastThreshold(1.f / 16.f)
  , SubpixelBlendLimit(3.f / 4.f)
  , SubpixelContrastThreshold(1.f / 4.f)
  , EndpointSearchIterations(12)
  , UseHighQualityEndpoints(true)
  , DebugOptionValue(vtkFXAAOptions::FXAA_NO_DEBUG)
  , NeedToRebuildShader(true)
  , Renderer(nullptr)
  , Input(nullptr)
  , QHelper(nullptr)
{
  std::fill(this->Viewport, this->Viewport + 4, 0);
}

//------------------------------------------------------------------------------
vtkOpenGLFXAAFilter::~vtkOpenGLFXAAFilter()
{
  if (this->QHelper)
  {
    delete this->QHelper;
    this->QHelper = nullptr;
  }
  this->FreeGLObjects();
  delete PreparationTimer;
  delete FXAATimer;
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::Prepare()
{
  this->Renderer->GetTiledSizeAndOrigin(
    &this->Viewport[2], &this->Viewport[3], &this->Viewport[0], &this->Viewport[1]);

  // Check if we need to create a new working texture:
  if (this->Input)
  {
    unsigned int rendererWidth = static_cast<unsigned int>(this->Viewport[2]);
    unsigned int rendererHeight = static_cast<unsigned int>(this->Viewport[3]);
    if (this->Input->GetWidth() != rendererWidth || this->Input->GetHeight() != rendererHeight)
    {
      this->FreeGLObjects();
    }
  }

  if (!this->Input)
  {
    this->CreateGLObjects();
  }

  vtkOpenGLState* ostate = this->Renderer->GetState();
  this->BlendState = ostate->GetEnumState(GL_BLEND);
  this->DepthTestState = ostate->GetEnumState(GL_DEPTH_TEST);

#ifdef __APPLE__
  // Restore viewport to its original size. This is necessary only on
  // MacOS when HiDPI is supported. Enabling HiDPI has the side effect that
  // Cocoa will start overriding any glViewport calls in application code.
  // For reference, see QCocoaWindow::initialize().
  int origin[2];
  int usize, vsize;
  this->Renderer->GetTiledSizeAndOrigin(&usize, &vsize, origin, origin + 1);
  ostate->vtkglViewport(origin[0], origin[1], usize, vsize);
#endif

  ostate->vtkglDisable(GL_BLEND);
  ostate->vtkglDisable(GL_DEPTH_TEST);

  vtkOpenGLCheckErrorMacro("Error after saving GL state.");
}

//------------------------------------------------------------------------------
// Delete the vtkObject subclass pointed at by ptr if it is set.
namespace
{
template <typename T>
void DeleteHelper(T*& ptr)
{
  if (ptr)
  {
    ptr->Delete();
    ptr = nullptr;
  }
}
} // end anon namespace

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::FreeGLObjects()
{
  DeleteHelper(this->Input);
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::CreateGLObjects()
{
  assert(!this->Input);
  this->Input = vtkTextureObject::New();
  vtkOpenGLRenderWindow* renWin =
    static_cast<vtkOpenGLRenderWindow*>(this->Renderer->GetRenderWindow());
  this->Input->SetContext(renWin);
  this->Input->SetFormat(GL_RGB);

  // we need to get the format of current color buffer in order to allocate the right format
  // for the texture used in FXAA
  int internalFormat = renWin->GetColorBufferInternalFormat(0);

  if (internalFormat != 0)
  {
    this->Input->SetInternalFormat(static_cast<unsigned int>(internalFormat));
  }
  else // the query failed, fallback to classic texture
  {
    // ES doesn't support GL_RGB8, and OpenGL 3 doesn't support GL_RGB.
    // What a world.
#ifdef GL_ES_VERSION_3_0
    this->Input->SetInternalFormat(GL_RGB);
#else  // OpenGL ES
    this->Input->SetInternalFormat(GL_RGB8);
#endif // OpenGL ES
  }

  // Required for FXAA, since we interpolate texels for blending.
  this->Input->SetMinificationFilter(vtkTextureObject::Linear);
  this->Input->SetMagnificationFilter(vtkTextureObject::Linear);

  // Clamp to edge, since we'll be sampling off-texture texels:
  this->Input->SetWrapS(vtkTextureObject::ClampToEdge);
  this->Input->SetWrapT(vtkTextureObject::ClampToEdge);
  this->Input->SetWrapR(vtkTextureObject::ClampToEdge);

  this->Input->Allocate2D(
    this->Viewport[2], this->Viewport[3], 4, vtkTypeTraits<vtkTypeUInt8>::VTK_TYPE_ID);
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::LoadInput()
{
  this->Input->CopyFromFrameBuffer(
    this->Viewport[0], this->Viewport[1], 0, 0, this->Viewport[2], this->Viewport[3]);
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::ApplyFilter()
{
  typedef vtkOpenGLRenderUtilities GLUtil;

  vtkOpenGLRenderWindow* renWin =
    static_cast<vtkOpenGLRenderWindow*>(this->Renderer->GetRenderWindow());

  this->Input->Activate();

  if (this->NeedToRebuildShader)
  {
    delete this->QHelper;
    this->QHelper = nullptr;
    this->NeedToRebuildShader = false;
  }

  if (!this->QHelper)
  {
    std::string fragShader = vtkFXAAFilterFS;
    this->SubstituteFragmentShader(fragShader);
    this->QHelper = new vtkOpenGLQuadHelper(renWin, GLUtil::GetFullScreenQuadVertexShader().c_str(),
      fragShader.c_str(), GLUtil::GetFullScreenQuadGeometryShader().c_str());
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->QHelper->Program);
  }

  vtkShaderProgram* program = this->QHelper->Program;
  program->SetUniformi("Input", this->Input->GetTextureUnit());
  float invTexSize[2] = { 1.f / static_cast<float>(this->Viewport[2]),
    1.f / static_cast<float>(this->Viewport[3]) };
  program->SetUniform2f("InvTexSize", invTexSize);

  program->SetUniformf("RelativeContrastThreshold", this->RelativeContrastThreshold);
  program->SetUniformf("HardContrastThreshold", this->HardContrastThreshold);
  program->SetUniformf("SubpixelBlendLimit", this->SubpixelBlendLimit);
  program->SetUniformf("SubpixelContrastThreshold", this->SubpixelContrastThreshold);
  program->SetUniformi("EndpointSearchIterations", this->EndpointSearchIterations);

  this->QHelper->Render();

  this->Input->Deactivate();
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::SubstituteFragmentShader(std::string& fragShader)
{
  if (this->UseHighQualityEndpoints)
  {
    vtkShaderProgram::Substitute(
      fragShader, "//VTK::EndpointAlgo::Def", "#define FXAA_USE_HIGH_QUALITY_ENDPOINTS");
  }

#define DEBUG_OPT_CASE(optName)                                                                    \
  case vtkFXAAOptions::optName:                                                                    \
    vtkShaderProgram::Substitute(fragShader, "//VTK::DebugOptions::Def", "#define " #optName);     \
    break

  switch (this->DebugOptionValue)
  {
    default:
    case vtkFXAAOptions::FXAA_NO_DEBUG:
      break;
      DEBUG_OPT_CASE(FXAA_DEBUG_SUBPIXEL_ALIASING);
      DEBUG_OPT_CASE(FXAA_DEBUG_EDGE_DIRECTION);
      DEBUG_OPT_CASE(FXAA_DEBUG_EDGE_NUM_STEPS);
      DEBUG_OPT_CASE(FXAA_DEBUG_EDGE_DISTANCE);
      DEBUG_OPT_CASE(FXAA_DEBUG_EDGE_SAMPLE_OFFSET);
      DEBUG_OPT_CASE(FXAA_DEBUG_ONLY_SUBPIX_AA);
      DEBUG_OPT_CASE(FXAA_DEBUG_ONLY_EDGE_AA);
  }

#undef DEBUG_OPT_CASE
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::Finalize()
{
  vtkOpenGLState* ostate = this->Renderer->GetState();
  if (this->BlendState)
  {
    ostate->vtkglEnable(GL_BLEND);
  }
  if (this->DepthTestState)
  {
    ostate->vtkglEnable(GL_DEPTH_TEST);
  }

  vtkOpenGLCheckErrorMacro("Error after restoring GL state.");
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::StartTimeQuery(vtkOpenGLRenderTimer* timer)
{
  // Since it may take a few frames for the results to become available,
  // check if we've started the timer already.
  if (!timer->Started())
  {
    timer->Start();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::EndTimeQuery(vtkOpenGLRenderTimer* timer)
{
  // Since it may take a few frames for the results to become available,
  // check if we've stopped the timer already.
  if (!timer->Stopped())
  {
    timer->Stop();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::PrintBenchmark()
{
  if (this->PreparationTimer->Ready() && this->FXAATimer->Ready())
  {

#ifdef FXAA_BENCHMARK
    int numPixels = this->Input->GetWidth() * this->Input->GetHeight();
    float ptime = this->PreparationTimer->GetElapsedMilliseconds();
    float ftime = this->FXAATimer->GetElapsedMilliseconds();
    float ttime = ptime + ftime;

    float ptimePerPixel =
      (this->PreparationTimer->GetElapsedNanoseconds() / static_cast<float>(numPixels));
    float ftimePerPixel =
      (this->FXAATimer->GetElapsedNanoseconds() / static_cast<float>(numPixels));
    float ttimePerPixel = ptimePerPixel + ftimePerPixel;

    std::cerr << "FXAA Info:\n"
              << " - Number of pixels: " << numPixels << "\n"
              << " - Preparation time: " << ptime << "ms (" << ptimePerPixel << "ns per pixel)\n"
              << " - FXAA time: " << ftime << "ms (" << ftimePerPixel << "ns per pixel)\n"
              << " - Total time: " << ttime << "ms (" << ttimePerPixel << "ns per pixel)\n";
#endif // FXAA_BENCHMARK

    this->PreparationTimer->Reset();
    this->FXAATimer->Reset();
  }
}
