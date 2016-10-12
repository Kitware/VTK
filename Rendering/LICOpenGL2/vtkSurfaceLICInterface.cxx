/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSurfaceLICInterface.h"

#include "vtkFloatArray.h"
#include "vtkFrameBufferObject2.h"
#include "vtkImageData.h"
#include "vtkLineIntegralConvolution2D.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkPainterCommunicator.h"
#include "vtkPixelBufferObject.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkShaderProgram.h"
#include "vtkSurfaceLICComposite.h"
#include "vtkTextureObjectVS.h"

#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLIndexBufferObject.h"

#include "vtkLICNoiseHelper.h"
#include "vtkSurfaceLICHelper.h"

#include <cstring>
#include <algorithm>
#include <limits>
#include <vector>
#include <deque>
#include <cstdlib>

#include "vtkSurfaceLICInterface_SC.h"
#include "vtkSurfaceLICInterface_CE.h"
#include "vtkSurfaceLICInterface_DCpy.h"
#include "vtkTextureObjectVS.h"

typedef vtkLineIntegralConvolution2D vtkLIC2D;

// write intermediate results to disk for debugging
#define vtkSurfaceLICInterfaceDEBUG 0
#if vtkSurfaceLICInterfaceDEBUG >= 2
#include "vtkTextureIO.h"
#include <sstream>
using std::ostringstream;
//----------------------------------------------------------------------------
static
std::string mpifn(vtkPainterCommunicator *comm, const char *fn)
{
  ostringstream oss;
  oss << comm->GetRank() << "_" << fn;
  return oss.str();
}
#endif

vtkObjectFactoryNewMacro(vtkSurfaceLICInterface);

//----------------------------------------------------------------------------
vtkSurfaceLICInterface::vtkSurfaceLICInterface()
{
  this->Internals = new vtkSurfaceLICHelper();

  this->Enable = 1;
  this->AlwaysUpdate = 0;

  this->StepSize = 1;
  this->NumberOfSteps = 20;
  this->NormalizeVectors = 1;

  this->EnhancedLIC = 1;

  this->EnhanceContrast = 0;
  this->LowLICContrastEnhancementFactor = 0.0;
  this->HighLICContrastEnhancementFactor = 0.0;
  this->LowColorContrastEnhancementFactor = 0.0;
  this->HighColorContrastEnhancementFactor = 0.0;
  this->AntiAlias = 0;
  this->ColorMode = COLOR_MODE_BLEND;
  this->LICIntensity = 0.8;
  this->MapModeBias = 0.0;

  this->GenerateNoiseTexture = 0;
  this->NoiseType = NOISE_TYPE_GAUSSIAN;
  this->NoiseTextureSize = 200;
  this->MinNoiseValue = 0.0;
  this->MaxNoiseValue = 0.8;
  this->NoiseGrainSize = 1;
  this->NumberOfNoiseLevels = 256;
  this->ImpulseNoiseProbability = 1.0;
  this->ImpulseNoiseBackgroundValue = 0.0;
  this->NoiseGeneratorSeed = 1;

  this->MaskOnSurface = 0;
  this->MaskThreshold = 0.0;
  this->MaskIntensity = 0.0;
  this->MaskColor[0] = 0.5;
  this->MaskColor[1] = 0.5;
  this->MaskColor[2] = 0.5;

  this->CompositeStrategy = COMPOSITE_AUTO;
}

//----------------------------------------------------------------------------
vtkSurfaceLICInterface::~vtkSurfaceLICInterface()
{
  #if vtkSurfaceLICInterfaceDEBUG >= 1
  cerr << "=====vtkSurfaceLICInterface::~vtkSurfaceLICInterface" << endl;
  #endif
  this->ReleaseGraphicsResources(this->Internals->Context);
  delete this->Internals;
}

void vtkSurfaceLICInterface::ShallowCopy(vtkSurfaceLICInterface *m)
{
  this->SetNumberOfSteps(m->GetNumberOfSteps());
  this->SetStepSize(m->GetStepSize());
  this->SetEnhancedLIC(m->GetEnhancedLIC());
  this->SetGenerateNoiseTexture(m->GetGenerateNoiseTexture());
  this->SetNoiseType(m->GetNoiseType());
  this->SetNormalizeVectors(m->GetNormalizeVectors());
  this->SetNoiseTextureSize(m->GetNoiseTextureSize());
  this->SetNoiseGrainSize(m->GetNoiseGrainSize());
  this->SetMinNoiseValue(m->GetMinNoiseValue());
  this->SetMaxNoiseValue(m->GetMaxNoiseValue());
  this->SetNumberOfNoiseLevels(m->GetNumberOfNoiseLevels());
  this->SetImpulseNoiseProbability(m->GetImpulseNoiseProbability());
  this->SetImpulseNoiseBackgroundValue(m->GetImpulseNoiseBackgroundValue());
  this->SetNoiseGeneratorSeed(m->GetNoiseGeneratorSeed());
  this->SetEnhanceContrast(m->GetEnhanceContrast());
  this->SetLowLICContrastEnhancementFactor(
    m->GetLowLICContrastEnhancementFactor());
  this->SetHighLICContrastEnhancementFactor(
    m->GetHighLICContrastEnhancementFactor());
  this->SetLowColorContrastEnhancementFactor(
    m->GetLowColorContrastEnhancementFactor());
  this->SetHighColorContrastEnhancementFactor(
    m->GetHighColorContrastEnhancementFactor());
  this->SetAntiAlias(m->GetAntiAlias());
  this->SetColorMode(m->GetColorMode());
  this->SetLICIntensity(m->GetLICIntensity());
  this->SetMapModeBias(m->GetMapModeBias());
  this->SetMaskOnSurface(m->GetMaskOnSurface());
  this->SetMaskThreshold(m->GetMaskThreshold());
  this->SetMaskIntensity(m->GetMaskIntensity());
  this->SetMaskColor(m->GetMaskColor());
  this->SetEnable(m->GetEnable());
}

void vtkSurfaceLICInterface::UpdateCommunicator(
  vtkRenderer *renderer, vtkActor *actor, vtkDataObject *input)
{
    // commented out as camera and data changes also
    // require a communicator update, currently the
    // test does not include these
//  if (this->NeedToUpdateCommunicator())
  {
    // create a communicator that contains only ranks
    // that have visible data. In parallel this is a
    // collective operation accross all ranks. In
    // serial this is a no-op.
    this->CreateCommunicator(renderer,actor, input);
  }
}

void vtkSurfaceLICInterface::PrepareForGeometry()
{
  // save the active fbo and its draw buffer
  this->PrevDrawBuf = 0;
  glGetIntegerv(GL_DRAW_BUFFER, &this->PrevDrawBuf);

  this->PrevFbo = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &this->PrevFbo);

  // ------------------------------------------- render geometry, project vectors onto screen, etc
  // setup our fbo
  vtkFrameBufferObject2 *fbo = this->Internals->FBO;
  fbo->SaveCurrentBindings();
  fbo->Bind(GL_FRAMEBUFFER);
  fbo->AddDepthAttachment(GL_DRAW_FRAMEBUFFER, this->Internals->DepthImage);
  fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 0U, this->Internals->GeometryImage);
  fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 1U, this->Internals->VectorImage);
  fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 2U, this->Internals->MaskVectorImage);
  fbo->ActivateDrawBuffers(3);
  vtkCheckFrameBufferStatusMacro(GL_FRAMEBUFFER);

  // clear internal color and depth buffers
  // the LIC'er requires *all* fragments in the vector
  // texture to be initialized to 0
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
}

void vtkSurfaceLICInterface::CompletedGeometry()
{
  vtkFrameBufferObject2 *fbo = this->Internals->FBO;
  fbo->RemoveRenDepthAttachment(GL_DRAW_FRAMEBUFFER);
  fbo->RemoveTexColorAttachment(GL_DRAW_FRAMEBUFFER, 0U);
  fbo->RemoveTexColorAttachment(GL_DRAW_FRAMEBUFFER, 1U);
  fbo->RemoveTexColorAttachment(GL_DRAW_FRAMEBUFFER, 2U);
  fbo->DeactivateDrawBuffers();
  fbo->UnBind(GL_FRAMEBUFFER);

  #if vtkSurfaceLICInterfaceDEBUG >= 2
  vtkPainterCommunicator *comm = this->GetCommunicator();

  vtkTextureIO::Write(
        mpifn(comm,"slicp_geometry_image.vtm"),
        this->Internals->GeometryImage,
        this->Internals->BlockExts);
  vtkTextureIO::Write(
        mpifn(comm,"slicp_vector_image.vtm"),
        this->Internals->VectorImage,
        this->Internals->BlockExts);
  vtkTextureIO::Write(
        mpifn(comm,"slicp_mask_vector_image.vtm"),
        this->Internals->MaskVectorImage,
        this->Internals->BlockExts);
  vtkTextureIO::Write(
        mpifn(comm,"slicp_depth_image.vtm"),
        this->Internals->DepthImage,
        this->Internals->BlockExts);
  #endif
}


void vtkSurfaceLICInterface::GatherVectors()
{
  vtkPixelExtent viewExt(
        this->Internals->Viewsize[0],
        this->Internals->Viewsize[1]);

  vtkPainterCommunicator *comm = this->GetCommunicator();

  // get tight screen space bounds to reduce communication/computation
  vtkPixelBufferObject *vecPBO = this->Internals->VectorImage->Download();
  void *pVecPBO = vecPBO->MapPackedBuffer();

  this->Internals->GetPixelBounds(
          (float*)pVecPBO,
          this->Internals->Viewsize[0],
          this->Internals->BlockExts);

  // initialize compositor
  this->Internals->Compositor->Initialize(
        viewExt,
        this->Internals->BlockExts,
        this->CompositeStrategy,
        this->StepSize,
        this->NumberOfSteps,
        this->NormalizeVectors,
        this->EnhancedLIC,
        this->AntiAlias);

  if (comm->GetMPIInitialized())
  {
    // parallel run
    // need to use the communicator provided by the rendering engine
    this->Internals->Compositor->SetCommunicator(comm);

    // build compositing program and set up the screen space decomp
    // with guard pixels
    int iErr = 0;
    iErr = this->Internals->Compositor->BuildProgram((float*)pVecPBO);
    if (iErr)
    {
      vtkErrorMacro("Failed to construct program, reason " << iErr);
    }

    // composite vectors
    vtkTextureObject *compositeVectors = this->Internals->CompositeVectorImage;
    iErr = this->Internals->Compositor->Gather(
            pVecPBO,
            VTK_FLOAT,
            4,
            compositeVectors);
    if (iErr)
    {
      vtkErrorMacro("Failed to composite vectors, reason  " << iErr);
    }

    // composite mask vectors
    vtkTextureObject *compositeMaskVectors = this->Internals->CompositeMaskVectorImage;
    vtkPixelBufferObject *maskVecPBO = this->Internals->MaskVectorImage->Download();
    void *pMaskVecPBO = maskVecPBO->MapPackedBuffer();
    iErr = this->Internals->Compositor->Gather(
            pMaskVecPBO,
            VTK_FLOAT,
            4,
            compositeMaskVectors);
    if (iErr)
    {
      vtkErrorMacro("Failed to composite mask vectors, reason " << iErr);
    }
    maskVecPBO->UnmapPackedBuffer();
    maskVecPBO->Delete();

    // restore the default communicator
    this->Internals->Compositor->RestoreDefaultCommunicator();

    #if vtkSurfaceLICInterfaceDEBUG >= 2
    vtkTextureIO::Write(
           mpifn(comm,"slicp_new_vector_image.vtm"),
           this->Internals->CompositeVectorImage,
           this->Internals->Compositor->GetDisjointGuardExtents());

    vtkTextureIO::Write(
           mpifn(comm,"slicp_new_mask_vector_image.vtm"),
           this->Internals->CompositeMaskVectorImage,
           this->Internals->Compositor->GetDisjointGuardExtents());
    #endif
  }
  else
  {
    // serial run
    // make the decomposition disjoint and add guard pixels
    this->Internals->Compositor->InitializeCompositeExtents((float*)pVecPBO);

    // use the lic decomp from here on out, in serial we have this
    // flexibility because we don't need to worry about ordered compositing
    // or IceT's scissor boxes
    this->Internals->BlockExts
       = this->Internals->Compositor->GetCompositeExtents();

    // pass through without compositing
    this->Internals->CompositeVectorImage = this->Internals->VectorImage;
    this->Internals->CompositeMaskVectorImage = this->Internals->MaskVectorImage;
  }

 vecPBO->UnmapPackedBuffer();
 vecPBO->Delete();
}

void vtkSurfaceLICInterface::ApplyLIC()
{
  vtkPainterCommunicator *comm = this->GetCommunicator();

  vtkPixelExtent viewExt(
        this->Internals->Viewsize[0],
        this->Internals->Viewsize[1]);

  #if vtkSurfaceLICInterfaceDEBUG >= 2
  ostringstream oss;
  if ( this->GenerateNoiseTexture )
  {
    const char *noiseType[3]={"unif","gauss","perl"};
    oss
     << "slicp_noise_"
     << noiseType[this->NoiseType]
     << "_size_" << this->NoiseTextureSize
     << "_grain_" << this->NoiseGrainSize
     << "_minval_" << this->MinNoiseValue
     << "_maxval_" << this->MaxNoiseValue
     << "_nlevels_" << this->NumberOfNoiseLevels
     << "_impulseprob_" << this->ImpulseNoiseProbability
     << "_impulseprob_" << this->ImpulseNoiseBackgroundValue
     << ".vtk";
  }
  else
  {
    oss << "slicp_noise_default.vtk";
  }
  vtkTextureIO::Write(
        mpifn(comm, oss.str().c_str()),
        this->Internals->NoiseImage);
  #endif

  // TODO -- this means that the steps size is a function
  // of aspect ratio which is pretty insane...
  // convert from window units to texture units
  // this isn't correct since there's no way to account
  // for anisotropy in the trasnform to texture space
  double tcScale[2] = {
        1.0/this->Internals->Viewsize[0],
        1.0/this->Internals->Viewsize[1]};

  double stepSize
    = this->StepSize*sqrt(tcScale[0]*tcScale[0]+tcScale[1]*tcScale[1]);

  stepSize = stepSize <= 0.0 ? 1.0e-10 : stepSize;

  // configure image lic
  vtkLineIntegralConvolution2D *LICer = this->Internals->LICer;

  LICer->SetStepSize(stepSize);
  LICer->SetNumberOfSteps(this->NumberOfSteps);
  LICer->SetEnhancedLIC(this->EnhancedLIC);
  switch (this->EnhanceContrast)
  {
    case ENHANCE_CONTRAST_LIC:
    case ENHANCE_CONTRAST_BOTH:
      LICer->SetEnhanceContrast(vtkLIC2D::ENHANCE_CONTRAST_ON);
      break;
    default:
      LICer->SetEnhanceContrast(vtkLIC2D::ENHANCE_CONTRAST_OFF);
  }
  LICer->SetLowContrastEnhancementFactor(this->LowLICContrastEnhancementFactor);
  LICer->SetHighContrastEnhancementFactor(this->HighLICContrastEnhancementFactor);
  LICer->SetAntiAlias(this->AntiAlias);
  LICer->SetComponentIds(0, 1);
  LICer->SetNormalizeVectors(this->NormalizeVectors);
  LICer->SetMaskThreshold(this->MaskThreshold);
  LICer->SetCommunicator(comm);

  // loop over composited extents
  const std::deque<vtkPixelExtent> &compositeExts
    = this->Internals->Compositor->GetCompositeExtents();

  const std::deque<vtkPixelExtent> &disjointGuardExts
    = this->Internals->Compositor->GetDisjointGuardExtents();

  this->Internals->LICImage.TakeReference(
       LICer->Execute(
            viewExt,            // screen extent
            disjointGuardExts,  // disjoint extent of valid vectors
            compositeExts,      // disjoint extent where lic is needed
            this->Internals->CompositeVectorImage,
            this->Internals->CompositeMaskVectorImage,
            this->Internals->NoiseImage));

  if (!this->Internals->LICImage)
  {
    vtkErrorMacro("Failed to compute image LIC");
    return;
  }

  #if vtkSurfaceLICInterfaceDEBUG >= 2
  vtkTextureIO::Write(
        mpifn(comm,"slicp_lic.vtm"),
        this->Internals->LICImage,
        compositeExts);
  #endif

  // ------------------------------------------- move from LIC decomp back to geometry decomp
  if ( comm->GetMPIInitialized()
    && (this->Internals->Compositor->GetStrategy()!=COMPOSITE_INPLACE ) )
  {
    #ifdef vtkSurfaceLICMapperTIME
    this->StartTimerEvent("vtkSurfaceLICMapper::ScatterLIC");
    #endif

    // parallel run
    // need to use the communicator provided by the rendering engine
    this->Internals->Compositor->SetCommunicator(comm);

    vtkPixelBufferObject *licPBO = this->Internals->LICImage->Download();
    void *pLicPBO = licPBO->MapPackedBuffer();
    vtkTextureObject *newLicImage = NULL;
    int iErr = this->Internals->Compositor->Scatter(pLicPBO, VTK_FLOAT, 4, newLicImage);
    if (iErr)
    {
      vtkErrorMacro("Failed to scatter lic");
    }
    licPBO->UnmapPackedBuffer();
    licPBO->Delete();
    this->Internals->LICImage = NULL;
    this->Internals->LICImage = newLicImage;
    newLicImage->Delete();

    // restore the default communicator
    this->Internals->Compositor->RestoreDefaultCommunicator();

    #ifdef vtkSurfaceLICMapperTIME
    this->EndTimerEvent("vtkSurfaceLICMapper::ScatterLIC");
    #endif
    #if vtkSurfaceLICMapperDEBUG >= 2
    vtkTextureIO::Write(
          mpifn(comm,"slicp_new_lic.vtm"),
          this->Internals->LICImage,
          this->Internals->BlockExts);
    #endif
  }
}

void vtkSurfaceLICInterface::CombineColorsAndLIC()
{
  vtkOpenGLRenderWindow *renWin = this->Internals->Context;

  vtkPainterCommunicator *comm = this->GetCommunicator();

  vtkPixelExtent viewExt(
        this->Internals->Viewsize[0],
        this->Internals->Viewsize[1]);

  vtkFrameBufferObject2 *fbo = this->Internals->FBO;
  fbo->SaveCurrentBindings();
  fbo->Bind(GL_FRAMEBUFFER);
  fbo->InitializeViewport(this->Internals->Viewsize[0], this->Internals->Viewsize[1]);
  fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 0U, this->Internals->RGBColorImage);
  fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 1U, this->Internals->HSLColorImage);
  fbo->ActivateDrawBuffers(2U);
  vtkCheckFrameBufferStatusMacro(GL_FRAMEBUFFER);

  // clear the parts of the screen which we will modify
  glEnable(GL_SCISSOR_TEST);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  size_t nBlocks = this->Internals->BlockExts.size();
  for (size_t e=0; e<nBlocks; ++e)
  {
    vtkPixelExtent ext = this->Internals->BlockExts[e];
    ext.Grow(2); // halo for linear filtering
    ext &= viewExt;

    unsigned int extSize[2];
    ext.Size(extSize);

    glScissor(ext[0], ext[2], extSize[0], extSize[1]);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  glDisable(GL_SCISSOR_TEST);

  this->Internals->VectorImage->Activate();
  this->Internals->GeometryImage->Activate();
  this->Internals->LICImage->Activate();

  if (!this->Internals->ColorPass->Program)
  {
    this->InitializeResources();
  }
  vtkShaderProgram *colorPass = this->Internals->ColorPass->Program;
  renWin->GetShaderCache()->ReadyShaderProgram(colorPass);

  colorPass->SetUniformi("texVectors",
    this->Internals->VectorImage->GetTextureUnit());
  colorPass->SetUniformi("texGeomColors",
    this->Internals->GeometryImage->GetTextureUnit());
  colorPass->SetUniformi("texLIC",
    this->Internals->LICImage->GetTextureUnit());
  colorPass->SetUniformi("uScalarColorMode", this->ColorMode);
  colorPass->SetUniformf("uLICIntensity", this->LICIntensity);
  colorPass->SetUniformf("uMapBias", this->MapModeBias);
  colorPass->SetUniformf("uMaskIntensity", this->MaskIntensity);
  float fMaskColor[3];
  fMaskColor[0] = this->MaskColor[0];
  fMaskColor[1] = this->MaskColor[1];
  fMaskColor[2] = this->MaskColor[2];
  colorPass->SetUniform3f("uMaskColor", fMaskColor);

  for (size_t e=0; e<nBlocks; ++e)
  {
    this->Internals->RenderQuad(viewExt, this->Internals->BlockExts[e],
      this->Internals->ColorPass);
  }

  this->Internals->VectorImage->Deactivate();
  this->Internals->GeometryImage->Deactivate();
  this->Internals->LICImage->Deactivate();

  // --------------------------------------------- color contrast enhance
  if ( ( this->EnhanceContrast == ENHANCE_CONTRAST_COLOR )
    || ( this->EnhanceContrast == ENHANCE_CONTRAST_BOTH ) )
  {
    #if vtkSurfaceLICInterfaceDEBUG >= 2
    vtkTextureIO::Write(
          mpifn(comm,"slic_color_rgb_in.vtm"),
          this->Internals->RGBColorImage,
          this->Internals->BlockExts);
    vtkTextureIO::Write(
          mpifn(comm,"slic_color_hsl_in.vtm"),
          this->Internals->HSLColorImage,
          this->Internals->BlockExts);
    #endif

    // find min/max lighness value for color contrast enhancement.
    float LMin = VTK_FLOAT_MAX;
    float LMax = -VTK_FLOAT_MAX;
    float LMaxMinDiff = VTK_FLOAT_MAX;

    vtkSurfaceLICHelper::StreamingFindMinMax(fbo, this->Internals->BlockExts, LMin, LMax);

    if ( this->Internals->BlockExts.size()
      && ((LMax <= LMin) || (LMin < 0.0f) || (LMax > 1.0f)) )
    {
      vtkErrorMacro(
        << comm->GetRank()
        << ": Invalid  range " << LMin << ", " << LMax
        << " for color contrast enhancement");
      LMin = 0.0;
      LMax = 1.0;
      LMaxMinDiff = 1.0;
    }

    // global collective reduction for parallel operation
    this->GetGlobalMinMax(comm, LMin, LMax);

    // set M and m as a fraction of the range.
    LMaxMinDiff = LMax-LMin;
    LMin += LMaxMinDiff*this->LowColorContrastEnhancementFactor;
    LMax -= LMaxMinDiff*this->HighColorContrastEnhancementFactor;
    LMaxMinDiff = LMax-LMin;

    // normalize shader
    fbo->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 0U, this->Internals->RGBColorImage);
    fbo->ActivateDrawBuffer(0U);
    vtkCheckFrameBufferStatusMacro(GL_DRAW_FRAMEBUFFER);

    this->Internals->GeometryImage->Activate();
    this->Internals->HSLColorImage->Activate();
    this->Internals->LICImage->Activate();

    if (!this->Internals->ColorEnhancePass->Program)
    {
      this->InitializeResources();
    }
    vtkShaderProgram *colorEnhancePass =
      this->Internals->ColorEnhancePass->Program;
    renWin->GetShaderCache()->ReadyShaderProgram(colorEnhancePass);
    colorEnhancePass->SetUniformi("texGeomColors",
      this->Internals->GeometryImage->GetTextureUnit());
    colorEnhancePass->SetUniformi("texHSLColors",
      this->Internals->HSLColorImage->GetTextureUnit());
    colorEnhancePass->SetUniformi("texLIC",
      this->Internals->LICImage->GetTextureUnit());
    colorEnhancePass->SetUniformf("uLMin", LMin);
    colorEnhancePass->SetUniformf("uLMaxMinDiff", LMaxMinDiff);

    for (size_t e=0; e<nBlocks; ++e)
    {
      this->Internals->RenderQuad(viewExt, this->Internals->BlockExts[e],
        this->Internals->ColorEnhancePass);
    }

    this->Internals->GeometryImage->Deactivate();
    this->Internals->HSLColorImage->Deactivate();
    this->Internals->LICImage->Deactivate();

    fbo->RemoveTexColorAttachment(GL_DRAW_FRAMEBUFFER, 0U);
    fbo->DeactivateDrawBuffers();
  }
  else
  {
    fbo->RemoveTexColorAttachment(GL_DRAW_FRAMEBUFFER, 0U);
    fbo->RemoveTexColorAttachment(GL_DRAW_FRAMEBUFFER, 1U);
    fbo->DeactivateDrawBuffers();
  }

  fbo->UnBind(GL_FRAMEBUFFER);

  #if vtkSurfaceLICInterfaceDEBUG >= 2
  vtkTextureIO::Write(
         mpifn(comm,"slicp_new_rgb.vtm"),
         this->Internals->RGBColorImage,
         this->Internals->BlockExts);
  #endif
}

void vtkSurfaceLICInterface::CopyToScreen()
{
  vtkOpenGLRenderWindow *renWin = this->Internals->Context;

  vtkPixelExtent viewExt(
      this->Internals->Viewsize[0],
      this->Internals->Viewsize[1]);

  glBindFramebuffer(GL_FRAMEBUFFER, this->PrevFbo);
  glDrawBuffer(this->PrevDrawBuf);
  vtkFrameBufferObject2::InitializeViewport(
        this->Internals->Viewsize[0],
        this->Internals->Viewsize[1]);
  glEnable(GL_DEPTH_TEST);

  this->Internals->DepthImage->Activate();
  this->Internals->RGBColorImage->Activate();

  if (!this->Internals->CopyPass->Program)
  {
    this->InitializeResources();
  }
  vtkShaderProgram *copyPass =
    this->Internals->CopyPass->Program;
  renWin->GetShaderCache()->ReadyShaderProgram(copyPass);
  copyPass->SetUniformi("texDepth",
    this->Internals->DepthImage->GetTextureUnit());
  copyPass->SetUniformi("texRGBColors",
    this->Internals->RGBColorImage->GetTextureUnit());

  size_t nBlocks = this->Internals->BlockExts.size();
  for (size_t e=0; e<nBlocks; ++e)
  {
    this->Internals->RenderQuad(viewExt, this->Internals->BlockExts[e],
      this->Internals->CopyPass);
  }

  this->Internals->DepthImage->Deactivate();
  this->Internals->RGBColorImage->Deactivate();

  #ifdef vtkSurfaceLICMapperTIME
  this->EndTimerEvent("vtkSurfaceLICMapper::DepthCopy");
  #endif

  //
  this->Internals->Updated();
}

//----------------------------------------------------------------------------
void vtkSurfaceLICInterface::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Internals->ReleaseGraphicsResources(win);
  this->Internals->Context = NULL;
}

//----------------------------------------------------------------------------
#define vtkSetMonitoredParameterMacro(_name, _type, _code)  \
void vtkSurfaceLICInterface::Set##_name (_type val)           \
{                                                           \
  if (val == this->_name)                                   \
  {                                                       \
    return;                                                 \
  }                                                       \
  _code                                                     \
  this->_name = val;                                        \
  this->Modified();                                         \
}
// lic
vtkSetMonitoredParameterMacro(
      GenerateNoiseTexture,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;)

vtkSetMonitoredParameterMacro(
      NoiseType,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;)

vtkSetMonitoredParameterMacro(
      NoiseTextureSize,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;)

vtkSetMonitoredParameterMacro(
      NoiseGrainSize,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;)

vtkSetMonitoredParameterMacro(
      MinNoiseValue,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;)

vtkSetMonitoredParameterMacro(
      MaxNoiseValue,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;)

vtkSetMonitoredParameterMacro(
      NumberOfNoiseLevels,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;)

vtkSetMonitoredParameterMacro(
      ImpulseNoiseProbability,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;)

vtkSetMonitoredParameterMacro(
      ImpulseNoiseBackgroundValue,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;)

vtkSetMonitoredParameterMacro(
      NoiseGeneratorSeed,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;)

// compositor
vtkSetMonitoredParameterMacro(
      CompositeStrategy,
      int,)

// lic/compositor
vtkSetMonitoredParameterMacro(
      NumberOfSteps,
      int,)

vtkSetMonitoredParameterMacro(
      StepSize,
      double,)

vtkSetMonitoredParameterMacro(
      NormalizeVectors,
      int,
      val = val < 0 ? 0 : val;
      val = val > 1 ? 1 : val;)

vtkSetMonitoredParameterMacro(
      MaskThreshold,
      double,)

vtkSetMonitoredParameterMacro(
      EnhancedLIC,
      int,)

// lic
vtkSetMonitoredParameterMacro(
      LowLICContrastEnhancementFactor,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;)

vtkSetMonitoredParameterMacro(
      HighLICContrastEnhancementFactor,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;)

vtkSetMonitoredParameterMacro(
      AntiAlias,
      int,
      val = val < 0 ? 0 : val;)

// geometry
vtkSetMonitoredParameterMacro(
      MaskOnSurface,
      int,
      val = val < 0 ? 0 : val;
      val = val > 1 ? 1 : val;)

// colors
vtkSetMonitoredParameterMacro(
      ColorMode,
      int,)

vtkSetMonitoredParameterMacro(
      LICIntensity,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;)

vtkSetMonitoredParameterMacro(
      MaskIntensity,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;)

vtkSetMonitoredParameterMacro(
      MapModeBias,
      double,
      val = val <-1.0 ? -1.0 : val;
      val = val > 1.0 ?  1.0 : val;)

vtkSetMonitoredParameterMacro(
      LowColorContrastEnhancementFactor,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;)

vtkSetMonitoredParameterMacro(
      HighColorContrastEnhancementFactor,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;)

//----------------------------------------------------------------------------
void vtkSurfaceLICInterface::SetMaskColor(double *val)
{
  double rgb[3];
  for (int q=0; q<3; ++q)
  {
    rgb[q] = val[q];
    rgb[q] = rgb[q] < 0.0 ? 0.0 : rgb[q];
    rgb[q] = rgb[q] > 1.0 ? 1.0 : rgb[q];
  }
  if ( (rgb[0] == this->MaskColor[0])
    && (rgb[1] == this->MaskColor[1])
    && (rgb[2] == this->MaskColor[2]) )
  {
    return;
  }
  for (int q=0; q<3; ++q)
  {
    this->MaskColor[q] = rgb[q];
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSurfaceLICInterface::SetEnhanceContrast(int val)
{
  val = val < ENHANCE_CONTRAST_OFF ? ENHANCE_CONTRAST_OFF : val;
  val = val > ENHANCE_CONTRAST_BOTH ? ENHANCE_CONTRAST_BOTH : val;
  if (val == this->EnhanceContrast)
  {
    return;
  }

  this->EnhanceContrast = val;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSurfaceLICInterface::SetNoiseDataSet(vtkImageData *data)
{
  if (data == this->Internals->Noise)
  {
    return;
  }
  this->Internals->Noise = data;
  this->Internals->NoiseImage = NULL;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkImageData *vtkSurfaceLICInterface::GetNoiseDataSet()
{
  if (this->Internals->Noise == NULL)
  {
    vtkImageData *noise = NULL;
    if ( this->GenerateNoiseTexture )
    {
      // report potential issues
      if ( this->NoiseGrainSize >= this->NoiseTextureSize )
      {
        vtkErrorMacro(
          "NoiseGrainSize must be smaller than NoiseTextureSize");
      }
      if ( this->MinNoiseValue >= this->MaxNoiseValue )
      {
        vtkErrorMacro(
          "MinNoiseValue must be smaller than MaxNoiseValue");
      }
      if ( (this->ImpulseNoiseProbability == 1.0)
        && (this->NumberOfNoiseLevels < 2) )
      {
        vtkErrorMacro(
          "NumberOfNoiseLevels must be greater than 1 "
          "when not generating impulse noise");
      }

      // generate a custom noise texture based on the
      // current settings.
      int noiseTextureSize = this->NoiseTextureSize;
      int noiseGrainSize = this->NoiseGrainSize;
      vtkLICRandomNoise2D noiseGen;
      float *noiseValues = noiseGen.Generate(
            this->NoiseType,
            noiseTextureSize,
            noiseGrainSize,
            static_cast<float>(this->MinNoiseValue),
            static_cast<float>(this->MaxNoiseValue),
            this->NumberOfNoiseLevels,
            this->ImpulseNoiseProbability,
            static_cast<float>(this->ImpulseNoiseBackgroundValue),
            this->NoiseGeneratorSeed);
      if ( noiseValues == NULL )
      {
        vtkErrorMacro("Failed to generate noise.");
      }

      vtkFloatArray *noiseArray = vtkFloatArray::New();
      noiseArray->SetNumberOfComponents(2);
      noiseArray->SetName("noise");
      vtkIdType arraySize = 2*noiseTextureSize*noiseTextureSize;
      noiseArray->SetArray(noiseValues, arraySize, 0);

      noise = vtkImageData::New();
      noise->SetSpacing(1.0, 1.0, 1.0);
      noise->SetOrigin(0.0, 0.0, 0.0);
      noise->SetDimensions(noiseTextureSize, noiseTextureSize, 1);
      noise->GetPointData()->SetScalars(noiseArray);

      noiseArray->Delete();
    }
    else
    {
      // load a predefined noise texture.
      noise = vtkLICRandomNoise2D::GetNoiseResource();
    }

    this->Internals->Noise = noise;
    this->Internals->NoiseImage = NULL;
    noise->Delete();
    noise = NULL;
  }

  return this->Internals->Noise;
}

//----------------------------------------------------------------------------
void vtkSurfaceLICInterface::UpdateNoiseImage(vtkRenderWindow *renWin)
{
  vtkOpenGLRenderWindow *rw = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  vtkImageData *noiseDataSet = this->GetNoiseDataSet();

  int ext[6];
  noiseDataSet->GetExtent(ext);
  unsigned int dataWidth = ext[1]-ext[0]+1;
  unsigned int dataHeight = ext[3]-ext[2]+1;

  vtkDataArray *noiseArray = noiseDataSet->GetPointData()->GetScalars();
  int dataType = noiseArray->GetDataType();
  void *data = noiseArray->GetVoidPointer(0);
  int dataComps = noiseArray->GetNumberOfComponents();
  unsigned int dataSize = noiseArray->GetNumberOfTuples()*dataComps;

  vtkPixelBufferObject *pbo = vtkPixelBufferObject::New();
  pbo->SetContext(renWin);
  pbo->Upload1D(dataType, data, dataSize, 1, 0);

  vtkTextureObject *tex = vtkTextureObject::New();
  tex->SetContext(rw);
  tex->SetBaseLevel(0);
  tex->SetMaxLevel(0);
  tex->SetWrapS(vtkTextureObject::Repeat);
  tex->SetWrapT(vtkTextureObject::Repeat);
  tex->SetMinificationFilter(vtkTextureObject::Nearest);
  tex->SetMagnificationFilter(vtkTextureObject::Nearest);
  tex->Create2D(dataWidth, dataHeight, dataComps, pbo, false);
  tex->SetAutoParameters(0);
  pbo->Delete();

  this->Internals->NoiseImage = tex;
  tex->Delete();
}


//----------------------------------------------------------------------------
bool vtkSurfaceLICInterface::IsSupported(vtkRenderWindow *renWin)
{
  vtkOpenGLRenderWindow *context
    = vtkOpenGLRenderWindow::SafeDownCast(renWin);

  return vtkSurfaceLICHelper::IsSupported(context);
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICInterface::CanRenderSurfaceLIC(vtkActor *actor)
{
  // check the render context for GL fetaure support
  // note this also handles non-opengl render window
  if ( this->Internals->ContextNeedsUpdate
    && !vtkSurfaceLICInterface::IsSupported(this->Internals->Context) )
  {
    vtkErrorMacro("SurfaceLIC is not supported");
    return false;
  }

  bool canRender = false;

  int rep = actor->GetProperty()->GetRepresentation();

  if (this->Enable &&
    this->Internals->HasVectors
    && (rep == VTK_SURFACE))
  {
    canRender = true;
  }

  #if vtkSurfaceLICInterfaceDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " CanRender " << canRender << endl;
  #endif

  return canRender;
}

namespace {
  void BuildAShader(vtkOpenGLRenderWindow *renWin,
    vtkOpenGLHelper **cbor, const char * vert,
    const char *frag)
  {
  if (*cbor == NULL)
  {
    *cbor = new vtkOpenGLHelper;
  }
  if (!(*cbor)->Program)
  {
    (*cbor)->Program =
        renWin->GetShaderCache()->ReadyShaderProgram(vert,
                                              frag,
                                              "");
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram((*cbor)->Program);
  }
  }
}

//----------------------------------------------------------------------------
void vtkSurfaceLICInterface::InitializeResources()
{
  bool initialized = true;

  // noise image
  if (!this->Internals->NoiseImage)
  {
    initialized = false;

    this->UpdateNoiseImage(this->Internals->Context);
  }

  // compositer for parallel operation
  if (!this->Internals->Compositor)
  {
    this->Internals->UpdateAll();
    vtkSurfaceLICComposite *compositor = vtkSurfaceLICComposite::New();
    compositor->SetContext(this->Internals->Context);
    this->Internals->Compositor = compositor;
    compositor->Delete();
  }

  // image LIC
  if (!this->Internals->LICer)
  {
    initialized = false;

    vtkLineIntegralConvolution2D *LICer = vtkLineIntegralConvolution2D::New();
    LICer->SetContext(this->Internals->Context);
    this->Internals->LICer = LICer;
    LICer->Delete();
  }

  // frame buffers
  if (!this->Internals->FBO)
  {
    initialized = false;

    vtkFrameBufferObject2 * fbo = vtkFrameBufferObject2::New();
    fbo->SetContext(this->Internals->Context);
    this->Internals->FBO = fbo;
    fbo->Delete();
  }

  // load shader codes
  vtkOpenGLRenderWindow *renWin = this->Internals->Context;

  if (!this->Internals->ColorPass || !this->Internals->ColorPass->Program)
  {
    initialized = false;
    BuildAShader(renWin, &this->Internals->ColorPass,
      vtkTextureObjectVS, vtkSurfaceLICInterface_SC);
  }

  if (!this->Internals->ColorEnhancePass || !this->Internals->ColorEnhancePass->Program)
  {
    initialized = false;
    BuildAShader(renWin, &this->Internals->ColorEnhancePass,
      vtkTextureObjectVS, vtkSurfaceLICInterface_CE);
  }

  if (!this->Internals->CopyPass || !this->Internals->CopyPass->Program)
  {
    initialized = false;
    BuildAShader(renWin, &this->Internals->CopyPass,
      vtkTextureObjectVS, vtkSurfaceLICInterface_DCpy);
  }

  // if any of the above were not already initialized
  // then execute all stages
  if (!initialized)
  {
    this->Internals->UpdateAll();
  }
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICInterface::NeedToUpdateCommunicator()
{
  // no comm or externally modfied paramters
  if ( this->Internals->CommunicatorNeedsUpdate
    || this->Internals->ContextNeedsUpdate
    || !this->Internals->Communicator
    || this->AlwaysUpdate )
  {
    this->Internals->CommunicatorNeedsUpdate = true;
    this->Internals->UpdateAll();
  }

  #if vtkSurfaceLICInterfaceDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " NeedToUpdateCommunicator "
    << this->Internals->CommunicatorNeedsUpdate << endl;
  #endif

  return this->Internals->CommunicatorNeedsUpdate;
}

//----------------------------------------------------------------------------
void vtkSurfaceLICInterface::ValidateContext(vtkRenderer *renderer)
{
  bool modified = false;

  vtkOpenGLRenderWindow *context
    = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());

  // context changed
  if (this->Internals->Context != context)
  {
    modified = true;
    if (this->Internals->Context)
    {
      this->ReleaseGraphicsResources(this->Internals->Context);
    }
    this->Internals->Context = context;
  }

  // viewport size changed
  int viewsize[2];
  renderer->GetTiledSize(&viewsize[0], &viewsize[1]);
  if ( this->Internals->Viewsize[0] != viewsize[0]
    || this->Internals->Viewsize[1] != viewsize[1] )
  {
    modified = true;

    // udpate view size
    this->Internals->Viewsize[0] = viewsize[0];
    this->Internals->Viewsize[1] = viewsize[1];

    // resize textures
    this->Internals->ClearTextures();
    this->Internals->AllocateTextures(context, viewsize);
  }

  // if anything changed execute all stages
  if (modified)
  {
    this->Internals->UpdateAll();
  }

  #if vtkSurfaceLICInterfaceDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " NeedToUpdatContext " << modified << endl;
  #endif
}

void vtkSurfaceLICInterface::SetHasVectors(bool v)
{
  this->Internals->HasVectors = v;
}

bool vtkSurfaceLICInterface::GetHasVectors()
{
  return this->Internals->HasVectors;
}

//----------------------------------------------------------------------------
vtkPainterCommunicator *vtkSurfaceLICInterface::GetCommunicator()
{
  return this->Internals->Communicator;
}

//----------------------------------------------------------------------------
vtkPainterCommunicator *vtkSurfaceLICInterface::CreateCommunicator(int)
{
  return new vtkPainterCommunicator;
}

//----------------------------------------------------------------------------
void vtkSurfaceLICInterface::CreateCommunicator(
  vtkRenderer *ren,
  vtkActor *act,
  vtkDataObject *input)
{
  // compute screen space pixel extent of local blocks and
  // union of local blocks. only blocks that pass view frustum
  // visibility test are used in the computation.
  this->Internals->DataSetExt.Clear();
  this->Internals->BlockExts.clear();

  int includeRank = this->Internals->ProjectBounds(
          ren, act, input,
          this->Internals->Viewsize,
          this->Internals->DataSetExt,
          this->Internals->BlockExts);

  delete this->Internals->Communicator;
  this->Internals->Communicator = this->CreateCommunicator(includeRank);

  #if vtkSurfaceLICInterfaceDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " is rendering " << includeRank << endl;
  #endif
}

//----------------------------------------------------------------------------
void vtkSurfaceLICInterface::SetUpdateAll()
{
  this->Internals->UpdateAll();
}

//----------------------------------------------------------------------------
void vtkSurfaceLICInterface::PrintSelf(ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os
    << indent << "NumberOfSteps=" << this->NumberOfSteps << endl
    << indent << "StepSize=" << this->StepSize << endl
    << indent << "NormalizeVectors=" << this->NormalizeVectors << endl
    << indent << "EnhancedLIC=" << this->EnhancedLIC << endl
    << indent << "EnhanceContrast=" << this->EnhanceContrast << endl
    << indent << "LowLICContrastEnhancementFactor=" << this->LowLICContrastEnhancementFactor << endl
    << indent << "HighLICContrastEnhancementFactor=" << this->HighLICContrastEnhancementFactor << endl
    << indent << "LowColorContrastEnhancementFactor=" << this->LowColorContrastEnhancementFactor << endl
    << indent << "HighColorContrastEnhancementFactor=" << this->HighColorContrastEnhancementFactor << endl
    << indent << "AntiAlias=" << this->AntiAlias << endl
    << indent << "MaskOnSurface=" << this->MaskOnSurface << endl
    << indent << "MaskThreshold=" << this->MaskThreshold << endl
    << indent << "MaskIntensity=" << this->MaskIntensity << endl
    << indent << "MaskColor=" << this->MaskColor[0] << ", " << this->MaskColor[1] << ", " << this->MaskColor[2] << endl
    << indent << "ColorMode=" << this->ColorMode << endl
    << indent << "LICIntensity=" << this->LICIntensity << endl
    << indent << "MapModeBias=" << this->MapModeBias << endl
    << indent << "GenerateNoiseTexture=" << this->GenerateNoiseTexture << endl
    << indent << "NoiseType=" << this->NoiseType << endl
    << indent << "NoiseTextureSize=" << this->NoiseTextureSize << endl
    << indent << "NoiseGrainSize=" << this->NoiseGrainSize << endl
    << indent << "MinNoiseValue=" << this->MinNoiseValue << endl
    << indent << "MaxNoiseValue=" << this->MaxNoiseValue << endl
    << indent << "NumberOfNoiseLevels=" << this->NumberOfNoiseLevels << endl
    << indent << "ImpulseNoiseProbablity=" << this->ImpulseNoiseProbability << endl
    << indent << "ImpulseNoiseBackgroundValue=" << this->ImpulseNoiseBackgroundValue << endl
    << indent << "NoiseGeneratorSeed=" << this->NoiseGeneratorSeed << endl
    << indent << "AlwaysUpdate=" << this->AlwaysUpdate << endl
    << indent << "CompositeStrategy=" << this->CompositeStrategy << endl;
}
