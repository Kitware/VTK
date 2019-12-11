/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLFluidMapper.h"
#include "vtkOpenGLHelper.h"

#include "vtkCommand.h"
#include "vtkExecutive.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPBRPrefilterTexture.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkVolumeProperty.h"

#include "vtkFluidMapperDepthFilterBiGaussFS.h"
#include "vtkFluidMapperDepthFilterNarrowRangeFS.h"
#include "vtkFluidMapperFS.h"
#include "vtkFluidMapperFinalFS.h"
#include "vtkFluidMapperGS.h"
#include "vtkFluidMapperSurfaceNormalFS.h"
#include "vtkFluidMapperThicknessAndVolumeColorFilterFS.h"
#include "vtkFluidMapperVS.h"

#include "vtk_glew.h"

#include <cassert>
#include <sstream>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLFluidMapper);

//-----------------------------------------------------------------------------
vtkOpenGLFluidMapper::vtkOpenGLFluidMapper()
  : VBOs(vtkOpenGLVertexBufferObjectGroup::New())
  , TempMatrix4(vtkMatrix4x4::New())
{
  for (int i = 0; i < NumTexBuffers; ++i)
  {
    this->TexBuffer[i] = vtkTextureObject::New();
  }
  for (int i = 0; i < NumOptionalTexBuffers; ++i)
  {
    this->OptionalTexBuffer[i] = vtkTextureObject::New();
  }
  this->CamDCVC = vtkMatrix4x4::New();
  this->CamInvertedNorms = vtkMatrix3x3::New();
}

//-----------------------------------------------------------------------------
vtkOpenGLFluidMapper::~vtkOpenGLFluidMapper()
{
  this->TempMatrix4->Delete();
  this->VBOs->Delete();
  for (int i = 0; i < NumTexBuffers; ++i)
  {
    this->TexBuffer[i]->Delete();
  }
  for (int i = 0; i < NumOptionalTexBuffers; ++i)
  {
    this->OptionalTexBuffer[i]->Delete();
  }
  this->CamDCVC->Delete();
  this->CamInvertedNorms->Delete();
}

//----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::SetInputData(vtkPolyData* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkPolyData* vtkOpenGLFluidMapper::GetInput()
{
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//-----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Particle radius: " << this->ParticleRadius << "\n";
}

//-----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::UpdateDepthThicknessColorShaders(
  vtkOpenGLHelper& glHelper, vtkRenderer* renderer, vtkVolume* actor)
{
  const auto renderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());

  glHelper.VAO->Bind();

  // Has something changed that would require us to recreate the shader?
  if (!glHelper.Program)
  {
    // Build the shader source code
    std::map<vtkShader::Type, vtkShader*> shaders;

    vtkShader* vertexShader = vtkShader::New();
    vertexShader->SetType(vtkShader::Vertex);
    vertexShader->SetSource(vtkFluidMapperVS);
    shaders[vtkShader::Vertex] = vertexShader;

    vtkShader* geomShader = vtkShader::New();
    geomShader->SetType(vtkShader::Geometry);
    geomShader->SetSource(vtkFluidMapperGS);
    shaders[vtkShader::Geometry] = geomShader;

    vtkShader* fragmentShader = vtkShader::New();
    fragmentShader->SetType(vtkShader::Fragment);
    fragmentShader->SetSource(vtkFluidMapperFS);
    shaders[vtkShader::Fragment] = fragmentShader;

    // Compile and bind the program if needed
    vtkShaderProgram* newProgram = renderWindow->GetShaderCache()->ReadyShaderProgram(shaders);

    // Done with you, now you're thrown away
    fragmentShader->Delete();
    geomShader->Delete();
    vertexShader->Delete();

    // If the shader changed, reinitialize the VAO
    if (newProgram != glHelper.Program)
    {
      glHelper.Program = newProgram;
      // reset the VAO as the shader has changed
      glHelper.VAO->ReleaseGraphicsResources();
    }
    glHelper.ShaderSourceTime.Modified();
  }
  else
  {
    renderWindow->GetShaderCache()->ReadyShaderProgram(glHelper.Program);
  }

  if (glHelper.Program)
  {
    this->SetDepthThicknessColorShaderParameters(glHelper, renderer, actor);

    // Allow the program to set what it wants
    this->InvokeEvent(vtkCommand::UpdateShaderEvent, glHelper.Program);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::SetDepthThicknessColorShaderParameters(
  vtkOpenGLHelper& glHelper, vtkRenderer* ren, vtkVolume* actor)
{
  if (glHelper.IBO->IndexCount &&
    (this->VBOs->GetMTime() > glHelper.AttributeUpdateTime ||
      glHelper.ShaderSourceTime > glHelper.AttributeUpdateTime))
  {
    glHelper.VAO->Bind();
    this->VBOs->AddAllAttributesToVAO(glHelper.Program, glHelper.VAO);
    glHelper.AttributeUpdateTime.Modified();
  }

  const auto program = glHelper.Program;

  program->SetUniformi("outputEyeZ", this->InDepthPass);
  if (!this->InDepthPass)
  {
    // based on clipping range
    program->SetUniformf("minThickness", ren->GetActiveCamera()->GetClippingRange()[1] * 1.0e-9);
  }
  if (this->HasVertexColor)
  {
    program->SetUniformi("hasVertexColor", this->HasVertexColor);
  }

  // Set texture and particle radius
  program->SetUniformi("opaqueZTexture", this->TexBuffer[OpaqueZ]->GetTextureUnit());
  program->SetUniformf("particleRadius", this->ParticleRadius);

  // Set camera
  if (program->IsUniformUsed("VCDCMatrix"))
  {
    program->SetUniformMatrix("VCDCMatrix", this->CamVCDC);
  }

  if (program->IsUniformUsed("MCVCMatrix"))
  {
    if (!actor->GetIsIdentity())
    {
      vtkMatrix4x4* mcwc;
      vtkMatrix3x3* anorms;
      ((vtkOpenGLActor*)actor)->GetKeyMatrices(mcwc, anorms);
      vtkMatrix4x4::Multiply4x4(mcwc, this->CamWCVC, this->TempMatrix4);
      program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
    }
    else
    {
      program->SetUniformMatrix("MCVCMatrix", this->CamWCVC);
    }
  }
  if (program->IsUniformUsed("cameraParallel"))
  {
    glHelper.Program->SetUniformi("cameraParallel", this->CamParallelProjection);
  }
}

void vtkOpenGLFluidMapper::SetupBuffers(vtkOpenGLRenderWindow* const renderWindow)
{
  // create textures we need if not done already
  if (this->TexBuffer[0]->GetHandle() == 0)
  {
    for (int i = 0; i < NumTexBuffers; ++i)
    {
      this->TexBuffer[i]->SetContext(renderWindow);
      switch (i)
      {
        case OpaqueZ:
        case FluidZ:
          this->TexBuffer[i]->AllocateDepth(static_cast<unsigned int>(this->ViewportWidth),
            static_cast<unsigned int>(this->ViewportHeight), vtkTextureObject::Float32);
          break;
        case FluidEyeZ:
        case SmoothedFluidEyeZ:
        case FluidThickness:
        case SmoothedFluidThickness:
          this->TexBuffer[i]->SetInternalFormat(GL_R32F);
          this->TexBuffer[i]->SetFormat(GL_RED);
          this->TexBuffer[i]->Allocate2D(static_cast<unsigned int>(this->ViewportWidth),
            static_cast<unsigned int>(this->ViewportHeight), 1, VTK_FLOAT);
          break;
        case FluidNormal:
          this->TexBuffer[i]->Allocate2D(static_cast<unsigned int>(this->ViewportWidth),
            static_cast<unsigned int>(this->ViewportHeight), 3, VTK_FLOAT);
          break;
        case OpaqueRGBA:
          this->TexBuffer[i]->Allocate2D(static_cast<unsigned int>(this->ViewportWidth),
            static_cast<unsigned int>(this->ViewportHeight), 4, VTK_UNSIGNED_CHAR);
          break;
        default:;
      }

      this->TexBuffer[i]->SetMinificationFilter(vtkTextureObject::Nearest);
      this->TexBuffer[i]->SetMagnificationFilter(vtkTextureObject::Nearest);
      this->TexBuffer[i]->SetWrapS(vtkTextureObject::ClampToEdge);
      this->TexBuffer[i]->SetWrapT(vtkTextureObject::ClampToEdge);
    }
  }
  else
  {
    // make sure we handle size changes
    for (int i = 0; i < NumTexBuffers; ++i)
    {
      this->TexBuffer[i]->Resize(static_cast<unsigned int>(this->ViewportWidth),
        static_cast<unsigned int>(this->ViewportHeight));
    }
  }

  // Allocate additional 2 texture bufferes for color data
  if (this->HasVertexColor)
  {
    if (this->OptionalTexBuffer[0]->GetHandle() == 0)
    {
      for (int i = 0; i < NumOptionalTexBuffers; ++i)
      {
        this->OptionalTexBuffer[i]->SetContext(renderWindow);
        this->OptionalTexBuffer[i]->Allocate2D(static_cast<unsigned int>(this->ViewportWidth),
          static_cast<unsigned int>(this->ViewportHeight), 3, VTK_FLOAT);
        this->OptionalTexBuffer[i]->SetMinificationFilter(vtkTextureObject::Nearest);
        this->OptionalTexBuffer[i]->SetMagnificationFilter(vtkTextureObject::Nearest);
        this->OptionalTexBuffer[i]->SetWrapS(vtkTextureObject::ClampToEdge);
        this->OptionalTexBuffer[i]->SetWrapT(vtkTextureObject::ClampToEdge);
      }
    }
    else
    {
      // make sure we handle size changes
      for (int i = 0; i < NumOptionalTexBuffers; ++i)
      {
        this->OptionalTexBuffer[i]->Resize(static_cast<unsigned int>(this->ViewportWidth),
          static_cast<unsigned int>(this->ViewportHeight));
      }
    }
  }

  // copy the opaque buffers into textures
  this->TexBuffer[OpaqueZ]->CopyFromFrameBuffer(this->ViewportX, this->ViewportY, this->ViewportX,
    this->ViewportY, this->ViewportWidth, this->ViewportHeight);
  this->TexBuffer[OpaqueRGBA]->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
    this->ViewportX, this->ViewportY, this->ViewportWidth, this->ViewportHeight);

  if (!this->FBFluidEyeZ)
  {
    this->FBFluidEyeZ = vtkOpenGLFramebufferObject::New();
    this->FBFluidEyeZ->SetContext(renderWindow);
    this->FBFluidEyeZ->AddDepthAttachment(this->TexBuffer[FluidZ]); // Must have a depth buffer
  }

  if (!this->FBThickness)
  {
    this->FBThickness = vtkOpenGLFramebufferObject::New();
    this->FBThickness->SetContext(renderWindow);
    this->FBThickness->AddDepthAttachment(this->TexBuffer[FluidZ]); // Must have a depth buffer
  }

  if (!this->FBFilterThickness)
  {
    this->FBFilterThickness = vtkOpenGLFramebufferObject::New();
    this->FBFilterThickness->SetContext(renderWindow);
    // Color attachment will be dynamically added later
  }

  if (!this->FBFilterDepth)
  {
    this->FBFilterDepth = vtkOpenGLFramebufferObject::New();
    this->FBFilterDepth->SetContext(renderWindow);
    // Color attachment will be dynamically added later
  }

  if (!this->FBCompNormal)
  {
    this->FBCompNormal = vtkOpenGLFramebufferObject::New();
    this->FBCompNormal->SetContext(renderWindow);
    this->FBCompNormal->AddColorAttachment(0, this->TexBuffer[FluidNormal]);
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::Render(vtkRenderer* renderer, vtkVolume* vol)
{
  // make sure we have data
  vtkPolyData* input = vtkPolyData::SafeDownCast(GetInputDataObject(0, 0));
  if (input == nullptr || input->GetPoints() == nullptr)
  {
    return;
  }

  // check to see if we are using vertex coloring
  int cellFlag = 0;
  vtkDataArray* scalars = this->GetScalars(
    input, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);

  this->HasVertexColor = false;
  if (scalars && cellFlag == 0 && scalars->GetNumberOfComponents() == 3 && this->ScalarVisibility)
  {
    this->HasVertexColor = true;
  }

  // Get the viewport dimensions
  renderer->GetTiledSizeAndOrigin(
    &this->ViewportWidth, &this->ViewportHeight, &this->ViewportX, &this->ViewportY);

  // Get the camera parameters
  const auto cam = static_cast<vtkOpenGLCamera*>(renderer->GetActiveCamera());
  vtkMatrix3x3* tmpNormMat;
  cam->GetKeyMatrices(renderer, this->CamWCVC, tmpNormMat, this->CamVCDC, this->CamWCDC);
  this->CamDCVC->DeepCopy(this->CamVCDC);
  this->CamDCVC->Invert();
  this->CamInvertedNorms->DeepCopy(tmpNormMat);
  this->CamInvertedNorms->Invert();
  this->CamParallelProjection = cam->GetParallelProjection();

  // Prepare the texture and frame buffers
  const auto renderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
  this->SetupBuffers(renderWindow);

  const auto glState = renderWindow->GetState();
  glState->vtkglViewport(0, 0, this->ViewportWidth, this->ViewportHeight);
  bool saveScissorTestState = glState->GetEnumState(GL_SCISSOR_TEST);
#ifdef GL_MULTISAMPLE
  glState->vtkglDisable(GL_MULTISAMPLE);
#endif

  double* crange = cam->GetClippingRange();

  // Generate depth
  {
    // Attach texture every time, since it will be swapped out during smoothing
    this->FBFluidEyeZ->SetContext(renderWindow);
    glState->PushFramebufferBindings();
    this->FBFluidEyeZ->Bind();
    this->FBFluidEyeZ->AddColorAttachment(0U, this->TexBuffer[FluidEyeZ]);
    this->FBFluidEyeZ->ActivateDrawBuffers(1);
    this->FBFluidEyeZ->CheckFrameBufferStatus(GL_FRAMEBUFFER);
    glState->vtkglDisable(GL_SCISSOR_TEST);
    glState->vtkglClearDepth(1.0);
    glState->vtkglColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
    // Set a clear color value to be slightly past the far clipping plane
    glState->vtkglClearColor(-1.1 * crange[1], 0.0, 0.0, 0.0);
    glState->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the spheres to get the eye coordinate z values
    this->TexBuffer[OpaqueZ]->Activate();
    glState->vtkglDepthMask(GL_TRUE);
    glState->vtkglEnable(GL_DEPTH_TEST);
    glState->vtkglDepthFunc(GL_LEQUAL);
    this->InDepthPass = true;
    this->RenderParticles(renderer, vol);
    this->InDepthPass = false;
    this->TexBuffer[OpaqueZ]->Deactivate();
    this->FBFluidEyeZ->DeactivateDrawBuffers();
    this->FBFluidEyeZ->RemoveColorAttachment(0U);
    glState->PopFramebufferBindings();
  }

  // Generate thickness and color (if applicable)
  {
    // Attache texture every time, since it will be swapped out during smoothing
    this->FBThickness->SetContext(renderWindow);
    glState->PushFramebufferBindings();
    this->FBThickness->Bind();
    this->FBThickness->AddColorAttachment(0U, this->TexBuffer[FluidThickness]);
    this->FBThickness->ActivateDrawBuffers(1);
    this->FBThickness->CheckFrameBufferStatus(GL_FRAMEBUFFER);
    if (this->HasVertexColor)
    {
      this->FBThickness->AddColorAttachment(1, this->OptionalTexBuffer[Color]);
      this->FBThickness->ActivateDrawBuffers(2);
      this->FBThickness->CheckFrameBufferStatus(GL_FRAMEBUFFER);
    }
    glState->vtkglDisable(GL_SCISSOR_TEST);
    glState->vtkglClearDepth(1.0);
    glState->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    glState->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
    glState->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vtkOpenGLState::ScopedglBlendFuncSeparate bf(glState);
    glState->vtkglBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

    this->TexBuffer[OpaqueZ]->Activate();
    glState->vtkglDepthMask(GL_FALSE);
    glState->vtkglDisable(GL_DEPTH_TEST);
    glState->vtkglDepthFunc(GL_ALWAYS);
    this->RenderParticles(renderer, vol);
    this->TexBuffer[OpaqueZ]->Deactivate();
    this->FBThickness->DeactivateDrawBuffers();
    if (this->HasVertexColor)
    {
      this->FBThickness->RemoveColorAttachment(1U);
    }
    this->FBThickness->RemoveColorAttachment(0U);
    glState->PopFramebufferBindings();
  }

  // Filter fluid thickness and color (if applicable)
  if (1)
  {
    if (!this->QuadThicknessFilter)
    {
      this->QuadThicknessFilter = new vtkOpenGLQuadHelper(
        renderWindow, nullptr, vtkFluidMapperThicknessAndVolumeColorFilterFS, "");
    }
    else
    {
      renderWindow->GetShaderCache()->ReadyShaderProgram(this->QuadThicknessFilter->Program);
    }
    const auto program = this->QuadThicknessFilter->Program;
    assert(program);

    // Attache texture every time, since it will be swapped out during smoothing
    this->FBFilterThickness->SetContext(renderWindow);
    glState->PushFramebufferBindings();

    for (uint32_t iter = 0; iter < this->ThicknessAndVolumeColorFilterIterations; ++iter)
    {
      this->FBFilterThickness->Bind();
      this->FBFilterThickness->AddColorAttachment(0U, this->TexBuffer[SmoothedFluidThickness]);
      this->FBFilterThickness->ActivateDrawBuffers(1);
      this->FBFilterThickness->CheckFrameBufferStatus(GL_FRAMEBUFFER);
      glState->vtkglClearDepth(1.0);
      glState->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
      glState->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
      glState->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      if (this->HasVertexColor)
      {
        this->FBFilterThickness->AddColorAttachment(1, this->OptionalTexBuffer[SmoothedColor]);
        this->FBFilterThickness->ActivateDrawBuffers(2);
        this->OptionalTexBuffer[Color]->Activate();
        program->SetUniformi("hasVertexColor", this->HasVertexColor);
        program->SetUniformi("fluidColorTexture", this->OptionalTexBuffer[Color]->GetTextureUnit());
      }

      this->TexBuffer[FluidThickness]->Activate();
      program->SetUniformi(
        "fluidThicknessTexture", this->TexBuffer[FluidThickness]->GetTextureUnit());

      program->SetUniformi("viewportHeight", this->ViewportHeight);
      program->SetUniformi("viewportWidth", this->ViewportWidth);
      program->SetUniformi(
        "filterRadius", static_cast<int>(this->ThicknessAndVolumeColorFilterRadius));

      this->QuadThicknessFilter->Render();
      this->TexBuffer[FluidThickness]->Deactivate();
      this->FBFilterThickness->DeactivateDrawBuffers();
      this->FBFilterThickness->RemoveColorAttachment(0U);

      std::swap(this->TexBuffer[FluidThickness], this->TexBuffer[SmoothedFluidThickness]);
      if (this->HasVertexColor)
      {
        this->OptionalTexBuffer[Color]->Deactivate();
        std::swap(this->OptionalTexBuffer[Color], this->OptionalTexBuffer[SmoothedColor]);
      }
    }
    glState->PopFramebufferBindings();
  }

  if (1)
  {
    // Filter depth surface
    if (DisplayMode != UnfilteredOpaqueSurface && DisplayMode != UnfilteredSurfaceNormal)
    {
      if (!this->QuadFluidDepthFilter[SurfaceFilterMethod])
      {
        switch (this->SurfaceFilterMethod)
        {
          case BilateralGaussian:
            this->QuadFluidDepthFilter[SurfaceFilterMethod] = new vtkOpenGLQuadHelper(
              renderWindow, nullptr, vtkFluidMapperDepthFilterBiGaussFS, "");
            break;
          case NarrowRange:
            this->QuadFluidDepthFilter[SurfaceFilterMethod] = new vtkOpenGLQuadHelper(
              renderWindow, nullptr, vtkFluidMapperDepthFilterNarrowRangeFS, "");
            break;
          // New filter method is added here
          default:
            vtkErrorMacro("Invalid filter method");
        }
      }
      else
      {
        renderWindow->GetShaderCache()->ReadyShaderProgram(
          this->QuadFluidDepthFilter[SurfaceFilterMethod]->Program);
      }

      const auto program = this->QuadFluidDepthFilter[SurfaceFilterMethod]->Program;
      assert(program);
      this->FBFilterDepth->SetContext(renderWindow);
      glState->PushFramebufferBindings();

      program->SetUniformi("viewportHeight", this->ViewportHeight);
      program->SetUniformi("viewportWidth", this->ViewportWidth);
      program->SetUniformi("filterRadius", static_cast<int>(this->SurfaceFilterRadius));
      program->SetUniformf("particleRadius", this->ParticleRadius);
      program->SetUniformf("farZValue", -crange[1]);

      for (uint32_t iter = 0; iter < this->SurfaceFilterIterations; ++iter)
      {
        this->FBFilterDepth->Bind();
        this->FBFilterDepth->AddColorAttachment(
          0U, this->TexBuffer[SmoothedFluidEyeZ]); // Replace color attachement
        this->FBFilterDepth->ActivateDrawBuffers(1);
        this->FBFilterDepth->CheckFrameBufferStatus(GL_FRAMEBUFFER);
        glState->vtkglClearDepth(1.0);
        glState->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
        glState->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
        glState->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        switch (SurfaceFilterMethod)
        {
          case BilateralGaussian:
            program->SetUniformf("sigmaDepth", this->BiGaussFilterSigmaDepth);
            break;
          case NarrowRange:
            program->SetUniformf("lambda", this->NRFilterLambda);
            program->SetUniformf("mu", this->NRFilterMu);
            break;
          // New filter method is added here
          default:
            vtkErrorMacro("Invalid filter method");
        }

        glState->vtkglEnable(GL_DEPTH_TEST);
        this->TexBuffer[FluidEyeZ]->Activate();
        program->SetUniformi("fluidZTexture", this->TexBuffer[FluidEyeZ]->GetTextureUnit());

        this->QuadFluidDepthFilter[SurfaceFilterMethod]->Render();
        this->TexBuffer[FluidEyeZ]->Deactivate();
        this->FBFilterDepth->DeactivateDrawBuffers();
        this->FBFilterDepth->RemoveColorAttachment(0);

        // Swap the filtered buffers
        std::swap(this->TexBuffer[FluidEyeZ], this->TexBuffer[SmoothedFluidEyeZ]);
      }

      glState->PopFramebufferBindings();
    }
  }

  // Compute normal for the filtered depth surface
  if (1)
  {
    if (!this->QuadFluidNormal)
    {
      this->QuadFluidNormal =
        new vtkOpenGLQuadHelper(renderWindow, nullptr, vtkFluidMapperSurfaceNormalFS, "");
    }
    else
    {
      renderWindow->GetShaderCache()->ReadyShaderProgram(this->QuadFluidNormal->Program);
    }

    const auto program = this->QuadFluidNormal->Program;
    assert(program);

    this->FBCompNormal->SetContext(renderWindow);
    glState->PushFramebufferBindings();
    this->FBCompNormal->Bind();
    this->FBCompNormal->AddColorAttachment(0, this->TexBuffer[FluidNormal]);
    this->FBCompNormal->ActivateDrawBuffers(1);
    this->FBCompNormal->CheckFrameBufferStatus(GL_FRAMEBUFFER);

    this->TexBuffer[FluidEyeZ]->Activate();
    program->SetUniformi("fluidZTexture", this->TexBuffer[FluidEyeZ]->GetTextureUnit());

    program->SetUniformi("viewportHeight", this->ViewportHeight);
    program->SetUniformi("viewportWidth", this->ViewportWidth);
    program->SetUniformMatrix("DCVCMatrix", this->CamDCVC);
    program->SetUniformMatrix("VCDCMatrix", this->CamVCDC);

    glState->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    glState->vtkglDepthMask(GL_FALSE);
    glState->vtkglDisable(GL_DEPTH_TEST);
    glState->vtkglDepthFunc(GL_ALWAYS);
    glState->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
    glState->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    this->QuadFluidNormal->Render();
    this->TexBuffer[FluidEyeZ]->Deactivate();
    this->FBCompNormal->DeactivateDrawBuffers();
    glState->PopFramebufferBindings();
  }

  vtkOpenGLRenderer* oren = static_cast<vtkOpenGLRenderer*>(renderer);

  // Restore the original viewport properties
  glState->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glState->vtkglViewport(
    this->ViewportX, this->ViewportY, this->ViewportWidth, this->ViewportHeight);
  saveScissorTestState ? glState->vtkglEnable(GL_SCISSOR_TEST)
                       : glState->vtkglDisable(GL_SCISSOR_TEST);

  {
    bool useIBL = oren->GetUseImageBasedLighting() && oren->GetEnvironmentCubeMap();

    // Final blend, render everything
    if (!this->QuadFinalBlend)
    {
      std::ostringstream toString;

      // todo this needs to be done when the lighting code changes
      // if the light complexity changed then update the shader code
      std::string fssource = vtkFluidMapperFinalFS;
      vtkShaderProgram::Substitute(fssource, "//VTK::Light::Dec", oren->GetLightingUniforms());
      switch (oren->GetLightingComplexity())
      {
        // no lighting
        case 0:
          vtkShaderProgram::Substitute(fssource, "//VTK::Light::Impl",
            "  accumulatedLightSpecularColor = vec3(1.0,1.0,1.0);", false);
          break;

        // headlight
        case 1:
          vtkShaderProgram::Substitute(fssource, "//VTK::Light::Impl",
            "  float df = max(0.0,N.z);\n"
            "  float sf = pow(df, fluidShininess);\n"
            "  accumulatedLightDiffuseColor = df * lightColor0;\n"
            "  accumulatedLightSpecularColor = sf * lightColor0;\n"
            "  //VTK::Light::Impl\n",
            false);
          break;
        case 2:
          toString << "  float df;\n"
                      "  float sf;\n";
          for (int i = 0; i < oren->GetLightingCount(); ++i)
          {
            toString << "  df = max(0.0, dot(N, -lightDirectionVC" << i
                     << "));\n"
                        "  accumulatedLightDiffuseColor += (df * lightColor"
                     << i << ");\n"
                     << "  sf = sign(df)*pow(max(0.0, dot( reflect(lightDirectionVC" << i
                     << "     , N), normalize(-position))), fluidShininess);\n"
                        "  accumulatedLightSpecularColor += (sf * lightColor"
                     << i << ");\n";
          }
          vtkShaderProgram::Substitute(fssource, "//VTK::Light::Impl", toString.str(), false);
          break;
        case 3:
          toString << "  vec3 vertLightDirectionVC;\n"
                      "  float attenuation;\n"
                      "  float df;\n"
                      "  float sf;\n";
          for (int i = 0; i < oren->GetLightingCount(); ++i)
          {
            toString << "    attenuation = 1.0;\n"
                        "    if (lightPositional"
                     << i
                     << " == 0) {\n"
                        "      vertLightDirectionVC = lightDirectionVC"
                     << i
                     << "; }\n"
                        "    else {\n"
                        "      vertLightDirectionVC = position - lightPositionVC"
                     << i
                     << ";\n"
                        "      float distanceVC = length(vertLightDirectionVC);\n"
                        "      vertLightDirectionVC = "
                        "normalize(vertLightDirectionVC);\n"
                        "      attenuation = 1.0 /\n"
                        "        (lightAttenuation"
                     << i
                     << ".x\n"
                        "         + lightAttenuation"
                     << i
                     << ".y * distanceVC\n"
                        "         + lightAttenuation"
                     << i
                     << ".z * distanceVC * distanceVC);\n"
                        "      // per OpenGL standard cone angle is 90 or less for a "
                        "spot light\n"
                        "      if (lightConeAngle"
                     << i
                     << " <= 90.0) {\n"
                        "        float coneDot = dot(vertLightDirectionVC, "
                        "lightDirectionVC"
                     << i
                     << ");\n"
                        "        // if inside the cone\n"
                        "        if (coneDot >= cos(radians(lightConeAngle"
                     << i
                     << "))) {\n"
                        "          attenuation = attenuation * pow(coneDot, "
                        "lightExponent"
                     << i
                     << "); }\n"
                        "        else {\n"
                        "          attenuation = 0.0; }\n"
                        "        }\n"
                        "      }\n"
                     << "    df = max(0.0,attenuation*dot(N, "
                        "-vertLightDirectionVC));\n"
                        "    accumulatedLightDiffuseColor += (df * lightColor"
                     << i << ");\n"
                     << "    sf = sign(df)*attenuation*pow( max(0.0, dot( "
                        "reflect(vertLightDirectionVC, N), normalize(-position))), "
                        "fluidShininess);\n"
                        "    accumulatedLightSpecularColor += (sf * lightColor"
                     << i << ");\n";
          }

          vtkShaderProgram::Substitute(fssource, "//VTK::Light::Impl", toString.str(), false);
          break;
      }

      if (useIBL)
      {
        vtkShaderProgram::Substitute(fssource, "//VTK::UseIBL::Dec", "#define UseIBL", false);
      }

      this->QuadFinalBlend = new vtkOpenGLQuadHelper(renderWindow, nullptr, fssource.c_str(), "");
    }
    else
    {
      renderWindow->GetShaderCache()->ReadyShaderProgram(this->QuadFinalBlend->Program);
    }

    const auto program = this->QuadFinalBlend->Program;
    assert(program);

    oren->UpdateLightingUniforms(program);

    // Add IBL textures
    if (useIBL)
    {
      program->SetUniformi("prefilterTex", oren->GetEnvMapPrefiltered()->GetTextureUnit());
      program->SetUniformMatrix("invNormalMatrix", this->CamInvertedNorms);
    }

    this->TexBuffer[FluidEyeZ]->Activate();
    program->SetUniformi("fluidZTexture", this->TexBuffer[FluidEyeZ]->GetTextureUnit());

    this->TexBuffer[FluidThickness]->Activate();
    program->SetUniformi(
      "fluidThicknessTexture", this->TexBuffer[FluidThickness]->GetTextureUnit());

    this->TexBuffer[FluidNormal]->Activate();
    program->SetUniformi("fluidNormalTexture", this->TexBuffer[FluidNormal]->GetTextureUnit());

    this->TexBuffer[OpaqueRGBA]->Activate();
    program->SetUniformi("opaqueRGBATexture", this->TexBuffer[OpaqueRGBA]->GetTextureUnit());

    if (this->HasVertexColor)
    {
      this->OptionalTexBuffer[Color]->Activate();
      program->SetUniformi("fluidColorTexture", this->OptionalTexBuffer[Color]->GetTextureUnit());
      program->SetUniformi("hasVertexColor", this->HasVertexColor);
      program->SetUniformf("vertexColorPower", this->ParticleColorPower);
      program->SetUniformf("vertexColorScale", this->ParticleColorScale);
    }

    program->SetUniformMatrix("DCVCMatrix", this->CamDCVC);
    program->SetUniformMatrix("VCDCMatrix", this->CamVCDC);
    if (this->QuadFinalBlend->Program->IsUniformUsed("MCVCMatrix"))
    {
      if (!vol->GetIsIdentity())
      {
        vtkMatrix4x4* mcwc;
        vtkMatrix3x3* anorms;
        ((vtkOpenGLActor*)vol)->GetKeyMatrices(mcwc, anorms);
        vtkMatrix4x4::Multiply4x4(mcwc, this->CamWCVC, this->TempMatrix4);
        this->QuadFinalBlend->Program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
      }
      else
      {
        this->QuadFinalBlend->Program->SetUniformMatrix("MCVCMatrix", this->CamWCVC);
      }
    }

    program->SetUniformi("displayModeOpaqueSurface",
      this->DisplayMode == UnfilteredOpaqueSurface || this->DisplayMode == FilteredOpaqueSurface);
    program->SetUniformi("displayModeSurfaceNormal",
      this->DisplayMode == UnfilteredSurfaceNormal || this->DisplayMode == FilteredSurfaceNormal);
    program->SetUniformf("attenuationScale", this->AttenuationScale);
    program->SetUniformf("additionalReflection", this->AdditionalReflection);
    program->SetUniformf("refractiveIndex", this->RefractiveIndex);
    program->SetUniformf("refractionScale", this->RefractionScale);
    program->SetUniform3f("fluidOpaqueColor", this->OpaqueColor);
    program->SetUniform3f("fluidAttenuationColor", this->AttenuationColor);
    program->SetUniformf("farZValue", -crange[1]);
    program->SetUniformf("ambientValue", vol->GetProperty()->GetAmbient());
    glState->vtkglEnable(GL_DEPTH_TEST);
    glState->vtkglDepthMask(GL_TRUE);
    glState->vtkglDepthFunc(GL_ALWAYS);

    this->QuadFinalBlend->Render();

    this->TexBuffer[OpaqueZ]->Deactivate();
    this->TexBuffer[OpaqueRGBA]->Deactivate();
    this->TexBuffer[FluidEyeZ]->Deactivate();
    this->TexBuffer[FluidThickness]->Deactivate();
    this->TexBuffer[FluidNormal]->Deactivate();
    if (this->HasVertexColor)
    {
      this->OptionalTexBuffer[Color]->Deactivate();
    }

    glState->vtkglDepthFunc(GL_LEQUAL);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::RenderParticles(vtkRenderer* renderer, vtkVolume* vol)
{
  vtkPolyData* input = vtkPolyData::SafeDownCast(GetInputDataObject(0, 0));
  if (input == nullptr || input->GetPoints() == nullptr)
  {
    return;
  }

  if (this->VBOBuildTime < input->GetPoints()->GetMTime())
  {
    this->VBOs->CacheDataArray("vertexMC", input->GetPoints()->GetData(), renderer, VTK_FLOAT);

    if (this->HasVertexColor)
    {
      int cellFlag = 0;
      vtkDataArray* scalars = this->GetScalars(
        input, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);
      this->VBOs->CacheDataArray("vertexColor", scalars, renderer, VTK_FLOAT);
    }
    this->VBOs->BuildAllVBOs(renderer);

    vtkIdType numPts = input->GetPoints()->GetNumberOfPoints();
    this->GLHelperDepthThickness.IBO->IndexCount = static_cast<size_t>(numPts);
    this->VBOBuildTime.Modified();
  }

  // draw polygons
  int numVerts = this->VBOs->GetNumberOfTuples("vertexMC");
  if (numVerts)
  {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateDepthThicknessColorShaders(this->GLHelperDepthThickness, renderer, vol);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(numVerts));
  }
}

//-----------------------------------------------------------------------------
// Description:
// Destructor. Delete SourceCode if any.
void vtkOpenGLFluidMapper::ReleaseGraphicsResources(vtkWindow* w)
{
  if (this->FBFluidEyeZ != nullptr)
  {
    this->FBFluidEyeZ->ReleaseGraphicsResources(w);
    this->FBFluidEyeZ->UnRegister(this);
    this->FBFluidEyeZ = nullptr;
  }
  if (this->FBThickness != nullptr)
  {
    this->FBThickness->ReleaseGraphicsResources(w);
    this->FBThickness->UnRegister(this);
    this->FBThickness = nullptr;
  }
  if (this->FBFilterThickness != nullptr)
  {
    this->FBFilterThickness->ReleaseGraphicsResources(w);
    this->FBFilterThickness->UnRegister(this);
    this->FBFilterThickness = nullptr;
  }
  if (this->FBCompNormal != nullptr)
  {
    this->FBCompNormal->ReleaseGraphicsResources(w);
    this->FBCompNormal->UnRegister(this);
    this->FBCompNormal = nullptr;
  }
  if (this->FBFilterDepth != nullptr)
  {
    this->FBFilterDepth->ReleaseGraphicsResources(w);
    this->FBFilterDepth->UnRegister(this);
    this->FBFilterDepth = nullptr;
  }

  if (this->QuadThicknessFilter != nullptr)
  {
    delete this->QuadThicknessFilter;
    this->QuadThicknessFilter = nullptr;
  }
  if (this->QuadFluidNormal != nullptr)
  {
    delete this->QuadFluidNormal;
    this->QuadFluidNormal = nullptr;
  }
  if (this->QuadFinalBlend != nullptr)
  {
    delete this->QuadFinalBlend;
    this->QuadFinalBlend = nullptr;
  }
  for (int i = 0; i < this->NumFilterMethods; ++i)
  {
    if (this->QuadFluidDepthFilter[i] != nullptr)
    {
      delete this->QuadFluidDepthFilter[i];
      this->QuadFluidDepthFilter[i] = nullptr;
    }
  }

  this->VBOs->ReleaseGraphicsResources(w);

  for (int i = 0; i < NumTexBuffers; ++i)
  {
    this->TexBuffer[i]->ReleaseGraphicsResources(w);
  }
  for (int i = 0; i < NumOptionalTexBuffers; ++i)
  {
    this->OptionalTexBuffer[i]->ReleaseGraphicsResources(w);
  }

  this->GLHelperDepthThickness.ReleaseGraphicsResources(w);

  this->Modified();
}
