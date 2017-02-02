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
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderTimer.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
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

vtkStandardNewMacro(vtkOpenGLFXAAFilter)

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RelativeContrastThreshold: "
     << this->RelativeContrastThreshold << "\n";
  os << indent << "HardContrastThreshold: "
     << this->HardContrastThreshold << "\n";
  os << indent << "SubpixelBlendLimit: " <<
        this->SubpixelBlendLimit << "\n";
  os << indent << "SubpixelContrastThreshold: "
     << this->SubpixelContrastThreshold << "\n";
  os << indent << "EndpointSearchIterations: "
     << this->EndpointSearchIterations << "\n";
  os << indent << "UseHighQualityEndpoints: "
     << this->UseHighQualityEndpoints << "\n";

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
void vtkOpenGLFXAAFilter::Execute(vtkOpenGLRenderer *ren)
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

  this->Renderer = NULL;
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::ReleaseGraphicsResources()
{
  this->FreeGLObjects();
  this->PreparationTimer->ReleaseGraphicsResources();
  this->FXAATimer->ReleaseGraphicsResources();
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::UpdateConfiguration(vtkFXAAOptions *opts)
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
  : BlendState(false),
    DepthTestState(false),
    PreparationTimer(new vtkOpenGLRenderTimer),
    FXAATimer(new vtkOpenGLRenderTimer),
    RelativeContrastThreshold(1.f/8.f),
    HardContrastThreshold(1.f/16.f),
    SubpixelBlendLimit(3.f/4.f),
    SubpixelContrastThreshold(1.f/4.f),
    EndpointSearchIterations(12),
    UseHighQualityEndpoints(true),
    DebugOptionValue(vtkFXAAOptions::FXAA_NO_DEBUG),
    NeedToRebuildShader(true),
    Renderer(NULL),
    Input(NULL),
    Program(NULL),
    VAO(NULL),
    VBO(NULL)
{
  std::fill(this->Viewport, this->Viewport + 4, 0);
}

//------------------------------------------------------------------------------
vtkOpenGLFXAAFilter::~vtkOpenGLFXAAFilter()
{
  this->FreeGLObjects();
  delete PreparationTimer;
  delete FXAATimer;
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::Prepare()
{
  this->Renderer->GetTiledSizeAndOrigin(&this->Viewport[2], &this->Viewport[3],
                                        &this->Viewport[0], &this->Viewport[1]);

  // Check if we need to create a new working texture:
  if (this->Input)
  {
    unsigned int rendererWidth = static_cast<unsigned int>(this->Viewport[2]);
    unsigned int rendererHeight = static_cast<unsigned int>(this->Viewport[3]);
    if (this->Input->GetWidth()  != rendererWidth ||
        this->Input->GetHeight() != rendererHeight)
    {
      this->FreeGLObjects();
    }
  }

  if (!this->Input)
  {
    this->CreateGLObjects();
  }

  this->BlendState = glIsEnabled(GL_BLEND) == GL_TRUE;
  this->DepthTestState = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;

  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);

  vtkOpenGLCheckErrorMacro("Error after saving GL state.");
}

//------------------------------------------------------------------------------
// Delete the vtkObject subclass pointed at by ptr if it is set.
namespace {
template <typename T> void DeleteHelper(T *& ptr)
{
  if (ptr)
  {
    ptr->Delete();
    ptr = NULL;
  }
}
} // end anon namespace

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::FreeGLObjects()
{
  DeleteHelper(this->Input);
//  DeleteHelper(this->Program); // Managed by the shader cache
  DeleteHelper(this->VAO);
  DeleteHelper(this->VBO);
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::CreateGLObjects()
{
  assert(!this->Input);
  this->Input = vtkTextureObject::New();
  this->Input->SetContext(static_cast<vtkOpenGLRenderWindow*>(
                            this->Renderer->GetRenderWindow()));
  this->Input->SetFormat(GL_RGB);

  // ES doesn't support GL_RGB8, and OpenGL 3 doesn't support GL_RGB.
  // What a world.
#if defined(GL_ES_VERSION_3_0)
  this->Input->SetInternalFormat(GL_RGB);
#else // OpenGL ES
  this->Input->SetInternalFormat(GL_RGB8);
#endif // OpenGL ES

  // Required for FXAA, since we interpolate texels for blending.
  this->Input->SetMinificationFilter(vtkTextureObject::Linear);
  this->Input->SetMagnificationFilter(vtkTextureObject::Linear);

  // Clamp to edge, since we'll be sampling off-texture texels:
  this->Input->SetWrapS(vtkTextureObject::ClampToEdge);
  this->Input->SetWrapT(vtkTextureObject::ClampToEdge);
  this->Input->SetWrapR(vtkTextureObject::ClampToEdge);

  this->Input->Allocate2D(this->Viewport[2], this->Viewport[3], 4,
                          vtkTypeTraits<vtkTypeUInt8>::VTK_TYPE_ID);
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::LoadInput()
{
  this->Input->CopyFromFrameBuffer(this->Viewport[0], this->Viewport[1], 0, 0,
                                   this->Viewport[2], this->Viewport[3]);
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::ApplyFilter()
{
  typedef vtkOpenGLRenderUtilities GLUtil;

  vtkOpenGLRenderWindow *renWin = static_cast<vtkOpenGLRenderWindow*>(
        this->Renderer->GetRenderWindow());

  this->Input->Activate();

  if (this->NeedToRebuildShader)
  {
    DeleteHelper(this->VAO);
    DeleteHelper(this->VBO);
    this->Program = NULL; // Don't free, shader cache manages these.
    this->NeedToRebuildShader = false;
  }

  if (!this->Program)
  {
    std::string fragShader = vtkFXAAFilterFS;
    this->SubstituteFragmentShader(fragShader);
    this->Program = renWin->GetShaderCache()->ReadyShaderProgram(
          GLUtil::GetFullScreenQuadVertexShader().c_str(),
          fragShader.c_str(),
          GLUtil::GetFullScreenQuadGeometryShader().c_str());
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->Program);
  }

  if (!this->Program)
  {
    return;
  }

  if (!this->VAO)
  {
    this->VBO = vtkOpenGLBufferObject::New();
    this->VAO = vtkOpenGLVertexArrayObject::New();
    GLUtil::PrepFullScreenVAO(this->VBO, this->VAO, this->Program);
  }

  this->Program->SetUniformi("Input", this->Input->GetTextureUnit());
  float invTexSize[2] = { 1.f / static_cast<float>(this->Viewport[2]),
                          1.f / static_cast<float>(this->Viewport[3]) };
  this->Program->SetUniform2f("InvTexSize", invTexSize);

  this->Program->SetUniformf("RelativeContrastThreshold",
                             this->RelativeContrastThreshold);
  this->Program->SetUniformf("HardContrastThreshold",
                             this->HardContrastThreshold);
  this->Program->SetUniformf("SubpixelBlendLimit",
                             this->SubpixelBlendLimit);
  this->Program->SetUniformf("SubpixelContrastThreshold",
                             this->SubpixelContrastThreshold);
  this->Program->SetUniformi("EndpointSearchIterations",
                             this->EndpointSearchIterations);

  this->VAO->Bind();
  GLUtil::DrawFullScreenQuad();
  this->VAO->Release();
  this->Input->Deactivate();
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::SubstituteFragmentShader(std::string &fragShader)
{
  if (this->UseHighQualityEndpoints)
  {
    vtkShaderProgram::Substitute(fragShader, "//VTK::EndpointAlgo::Def",
                                 "#define FXAA_USE_HIGH_QUALITY_ENDPOINTS");
  }

#define DEBUG_OPT_CASE(optName) \
  case vtkFXAAOptions::optName: \
    vtkShaderProgram::Substitute(fragShader, "//VTK::DebugOptions::Def", \
                                 "#define " #optName); \
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
  if (this->BlendState)
  {
    glEnable(GL_BLEND);
  }
  if (this->DepthTestState)
  {
    glEnable(GL_DEPTH_TEST);
  }

  vtkOpenGLCheckErrorMacro("Error after restoring GL state.");
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::StartTimeQuery(vtkOpenGLRenderTimer *timer)
{
  // Since it may take a few frames for the results to become available,
  // check if we've started the timer already.
  if (!timer->Started())
  {
    timer->Start();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLFXAAFilter::EndTimeQuery(vtkOpenGLRenderTimer *timer)
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
  if (this->PreparationTimer->Ready() &&
      this->FXAATimer->Ready())
  {

#ifdef FXAA_BENCHMARK
    int numPixels = this->Input->GetWidth() * this->Input->GetHeight();
    float ptime = this->PreparationTimer->GetElapsedMilliseconds();
    float ftime = this->FXAATimer->GetElapsedMilliseconds();
    float ttime = ptime + ftime;

    float ptimePerPixel = (this->PreparationTimer->GetElapsedNanoseconds() /
                           static_cast<float>(numPixels));
    float ftimePerPixel = (this->FXAATimer->GetElapsedNanoseconds() /
                           static_cast<float>(numPixels));
    float ttimePerPixel =  ptimePerPixel + ftimePerPixel;

    std::cerr << "FXAA Info:\n"
              << " - Number of pixels: " << numPixels << "\n"
              << " - Preparation time: " << ptime << "ms ("
              << ptimePerPixel << "ns per pixel)\n"
              << " - FXAA time: " << ftime << "ms ("
              << ftimePerPixel << "ns per pixel)\n"
              << " - Total time: " << ttime << "ms ("
              << ttimePerPixel << "ns per pixel)\n";
#endif // FXAA_BENCHMARK

    this->PreparationTimer->Reset();
    this->FXAATimer->Reset();
  }
}
