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
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include "vtkFluidMapperVS.h"
#include "vtkFluidMapperGS.h"
#include "vtkFluidMapperFS.h"
#include "vtkFluidMapperFinalFS.h"

#include "vtk_glew.h"


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLFluidMapper)

//-----------------------------------------------------------------------------
vtkOpenGLFluidMapper::vtkOpenGLFluidMapper() :
  Framebuffer(nullptr)
{
  this->Radius = 0.3;
  this->VBOs = vtkOpenGLVertexBufferObjectGroup::New();

  this->IntermediateBlend = nullptr;
  this->FinalBlend = nullptr;

  for (int i = 0; i <= FluidThickness; ++i)
  {
    this->Textures[i] = vtkTextureObject::New();
  }
  this->TempMatrix4 = vtkMatrix4x4::New();
}

//-----------------------------------------------------------------------------
vtkOpenGLFluidMapper::~vtkOpenGLFluidMapper()
{
  this->TempMatrix4->Delete();
  this->VBOs->Delete();
}

//----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::SetInputData(vtkPolyData *input)
{
  this->SetInputDataInternal(0, input);
}

//-----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::UpdateShaders(
  vtkOpenGLHelper &cellBO, vtkRenderer* ren, vtkVolume *actor)
{
  vtkOpenGLRenderWindow *renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  cellBO.VAO->Bind();

  // has something changed that would require us to recreate the shader?
  if (!cellBO.Program)
  {
    // build the shader source code
    std::map<vtkShader::Type,vtkShader *> shaders;
    vtkShader *vss = vtkShader::New();
    vss->SetType(vtkShader::Vertex);
    shaders[vtkShader::Vertex] = vss;
    vtkShader *gss = vtkShader::New();
    gss->SetType(vtkShader::Geometry);
    shaders[vtkShader::Geometry] = gss;
    vtkShader *fss = vtkShader::New();
    fss->SetType(vtkShader::Fragment);
    shaders[vtkShader::Fragment] = fss;

    shaders[vtkShader::Vertex]->SetSource(vtkFluidMapperVS);
    shaders[vtkShader::Geometry]->SetSource(vtkFluidMapperGS);
    shaders[vtkShader::Fragment]->SetSource(vtkFluidMapperFS);

    // compile and bind the program if needed
    vtkShaderProgram *newShader =
      renWin->GetShaderCache()->ReadyShaderProgram(shaders);

    vss->Delete();
    fss->Delete();
    gss->Delete();

    // if the shader changed reinitialize the VAO
    if (newShader != cellBO.Program)
    {
      cellBO.Program = newShader;
      // reset the VAO as the shader has changed
      cellBO.VAO->ReleaseGraphicsResources();
    }

    cellBO.ShaderSourceTime.Modified();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(cellBO.Program);
  }

  if (cellBO.Program)
  {
    this->SetMapperShaderParameters(cellBO, ren, actor);
    this->SetCameraShaderParameters(cellBO, ren, actor);

    // allow the program to set what it wants
    this->InvokeEvent(vtkCommand::UpdateShaderEvent, cellBO.Program);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::SetCameraShaderParameters(
  vtkOpenGLHelper &cellBO,
  vtkRenderer* ren, vtkVolume *actor)
{
  vtkShaderProgram *program = cellBO.Program;

  vtkOpenGLCamera *cam = (vtkOpenGLCamera *)(ren->GetActiveCamera());

  vtkMatrix4x4 *wcdc;
  vtkMatrix4x4 *wcvc;
  vtkMatrix3x3 *norms;
  vtkMatrix4x4 *vcdc;
  cam->GetKeyMatrices(ren,wcvc,norms,vcdc,wcdc);
  if (program->IsUniformUsed("VCDCMatrix"))
  {
    program->SetUniformMatrix("VCDCMatrix", vcdc);
  }

  if (program->IsUniformUsed("MCVCMatrix"))
  {
    if (!actor->GetIsIdentity())
    {
      vtkMatrix4x4 *mcwc;
      vtkMatrix3x3 *anorms;
      ((vtkOpenGLActor *)actor)->GetKeyMatrices(mcwc,anorms);
      vtkMatrix4x4::Multiply4x4(mcwc, wcvc, this->TempMatrix4);
      program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
    }
    else
    {
      program->SetUniformMatrix("MCVCMatrix", wcvc);
    }
  }

  if (program->IsUniformUsed("cameraParallel"))
  {
    cellBO.Program->SetUniformi("cameraParallel", cam->GetParallelProjection());
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::SetMapperShaderParameters(
  vtkOpenGLHelper &cellBO,
  vtkRenderer *ren, vtkVolume *actor)
{
  if (cellBO.IBO->IndexCount &&
      (this->VBOs->GetMTime() > cellBO.AttributeUpdateTime ||
       cellBO.ShaderSourceTime > cellBO.AttributeUpdateTime))
  {
    cellBO.VAO->Bind();

    this->VBOs->AddAllAttributesToVAO(cellBO.Program, cellBO.VAO);

    cellBO.AttributeUpdateTime.Modified();
  }

  cellBO.Program->SetUniformi(
      "opaqueZTexture", this->Textures[OpaqueZ]->GetTextureUnit());
  cellBO.Program->SetUniformf("radius", this->Radius);
}

//-----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Radius: " << this->Radius << "\n";
}

//----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::Render(vtkRenderer *ren, vtkVolume *act)
{
  vtkOpenGLRenderWindow *renWin
    = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  auto state = renWin->GetState();

  // get the viewport dimensions
  ren->GetTiledSizeAndOrigin(
    &this->ViewportWidth, &this->ViewportHeight,
    &this->ViewportX, &this->ViewportY);

  // create textures we need if not done already
  if (this->Textures[0]->GetHandle() == 0)
  {
    for (int i = 0; i <= FluidThickness; ++i)
    {
      this->Textures[i]->SetContext(renWin);
      switch (i)
      {
        case FluidZ:
        case OpaqueZ:
          this->Textures[i]->AllocateDepth(
            this->ViewportWidth, this->ViewportHeight, vtkTextureObject::Float32);
          break;
        case OpaqueRGBA:
          this->Textures[i]->Allocate2D(
            this->ViewportWidth, this->ViewportHeight, 4, VTK_UNSIGNED_CHAR);
          break;
        case FluidThickness:
          this->Textures[i]->Allocate2D(
            this->ViewportWidth, this->ViewportHeight, 1, VTK_FLOAT);
          break;
      }
      this->Textures[i]->SetMinificationFilter(vtkTextureObject::Nearest);
      this->Textures[i]->SetMagnificationFilter(vtkTextureObject::Nearest);
      this->Textures[i]->SetWrapS(vtkTextureObject::ClampToEdge);
      this->Textures[i]->SetWrapT(vtkTextureObject::ClampToEdge);
    }
  }
  else
  {
    // make sure we handle size changes
    for (int i = 0; i <= FluidThickness; ++i)
    {
      this->Textures[i]->Resize(
        this->ViewportWidth, this->ViewportHeight);
    }
  }

  // copy the opaque buffers into textures
  this->Textures[OpaqueZ]->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
    this->ViewportX, this->ViewportY,
    this->ViewportWidth, this->ViewportHeight);
  this->Textures[OpaqueRGBA]->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
    this->ViewportX, this->ViewportY,
    this->ViewportWidth, this->ViewportHeight);

  if (!this->Framebuffer)
  {
    this->Framebuffer = vtkOpenGLFramebufferObject::New();
    this->Framebuffer->SetContext(renWin);
  }
  this->Framebuffer->SaveCurrentBindingsAndBuffers();
  this->Framebuffer->Bind();
  this->Framebuffer->AddDepthAttachment(this->Textures[FluidZ]);
  this->Framebuffer->AddColorAttachment(0, this->Textures[FluidThickness]);

  state->vtkglViewport(0, 0,
             this->ViewportWidth, this->ViewportHeight);
  bool saveScissorTestState = state->GetEnumState(GL_SCISSOR_TEST);
  state->vtkglDisable(GL_SCISSOR_TEST);

  state->vtkglClearDepth(static_cast<GLclampf>(1.0));
  state->vtkglColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
  state->vtkglClearColor(0.0,0.0,0.0,0.0); // always clear to black
  state->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef GL_MULTISAMPLE
  bool multiSampleStatus = state->GetEnumState(GL_MULTISAMPLE);
  state->vtkglDisable(GL_MULTISAMPLE);
#endif

  {
    vtkOpenGLState::ScopedglBlendFuncSeparate bf(state);
    state->vtkglBlendFuncSeparate(
      GL_ONE, GL_ONE,
      GL_ONE, GL_ONE
      );

    // render the spheres to get the z values
    this->Textures[OpaqueZ]->Activate();
    state->vtkglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    state->vtkglDepthMask(GL_TRUE);
    state->vtkglEnable(GL_DEPTH_TEST);
    state->vtkglDepthFunc( GL_LEQUAL );
    this->RenderVolume(ren,act);

    // render again to get the thickness
    state->vtkglColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
    state->vtkglDepthMask(GL_FALSE);
    state->vtkglDisable(GL_DEPTH_TEST);
    state->vtkglDepthFunc( GL_ALWAYS );
    this->RenderVolume(ren,act);
  }

  // smooth the two textures here
  // when you get to this part give me a holler and
  // I'll throiw some code your way once I know how
  // you are smoothing.

  state->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  // now do a full screen quad to render the results back onto
  // the buffers that were active when we were called
  this->Framebuffer->RestorePreviousBindingsAndBuffers();

  // Restore the original viewport and scissor test settings
  state->vtkglViewport(this->ViewportX, this->ViewportY,
             this->ViewportWidth, this->ViewportHeight);
  if (saveScissorTestState)
  {
    state->vtkglEnable(GL_SCISSOR_TEST);
  }
  else
  {
    state->vtkglDisable(GL_SCISSOR_TEST);
  }

  if (!this->FinalBlend)
  {
    this->FinalBlend = new vtkOpenGLQuadHelper(renWin,
      nullptr,
      vtkFluidMapperFinalFS,
      "");
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(
      this->FinalBlend->Program);
  }

  if (this->FinalBlend->Program)
  {
    this->Textures[FluidZ]->Activate();
    this->FinalBlend->Program->SetUniformi(
      "fluidZTexture", this->Textures[FluidZ]->GetTextureUnit());

    this->Textures[FluidThickness]->Activate();
    this->FinalBlend->Program->SetUniformi(
      "fluidThicknessTexture",
      this->Textures[FluidThickness]->GetTextureUnit());

    this->Textures[OpaqueRGBA]->Activate();
    this->FinalBlend->Program->SetUniformi(
      "opaqueRGBATexture",
      this->Textures[OpaqueRGBA]->GetTextureUnit());

    // already active from up above
    // this->Textures[OpaqueZ]->Activate();
    this->FinalBlend->Program->SetUniformi(
      "opaqueZTexture",
      this->Textures[OpaqueZ]->GetTextureUnit());

    // blend in OpaqueRGBA
    state->vtkglEnable(GL_DEPTH_TEST);
    state->vtkglDepthMask(GL_TRUE);
    state->vtkglDepthFunc( GL_ALWAYS );

    // do we need to set the viewport
    this->FinalBlend->Render();

    this->Textures[OpaqueZ]->Deactivate();
    this->Textures[OpaqueRGBA]->Deactivate();
    this->Textures[FluidZ]->Deactivate();
    this->Textures[FluidThickness]->Deactivate();
  }

  state->vtkglDepthFunc( GL_LEQUAL );
}

//-----------------------------------------------------------------------------
void vtkOpenGLFluidMapper::RenderVolume(vtkRenderer* ren, vtkVolume *vol)
{
  vtkPolyData *poly = vtkPolyData::SafeDownCast(
    this->GetInputDataObject(0, 0));

  if (poly == nullptr || poly->GetPoints() == nullptr)
  {
    return;
  }

  vtkIdType numPts = poly->GetPoints()->GetNumberOfPoints();

  if (this->VBOBuildTime < poly->GetPoints()->GetMTime())
  {
    // Iterate through all of the different types in the polydata, building OpenGLs
    // and IBOs as appropriate for each type.
    this->VBOs->CacheDataArray("vertexMC",
      poly->GetPoints()->GetData(), ren, VTK_FLOAT);
    this->VBOs->BuildAllVBOs(ren);
    this->CellBO.IBO->IndexCount = numPts;
    this->VBOBuildTime.Modified();
  }

  // draw polygons
  int numVerts = this->VBOs->GetNumberOfTuples("vertexMC");
  if (numVerts)
  {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShaders(this->CellBO, ren, vol);
    glDrawArrays(GL_POINTS, 0,
                static_cast<GLuint>(numVerts));
  }
}
