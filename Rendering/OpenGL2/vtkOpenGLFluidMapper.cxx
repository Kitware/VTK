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
#include "vtkOpenGLTexture.h"
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
#include "vtkFluidMapperThicknessFS.h"
#include "vtkFluidMapperThicknessAndVolumeColorFilterFS.h"
#include "vtkFluidMapperDepthFilterBiGaussFS.h"
#include "vtkFluidMapperDepthFilterNarrowRangeFS.h"
#include "vtkFluidMapperSurfaceNormalFS.h"
#include "vtkFluidMapperFinalFS.h"

#include "vtk_glew.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLFluidMapper)

//-----------------------------------------------------------------------------
vtkOpenGLFluidMapper::vtkOpenGLFluidMapper() :
    m_VBOs(vtkOpenGLVertexBufferObjectGroup::New()),
    m_TempMatrix4(vtkMatrix4x4::New()) {
    for(int i = 0; i < NumTexBuffers; ++i) {
        m_TexBuffer[i] = vtkTextureObject::New();
    }
    for(int i = 0; i < NumOptionalTexBuffers; ++i) {
        m_OptionalTexBuffer[i] = vtkTextureObject::New();
    }
}

//-----------------------------------------------------------------------------
vtkOpenGLFluidMapper::~vtkOpenGLFluidMapper() {
    m_TempMatrix4->Delete();
    m_VBOs->Delete();
    for(int i = 0; i < NumTexBuffers; ++i) {
        m_TexBuffer[i]->Delete();
    }
    for(int i = 0; i < NumOptionalTexBuffers; ++i) {
        m_OptionalTexBuffer[i]->Delete();
    }
}

//-----------------------------------------------------------------------------
void
vtkOpenGLFluidMapper::PrintSelf(ostream& os, vtkIndent indent) {
    Superclass::PrintSelf(os, indent);
    os << indent << "Particle radius: " << ParticleRadius << "\n";
}

//----------------------------------------------------------------------------
void
vtkOpenGLFluidMapper::SetInputData(vtkPolyData* particles) {
    SetInputDataInternal(0, particles);
    m_bHasVertexColor = false;
}

void
vtkOpenGLFluidMapper::SetInputData(vtkPolyData* particles, vtkPolyData* colors) {
    SetNumberOfInputPorts(2);
    SetInputDataInternal(0, particles);
    SetInputDataInternal(1, colors);
    m_bHasVertexColor = true;
}

//-----------------------------------------------------------------------------
void
vtkOpenGLFluidMapper::UpdateDepthThicknessColorShaders(vtkOpenGLHelper& glHelper, vtkRenderer* renderer, vtkVolume* actor) {
    const auto renderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
    glHelper.VAO->Bind();

    // Has something changed that would require us to recreate the shader?
    if(!glHelper.Program) {
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
        if(newProgram != glHelper.Program) {
            glHelper.Program = newProgram;
            // reset the VAO as the shader has changed
            glHelper.VAO->ReleaseGraphicsResources();
        }
        glHelper.ShaderSourceTime.Modified();
    } else {
        renderWindow->GetShaderCache()->ReadyShaderProgram(glHelper.Program);
    }

    if(glHelper.Program) {
        SetDepthThicknessColorShaderParameters(glHelper, renderer, actor);

        // Allow the program to set what it wants
        InvokeEvent(vtkCommand::UpdateShaderEvent, glHelper.Program);
    }
}

//-----------------------------------------------------------------------------
void
vtkOpenGLFluidMapper::SetDepthThicknessColorShaderParameters(vtkOpenGLHelper& glHelper,
                                                             vtkRenderer*, vtkVolume* actor) {
    if(glHelper.IBO->IndexCount
       && (m_VBOs->GetMTime() > glHelper.AttributeUpdateTime
           || glHelper.ShaderSourceTime > glHelper.AttributeUpdateTime)) {
        glHelper.VAO->Bind();
        m_VBOs->AddAllAttributesToVAO(glHelper.Program, glHelper.VAO);
        glHelper.AttributeUpdateTime.Modified();
    }

    const auto program = glHelper.Program;

    // Set texture and particle radius
    program->SetUniformi("opaqueZTexture", m_TexBuffer[OpaqueZ]->GetTextureUnit());
    program->SetUniformf("particleRadius", ParticleRadius);

    // Set camera
    if(program->IsUniformUsed("VCDCMatrix")) {
        program->SetUniformMatrix("VCDCMatrix", m_CamVCDC);
    }

    if(program->IsUniformUsed("MCVCMatrix")) {
        if(!actor->GetIsIdentity()) {
            vtkMatrix4x4* mcwc;
            vtkMatrix3x3* anorms;
            ((vtkOpenGLActor*)actor)->GetKeyMatrices(mcwc, anorms);
            vtkMatrix4x4::Multiply4x4(mcwc, m_CamWCVC, m_TempMatrix4);
            program->SetUniformMatrix("MCVCMatrix", m_TempMatrix4);
        } else {
            program->SetUniformMatrix("MCVCMatrix", m_CamWCVC);
        }
    }
    if(program->IsUniformUsed("cameraParallel")) {
        glHelper.Program->SetUniformi("cameraParallel", m_CamParallelProjection);
    }
}

void
vtkOpenGLFluidMapper::SetupBuffers(vtkOpenGLRenderWindow* const renderWindow) {
    // create textures we need if not done already
    if(m_TexBuffer[0]->GetHandle() == 0) {
        for(int i = 0; i < NumTexBuffers; ++i) {
            m_TexBuffer[i]->SetContext(renderWindow);
            switch(i) {
                case OpaqueZ:
                case FluidZ:
                    m_TexBuffer[i]->AllocateDepth(static_cast<unsigned int>(m_ViewportWidth),
                                                  static_cast<unsigned int>(m_ViewportHeight), vtkTextureObject::Float32);
                    break;
                case FluidEyeZ:
                case SmoothedFluidEyeZ:
                case FluidThickness:
                case SmoothedFluidThickness:
                    m_TexBuffer[i]->Allocate2D(static_cast<unsigned int>(m_ViewportWidth),
                                               static_cast<unsigned int>(m_ViewportHeight), 1, VTK_FLOAT);
                    break;
                case FluidNormal:
                    m_TexBuffer[i]->Allocate2D(static_cast<unsigned int>(m_ViewportWidth),
                                               static_cast<unsigned int>(m_ViewportHeight), 3, VTK_FLOAT);
                    break;
                case OpaqueRGBA:
                    m_TexBuffer[i]->Allocate2D(static_cast<unsigned int>(m_ViewportWidth),
                                               static_cast<unsigned int>(m_ViewportHeight), 4, VTK_UNSIGNED_CHAR);
                    break;
                default:;
            }

            m_TexBuffer[i]->SetMinificationFilter(vtkTextureObject::Nearest);
            m_TexBuffer[i]->SetMagnificationFilter(vtkTextureObject::Nearest);
            m_TexBuffer[i]->SetWrapS(vtkTextureObject::ClampToEdge);
            m_TexBuffer[i]->SetWrapT(vtkTextureObject::ClampToEdge);
        }
    } else {
        // make sure we handle size changes
        for(int i = 0; i < NumTexBuffers; ++i) {
            m_TexBuffer[i]->Resize(static_cast<unsigned int>(m_ViewportWidth),
                                   static_cast<unsigned int>(m_ViewportHeight));
        }
    }

    // Allocate additional 2 texture bufferes for color data
    if(m_bHasVertexColor) {
        if(m_OptionalTexBuffer[0]->GetHandle() == 0) {
            for(int i = 0; i < NumOptionalTexBuffers; ++i) {
                m_OptionalTexBuffer[i]->SetContext(renderWindow);
                m_OptionalTexBuffer[i]->Allocate2D(static_cast<unsigned int>(m_ViewportWidth),
                                                   static_cast<unsigned int>(m_ViewportHeight), 3, VTK_FLOAT);
                m_OptionalTexBuffer[i]->SetMinificationFilter(vtkTextureObject::Nearest);
                m_OptionalTexBuffer[i]->SetMagnificationFilter(vtkTextureObject::Nearest);
                m_OptionalTexBuffer[i]->SetWrapS(vtkTextureObject::ClampToEdge);
                m_OptionalTexBuffer[i]->SetWrapT(vtkTextureObject::ClampToEdge);
            }
        } else {
            // make sure we handle size changes
            for(int i = 0; i < NumOptionalTexBuffers; ++i) {
                m_OptionalTexBuffer[i]->Resize(static_cast<unsigned int>(m_ViewportWidth),
                                               static_cast<unsigned int>(m_ViewportHeight));
            }
        }
    }

    // copy the opaque buffers into textures
    m_TexBuffer[OpaqueZ]->CopyFromFrameBuffer(m_ViewportX, m_ViewportY,
                                              m_ViewportX, m_ViewportY,
                                              m_ViewportWidth, m_ViewportHeight);
    m_TexBuffer[OpaqueRGBA]->CopyFromFrameBuffer(m_ViewportX, m_ViewportY,
                                                 m_ViewportX, m_ViewportY,
                                                 m_ViewportWidth, m_ViewportHeight);

    if(!m_FBFluidEyeZ) {
        m_FBFluidEyeZ = vtkOpenGLFramebufferObject::New();
        m_FBFluidEyeZ->SetContext(renderWindow);
        m_FBFluidEyeZ->AddDepthAttachment(m_TexBuffer[FluidZ]); // Must have a depth buffer
    }

    if(!m_FBThickness) {
        m_FBThickness = vtkOpenGLFramebufferObject::New();
        m_FBThickness->SetContext(renderWindow);
        m_FBThickness->AddDepthAttachment(m_TexBuffer[FluidZ]); // Must have a depth buffer
    }

    if(!m_FBFilterThickness) {
        m_FBFilterThickness = vtkOpenGLFramebufferObject::New();
        m_FBFilterThickness->SetContext(renderWindow);
        // Color attachment will be dynamically added later
    }

    if(!m_FBFilterDepth) {
        m_FBFilterDepth = vtkOpenGLFramebufferObject::New();
        m_FBFilterDepth->SetContext(renderWindow);
        // Color attachment will be dynamically added later
    }

    if(!m_FBCompNormal) {
        m_FBCompNormal = vtkOpenGLFramebufferObject::New();
        m_FBCompNormal->SetContext(renderWindow);
        m_FBCompNormal->AddColorAttachment(0, m_TexBuffer[FluidNormal]);
    }
}

//----------------------------------------------------------------------------
void
vtkOpenGLFluidMapper::Render(vtkRenderer* renderer, vtkVolume* vol) {
    if(std::abs(ParticleRadius) < 1e-8f) {
        throw std::runtime_error("Particle radius has not been set");
    }

    // Get the viewport dimensions
    renderer->GetTiledSizeAndOrigin(&m_ViewportWidth, &m_ViewportHeight, &m_ViewportX, &m_ViewportY);

    // Get the camera parameters
    const auto cam = static_cast<vtkOpenGLCamera*>(renderer->GetActiveCamera());
    cam->GetKeyMatrices(renderer, m_CamWCVC, m_CamNorms, m_CamVCDC, m_CamWCDC);
    m_CamParallelProjection = cam->GetParallelProjection();

    // Prepare the texture and frame buffers
    const auto renderWindow = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
    SetupBuffers(renderWindow);

    const auto glState = renderWindow->GetState();
    glState->vtkglViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
    bool saveScissorTestState = glState->GetEnumState(GL_SCISSOR_TEST);
#ifdef GL_MULTISAMPLE
    glState->vtkglDisable(GL_MULTISAMPLE);
#endif

    // Generate depth
    {
        // Attache texture every time, since it will be swapped out during smoothing
        m_FBFluidEyeZ->AddColorAttachment(0, m_TexBuffer[FluidEyeZ]);
        m_FBFluidEyeZ->SaveCurrentBindingsAndBuffers();
        m_FBFluidEyeZ->Bind();
        glState->vtkglDisable(GL_SCISSOR_TEST);
        glState->vtkglClearDepth(1.0);
        glState->vtkglColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
        glState->vtkglClearColor(-1.0e9, 0.0, 0.0, 0.0); // Set a clear color value to be -inf
        glState->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the spheres to get the eye coordinate z values
        m_TexBuffer[OpaqueZ]->Activate();
        glState->vtkglDepthMask(GL_TRUE);
        glState->vtkglEnable(GL_DEPTH_TEST);
        glState->vtkglDepthFunc(GL_LEQUAL);
        RenderParticles(renderer, vol, true);
        m_TexBuffer[OpaqueZ]->Deactivate();
        m_FBFluidEyeZ->RestorePreviousBindingsAndBuffers();
    }

    // Generate thickness and color (if applicable)
    {
        // Attache texture every time, since it will be swapped out during smoothing
        m_FBThickness->SaveCurrentBindingsAndBuffers();
        m_FBThickness->Bind();
        m_FBThickness->AddColorAttachment(0, m_TexBuffer[FluidThickness]);
        if(m_bHasVertexColor) {
            m_FBThickness->AddColorAttachment(1, m_OptionalTexBuffer[Color]);
            m_FBThickness->ActivateDrawBuffers(2);
        }
        glState->vtkglDisable(GL_SCISSOR_TEST);
        glState->vtkglClearDepth(1.0);
        glState->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
        glState->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
        glState->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        vtkOpenGLState::ScopedglBlendFuncSeparate bf(glState);
        glState->vtkglBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

        m_TexBuffer[OpaqueZ]->Activate();
        glState->vtkglDepthMask(GL_FALSE);
        glState->vtkglDisable(GL_DEPTH_TEST);
        glState->vtkglDepthFunc(GL_ALWAYS);
        RenderParticles(renderer, vol, false);
        m_TexBuffer[OpaqueZ]->Deactivate();
        m_FBThickness->RestorePreviousBindingsAndBuffers();
    }

    // Filter fluid thickness and color (if applicable)
    if(1) {
        if(!m_QuadThicknessFilter) {
            m_QuadThicknessFilter =
                new vtkOpenGLQuadHelper(renderWindow, nullptr, vtkFluidMapperThicknessAndVolumeColorFilterFS, "");
        } else {
            renderWindow->GetShaderCache()->ReadyShaderProgram(m_QuadThicknessFilter->Program);
        }
        const auto program = m_QuadThicknessFilter->Program;
        assert(program);

        // Attache texture every time, since it will be swapped out during smoothing
        m_FBFilterThickness->SaveCurrentBindingsAndBuffers();
        m_FBFilterThickness->Bind();

        for(uint32_t iter = 0; iter < ThicknessAndVolumeColorFilterIterations; ++iter) {
            m_FBFilterThickness->AddColorAttachment(0, m_TexBuffer[SmoothedFluidThickness]);
            if(m_bHasVertexColor) {
                m_FBFilterThickness->AddColorAttachment(1, m_OptionalTexBuffer[SmoothedColor]);
                m_FBFilterThickness->ActivateDrawBuffers(2);
                m_OptionalTexBuffer[Color]->Activate();
                program->SetUniformi("hasVertexColor",    m_bHasVertexColor);
                program->SetUniformi("fluidColorTexture", m_OptionalTexBuffer[Color]->GetTextureUnit());
            }

            m_TexBuffer[FluidThickness]->Activate();
            program->SetUniformi("fluidThicknessTexture", m_TexBuffer[FluidThickness]->GetTextureUnit());

            program->SetUniformi("viewportHeight",        m_ViewportHeight);
            program->SetUniformi("viewportWidth",         m_ViewportWidth);
            program->SetUniformi("filterRadius",          static_cast<int>(ThicknessAndVolumeColorFilterRadius));

            m_QuadThicknessFilter->Render();
            m_TexBuffer[FluidThickness]->Deactivate();

            std::swap(m_TexBuffer[FluidThickness], m_TexBuffer[SmoothedFluidThickness]);
            if(m_bHasVertexColor) {
                m_OptionalTexBuffer[Color]->Deactivate();
                std::swap(m_OptionalTexBuffer[Color], m_OptionalTexBuffer[SmoothedColor]);
            }
        }
        m_FBFilterThickness->RestorePreviousBindingsAndBuffers();
    }

    if(1) {
        // Filter depth surface
        if(DisplayMode != UnfilteredOpaqueSurface
           && DisplayMode != UnfilteredSurfaceNormal) {
            if(!m_QuadFluidDepthFilter[SurfaceFilterMethod]) {
                switch(SurfaceFilterMethod) {
                    case BilateralGaussian:
                        m_QuadFluidDepthFilter[SurfaceFilterMethod] =
                            new vtkOpenGLQuadHelper(renderWindow, nullptr, vtkFluidMapperDepthFilterBiGaussFS, "");
                        break;
                    case NarrowRange:
                        m_QuadFluidDepthFilter[SurfaceFilterMethod] =
                            new vtkOpenGLQuadHelper(renderWindow, nullptr, vtkFluidMapperDepthFilterNarrowRangeFS, "");
                        break;
                    // New filter method is added here
                    default:
                        throw std::runtime_error("Invalid filter method");
                }
            } else {
                renderWindow->GetShaderCache()->ReadyShaderProgram(m_QuadFluidDepthFilter[SurfaceFilterMethod]->Program);
            }

            const auto program = m_QuadFluidDepthFilter[SurfaceFilterMethod]->Program;
            assert(program);
            m_FBFilterDepth->SaveCurrentBindingsAndBuffers();
            m_FBFilterDepth->Bind();

            for(uint32_t iter = 0; iter < SurfaceFilterIterations; ++iter) {
                m_FBFilterDepth->AddColorAttachment(0, m_TexBuffer[SmoothedFluidEyeZ]); // Replace color attachement
                program->SetUniformi("viewportHeight", m_ViewportHeight);
                program->SetUniformi("viewportWidth",  m_ViewportWidth);
                program->SetUniformi("filterRadius",   static_cast<int>(SurfaceFilterRadius));
                program->SetUniformf("particleRadius", ParticleRadius);

                switch(SurfaceFilterMethod) {
                    case BilateralGaussian:
                        program->SetUniformf("sigmaDepth", BiGaussFilterSigmaDepth);
                        break;
                    case NarrowRange:
                        program->SetUniformf("lambda",     NRFilterLambda);
                        program->SetUniformf("mu",         NRFilterMu);
                        break;
                    // New filter method is added here
                    default:
                        throw std::runtime_error("Invalid filter method");
                }

                glState->vtkglEnable(GL_DEPTH_TEST);
                m_TexBuffer[FluidEyeZ]->Activate();
                program->SetUniformi("fluidZTexture", m_TexBuffer[FluidEyeZ]->GetTextureUnit());

                m_QuadFluidDepthFilter[SurfaceFilterMethod]->Render();
                m_TexBuffer[FluidEyeZ]->Deactivate();

                // Swap the filtered buffers
                std::swap(m_TexBuffer[FluidEyeZ], m_TexBuffer[SmoothedFluidEyeZ]);
            }

            m_FBFilterDepth->RestorePreviousBindingsAndBuffers();
        }
    }

    // Compute normal for the filtered depth surface
    if(1) {
        if(!m_QuadFluidNormal) {
            m_QuadFluidNormal = new vtkOpenGLQuadHelper(renderWindow, nullptr, vtkFluidMapperSurfaceNormalFS, "");
        } else {
            renderWindow->GetShaderCache()->ReadyShaderProgram(m_QuadFluidNormal->Program);
        }

        const auto program = m_QuadFluidNormal->Program;
        assert(program);

        m_FBCompNormal->SaveCurrentBindingsAndBuffers();
        m_FBCompNormal->Bind();
        program->SetUniformi("fluidZTexture",  m_TexBuffer[FluidEyeZ]->GetTextureUnit());
        program->SetUniformi("viewportHeight", m_ViewportHeight);
        program->SetUniformi("viewportWidth",  m_ViewportWidth);
        if(program->IsUniformUsed("VCDCMatrix")) {
            program->SetUniformMatrix("VCDCMatrix", m_CamVCDC);
        }

        glState->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
        glState->vtkglDepthMask(GL_FALSE);
        glState->vtkglDisable(GL_DEPTH_TEST);
        glState->vtkglDepthFunc(GL_ALWAYS);

        m_TexBuffer[FluidEyeZ]->Activate();
        m_QuadFluidNormal->Render();
        m_TexBuffer[FluidEyeZ]->Deactivate();
        m_FBCompNormal->RestorePreviousBindingsAndBuffers();
    }

    // Restore the original viewport properties
    glState->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glState->vtkglViewport(m_ViewportX, m_ViewportY, m_ViewportWidth, m_ViewportHeight);
    saveScissorTestState ? glState->vtkglEnable(GL_SCISSOR_TEST) : glState->vtkglDisable(GL_SCISSOR_TEST);

    {
        // Final blend, render everything
        if(!m_QuadFinalBlend) {
            m_QuadFinalBlend = new vtkOpenGLQuadHelper(renderWindow, nullptr, vtkFluidMapperFinalFS, "");
        } else {
            renderWindow->GetShaderCache()->ReadyShaderProgram(m_QuadFinalBlend->Program);
        }

        const auto program = m_QuadFinalBlend->Program;
        assert(program);

        m_TexBuffer[FluidEyeZ]->Activate();
        program->SetUniformi("fluidZTexture",         m_TexBuffer[FluidEyeZ]->GetTextureUnit());

        m_TexBuffer[FluidThickness]->Activate();
        program->SetUniformi("fluidThicknessTexture", m_TexBuffer[FluidThickness]->GetTextureUnit());

        m_TexBuffer[FluidNormal]->Activate();
        program->SetUniformi("fluidNormalTexture",    m_TexBuffer[FluidNormal]->GetTextureUnit());

        m_TexBuffer[OpaqueRGBA]->Activate();
        program->SetUniformi("opaqueRGBATexture",     m_TexBuffer[OpaqueRGBA]->GetTextureUnit());

        if(m_bHasVertexColor) {
            m_OptionalTexBuffer[Color]->Activate();
            program->SetUniformi("fluidColorTexture", m_OptionalTexBuffer[Color]->GetTextureUnit());
            program->SetUniformi("hasVertexColor",    m_bHasVertexColor);
            program->SetUniformf("vertexColorPower", ParticleColorPower);
            program->SetUniformf("vertexColorScale", ParticleColorScale);
        }

        program->SetUniformMatrix("VCDCMatrix", m_CamVCDC);
        if(m_QuadFinalBlend->Program->IsUniformUsed("MCVCMatrix")) {
            if(!vol->GetIsIdentity()) {
                vtkMatrix4x4* mcwc;
                vtkMatrix3x3* anorms;
                ((vtkOpenGLActor*)vol)->GetKeyMatrices(mcwc, anorms);
                vtkMatrix4x4::Multiply4x4(mcwc, m_CamWCVC, m_TempMatrix4);
                m_QuadFinalBlend->Program->SetUniformMatrix("MCVCMatrix", m_TempMatrix4);
            } else {
                m_QuadFinalBlend->Program->SetUniformMatrix("MCVCMatrix", m_CamWCVC);
            }
        }

        program->SetUniformi("displayModeOpaqueSurface", DisplayMode == UnfilteredOpaqueSurface || DisplayMode == FilteredOpaqueSurface);
        program->SetUniformi("displayModeSurfaceNormal", DisplayMode == UnfilteredSurfaceNormal || DisplayMode == FilteredSurfaceNormal);
        program->SetUniformf("attennuationScale",    AttennuationScale);
        program->SetUniformf("additionalReflection", AdditionalReflection);
        program->SetUniformf("refractiveIndex",      RefractiveIndex);
        program->SetUniformf("refractionScale",      RefractionScale);
        program->SetUniform3f("fluidOpaqueColor",       OpaqueColor);
        program->SetUniform3f("fluidAttennuationColor", AttennuationColor);

        glState->vtkglEnable(GL_DEPTH_TEST);
        glState->vtkglDepthMask(GL_TRUE);
        glState->vtkglDepthFunc(GL_ALWAYS);

        m_QuadFinalBlend->Render();

        m_TexBuffer[OpaqueZ]->Deactivate();
        m_TexBuffer[OpaqueRGBA]->Deactivate();
        m_TexBuffer[FluidEyeZ]->Deactivate();
        m_TexBuffer[FluidThickness]->Deactivate();
        m_TexBuffer[FluidNormal]->Deactivate();
        if(m_bHasVertexColor) {
            m_OptionalTexBuffer[Color]->Deactivate();
        }

        glState->vtkglDepthFunc(GL_LEQUAL);
    }
}

//-----------------------------------------------------------------------------
void
vtkOpenGLFluidMapper::RenderParticles(vtkRenderer* renderer, vtkVolume* vol, bool bDepthShader) {
    vtkPolyData* positionData = vtkPolyData::SafeDownCast(GetInputDataObject(0, 0));
    if(positionData == nullptr || positionData->GetPoints() == nullptr) {
        return;
    }

    if(m_VBOBuildTime < positionData->GetPoints()->GetMTime()) {
        m_VBOs->CacheDataArray("vertexMC", positionData->GetPoints()->GetData(), renderer, VTK_FLOAT);
        if(m_bHasVertexColor) {
            vtkPolyData* colorData = vtkPolyData::SafeDownCast(GetInputDataObject(1, 0));
            m_VBOs->CacheDataArray("vertexColor", colorData->GetPoints()->GetData(), renderer, VTK_FLOAT);
        }
        m_VBOs->BuildAllVBOs(renderer);

        vtkIdType numPts = positionData->GetPoints()->GetNumberOfPoints();
        m_GLHelperDepthThickness.IBO->IndexCount = static_cast<size_t>(numPts);
        m_VBOBuildTime.Modified();
    }

    // draw polygons
    int numVerts = m_VBOs->GetNumberOfTuples("vertexMC");
    if(numVerts) {
        // First we do the triangles, update the shader, set uniforms, etc.
        UpdateDepthThicknessColorShaders(m_GLHelperDepthThickness, renderer, vol);

        const auto program = m_GLHelperDepthThickness.Program;
        program->SetUniformi("outputEyeZ", bDepthShader);
        if(m_bHasVertexColor) {
            program->SetUniformi("hasVertexColor", m_bHasVertexColor);
        }
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(numVerts));
    }
}
