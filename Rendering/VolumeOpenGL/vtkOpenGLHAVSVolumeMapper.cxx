/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenGLHAVSVolumeMapper.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/* Copyright 2005, 2006 by University of Utah. */

#include "vtkOpenGLHAVSVolumeMapper.h"

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkgl.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGL.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridPartialPreIntegration.h"
#include "vtkVolumeProperty.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkHAVSVolumeMapper_kbufferVP.h"
#include "vtkHAVSVolumeMapper_k2BeginFP.h"
#include "vtkHAVSVolumeMapper_k2FP.h"
#include "vtkHAVSVolumeMapper_k2EndFP.h"
#include "vtkHAVSVolumeMapper_k6BeginFP.h"
#include "vtkHAVSVolumeMapper_k6FP.h"
#include "vtkHAVSVolumeMapper_k6EndFP.h"

vtkStandardNewMacro(vtkOpenGLHAVSVolumeMapper);

//----------------------------------------------------------------------------
// return the correct type of UnstructuredGridVolumeMapper
vtkOpenGLHAVSVolumeMapper::vtkOpenGLHAVSVolumeMapper()
{
  this->VBOVertexName              = 0;
  this->VBOTexCoordName            = 0;
  this->VBOVertexIndexName         = 0;
  this->PsiTableTexture            = 0;
  this->FramebufferObjectSize      = 0;
  this->OrderedTriangles           = 0;
  this->RenderWindow=0;
}

//----------------------------------------------------------------------------
// return the correct type of UnstructuredGridVolumeMapper
vtkOpenGLHAVSVolumeMapper::~vtkOpenGLHAVSVolumeMapper()
{
  if (!this->GPUDataStructures)
    {
    if(this->OrderedTriangles!=0)
      {
      delete [] this->OrderedTriangles;
      }
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLHAVSVolumeMapper::CheckOpenGLError(const char * str)
{
  int err = glGetError(); (void)str;
  if ( err != GL_NO_ERROR && this->GetDebug() )
    {
    vtkDebugMacro( << "OpenGL Error: " << str );
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLHAVSVolumeMapper::ReleaseGraphicsResources(vtkWindow *renWin)
{
  if (this->Initialized)
    {
    static_cast<vtkRenderWindow *>(renWin)->MakeCurrent();
    this->DeleteShaders();

    glDisable( vtkgl::VERTEX_PROGRAM_ARB );
    glDisable( vtkgl::FRAGMENT_PROGRAM_ARB );

    glDeleteTextures(1, reinterpret_cast<GLuint *>(&this->TransferFunctionTexture));

    int numBuffers = (this->KBufferState == VTK_KBUFFER_SIZE_2)? 2 : 4;
    for (int i = 0; i < numBuffers; i++)
      {
      glDeleteTextures(1,reinterpret_cast<GLuint *>(&this->FramebufferTextures[i]));
      }

    vtkgl::DeleteFramebuffersEXT(1,reinterpret_cast<GLuint *>(&this->FramebufferObject));
    this->Initialized = false;
    if (this->GPUDataStructures)
      {
      vtkgl::DeleteBuffers(1, reinterpret_cast<GLuint *>(&this->VBOVertexName));
      vtkgl::DeleteBuffers(1, reinterpret_cast<GLuint *>(&this->VBOTexCoordName));
      vtkgl::DeleteBuffers(1,reinterpret_cast<GLuint *>(&this->VBOVertexIndexName));
      vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER, 0);
      vtkgl::BindBuffer(vtkgl::ELEMENT_ARRAY_BUFFER, 0);
      }
    }
  this->Superclass::ReleaseGraphicsResources(renWin);
}

//----------------------------------------------------------------------------
void vtkOpenGLHAVSVolumeMapper::Initialize(vtkRenderer *ren,
                                           vtkVolume *vol)
{
  // Check for the required extensions only.
  if (!this->SupportedByHardware(ren))
    {
    this->InitializationError = vtkHAVSVolumeMapper::UNSUPPORTED_EXTENSIONS;
    return;
    }

  vtkOpenGLExtensionManager *extensions=
    static_cast<vtkOpenGLRenderWindow *>(ren->GetRenderWindow())
    ->GetExtensionManager();

  // Load required extensions

  // supports_GL_1_3=1 as checked by this->SupportedByHardware()
  // OpenGL 1.3 is required by GL_ARB_draw_buffers, GL_ARB_fragment_program
  //  and GL_ARB_vertex_program.
  // CLAMP_TO_EGDE is core feature of OpenGL 1.2 and
  // multitexture is core feature of OpenGL 1.3.
  extensions->LoadExtension("GL_VERSION_1_3"); // multitexture

  // supports_draw_buffers as checked by this->SupportedByHardware()
  int supports_GL_2_0=extensions->ExtensionSupported( "GL_VERSION_2_0" );

  if(supports_GL_2_0)
    {
    extensions->LoadExtension("GL_VERSION_2_0");
    }
  else
    {
    extensions->LoadCorePromotedExtension( "GL_ARB_draw_buffers" );
    }

  // supports_fragment_program && supports_vertex_program as checked
  // by this->SupportedByHardware()
  extensions->LoadExtension( "GL_ARB_fragment_program" );
  extensions->LoadExtension( "GL_ARB_vertex_program" );

  // supports_GL_EXT_framebuffer_object==1 as checked
  // by this->SupportedByHardware()
  extensions->LoadExtension("GL_EXT_framebuffer_object");

  // GL_ARB_texture_float or GL_ATI_texture_float introduce new tokens but
  // no new function: don't need to call LoadExtension.

  // Optional extension.
  int supports_GL_1_5=extensions->ExtensionSupported( "GL_VERSION_1_5" );
  int supports_vertex_buffer_object;

  if(supports_GL_1_5)
    {
    supports_vertex_buffer_object=1;
    }
  else
    {
    supports_vertex_buffer_object =
      extensions->ExtensionSupported( "GL_ARB_vertex_buffer_object" );
    }

  if(supports_vertex_buffer_object)
    {
    if(supports_GL_1_5)
      {
      extensions->LoadExtension("GL_VERSION_1_5");
      }
    else
      {
      extensions->LoadCorePromotedExtension( "GL_ARB_vertex_buffer_object" );
      }
    }

  if (!supports_vertex_buffer_object)
    {
    this->SetGPUDataStructures(false);
    }

  this->UpdateProgress(0.0);

  // Initialize triangles and VBOs or Vertex Arrays
  this->InitializePrimitives(vol);

  this->UpdateProgress(0.4);

  // Initialize scalars and VBOs
  this->InitializeScalars();

  this->UpdateProgress(0.5);

  // Initialize Level-of-Detail data structures
  this->InitializeLevelOfDetail();

  this->UpdateProgress(0.7);

  // Initialize Lookup tables
  this->InitializeLookupTables(vol);

  this->UpdateProgress(0.8);

  // Initialize vertex and scalar storage
  this->InitializeGPUDataStructures();

  this->UpdateProgress(0.9);

  // Initialize shaders
  this->InitializeShaders();

  // Initialize FBOs
  this->InitializeFramebufferObject();

  this->UpdateProgress(1.0);

  this->Initialized = 1;
}

//----------------------------------------------------------------------------
// Change GPU data structures state
void vtkOpenGLHAVSVolumeMapper::SetGPUDataStructures(bool gpu)
{
  if(this->GPUDataStructures!=gpu)
    {
    if(!this->GPUDataStructures)
      {
      if(this->OrderedTriangles!=0)
        {
        delete [] this->OrderedTriangles;
        this->OrderedTriangles=0;
        }
      }
    this->GPUDataStructures = gpu;
    if (this->Initialized)
      {
      this->InitializeGPUDataStructures();
      }
    }
}

//----------------------------------------------------------------------------
// Store data structures on GPU if possible
void vtkOpenGLHAVSVolumeMapper::InitializeGPUDataStructures()
{
  if (this->GPUDataStructures)
    {
    if (VBOVertexName)
      {
      vtkgl::DeleteBuffers(1, reinterpret_cast<GLuint *>(&this->VBOVertexName));
      }
    if (VBOVertexIndexName)
      {
      vtkgl::DeleteBuffers(1,reinterpret_cast<GLuint *>(&this->VBOVertexIndexName));
      }
    if (VBOTexCoordName)
      {
      vtkgl::DeleteBuffers(1, reinterpret_cast<GLuint *>(&this->VBOTexCoordName));
      }

    // Build vertex array
    vtkgl::GenBuffers(1, reinterpret_cast<GLuint *>(&this->VBOVertexName));
    vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER, this->VBOVertexName);
    vtkgl::BufferData(vtkgl::ARRAY_BUFFER,
                      this->NumberOfVertices*3*sizeof(float),
                      this->Vertices, vtkgl::STATIC_DRAW);
    // Build dynamic vertex index array
    vtkgl::GenBuffers(1, reinterpret_cast<GLuint *>(&this->VBOVertexIndexName));
    vtkgl::BindBuffer(vtkgl::ELEMENT_ARRAY_BUFFER,
                      this->VBOVertexIndexName);
    vtkgl::BufferData(vtkgl::ELEMENT_ARRAY_BUFFER,
                      this->NumberOfTriangles*3*sizeof(GLuint), 0,
                      vtkgl::STREAM_DRAW);

    vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER, 0);
    vtkgl::BindBuffer(vtkgl::ELEMENT_ARRAY_BUFFER, 0);
    this->CheckOpenGLError("Initializing VBOs");

    // Build tex coord array
    vtkgl::GenBuffers(1, reinterpret_cast<GLuint *>(&this->VBOTexCoordName));
    vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER, this->VBOTexCoordName);
    vtkgl::BufferData(vtkgl::ARRAY_BUFFER,
                         this->NumberOfScalars*sizeof(float),
                         this->Scalars, vtkgl::STATIC_DRAW);
    vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER, 0);
    }
  else
    {
    if (this->OrderedTriangles)
      {
      delete [] this->OrderedTriangles;
      }
    this->OrderedTriangles = new unsigned int[this->NumberOfTriangles*3];
    }
}

//----------------------------------------------------------------------------
// Vertex and Fragment shaders
void vtkOpenGLHAVSVolumeMapper::InitializeShaders()
{
  // Create vertex shader
  glEnable( vtkgl::VERTEX_PROGRAM_ARB );
  vtkgl::GenProgramsARB(1, reinterpret_cast<GLuint *>(&this->VertexProgram));
  vtkgl::BindProgramARB(vtkgl::VERTEX_PROGRAM_ARB, this->VertexProgram);
  vtkgl::ProgramStringARB(vtkgl::VERTEX_PROGRAM_ARB,
                          vtkgl::PROGRAM_FORMAT_ASCII_ARB,
                          static_cast<GLsizei>(strlen(vtkHAVSVolumeMapper_kbufferVP)),
                          vtkHAVSVolumeMapper_kbufferVP);

  // Create fragment shaders
  glEnable( vtkgl::FRAGMENT_PROGRAM_ARB );
  if (this->KBufferSize == VTK_KBUFFER_SIZE_2)
    {
    vtkgl::GenProgramsARB(1,
                          reinterpret_cast<GLuint *>(&this->FragmentProgramBegin));
    vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                          this->FragmentProgramBegin);
    vtkgl::ProgramStringARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                            vtkgl::PROGRAM_FORMAT_ASCII_ARB,
                            static_cast<GLsizei>(strlen(vtkHAVSVolumeMapper_k2BeginFP)),
                            vtkHAVSVolumeMapper_k2BeginFP);
    vtkgl::GenProgramsARB(1, reinterpret_cast<GLuint *>(&this->FragmentProgram));
    vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB, this->FragmentProgram);
    vtkgl::ProgramStringARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                            vtkgl::PROGRAM_FORMAT_ASCII_ARB,
                            static_cast<GLsizei>(strlen(vtkHAVSVolumeMapper_k2FP)),
                            vtkHAVSVolumeMapper_k2FP);
    vtkgl::GenProgramsARB(1, reinterpret_cast<GLuint *>(&this->FragmentProgramEnd));
    vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                          this->FragmentProgramEnd);
    vtkgl::ProgramStringARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                            vtkgl::PROGRAM_FORMAT_ASCII_ARB,
                            static_cast<GLsizei>(strlen(vtkHAVSVolumeMapper_k2EndFP)),
                            vtkHAVSVolumeMapper_k2EndFP);
    }
  else
    {
    vtkgl::GenProgramsARB(1,
                          reinterpret_cast<GLuint *>(&this->FragmentProgramBegin));
    vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                          this->FragmentProgramBegin);
    vtkgl::ProgramStringARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                            vtkgl::PROGRAM_FORMAT_ASCII_ARB,
                            static_cast<GLsizei>(strlen(vtkHAVSVolumeMapper_k6BeginFP)),
                            vtkHAVSVolumeMapper_k6BeginFP);
    vtkgl::GenProgramsARB(1, reinterpret_cast<GLuint *>(&this->FragmentProgram));
    vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB, this->FragmentProgram);
    vtkgl::ProgramStringARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                            vtkgl::PROGRAM_FORMAT_ASCII_ARB,
                            static_cast<GLsizei>(strlen(vtkHAVSVolumeMapper_k6FP)),
                            vtkHAVSVolumeMapper_k6FP);
    vtkgl::GenProgramsARB(1, reinterpret_cast<GLuint *>(&this->FragmentProgramEnd));
    vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                          this->FragmentProgramEnd);
    vtkgl::ProgramStringARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                            vtkgl::PROGRAM_FORMAT_ASCII_ARB,
                            static_cast<GLsizei>(strlen(vtkHAVSVolumeMapper_k6EndFP)),
                            vtkHAVSVolumeMapper_k6EndFP);
    }

  // Disable shaders
  vtkgl::BindProgramARB(vtkgl::VERTEX_PROGRAM_ARB, 0);
  vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB, 0);
  glDisable( vtkgl::VERTEX_PROGRAM_ARB );
  glDisable( vtkgl::FRAGMENT_PROGRAM_ARB );
}

//----------------------------------------------------------------------------
void vtkOpenGLHAVSVolumeMapper::DeleteShaders()
{
  vtkgl::DeleteProgramsARB(1, reinterpret_cast<GLuint *>(&this->VertexProgram));
  vtkgl::DeleteProgramsARB(1,
                           reinterpret_cast<GLuint *>(&this->FragmentProgramBegin));
  vtkgl::DeleteProgramsARB(1, reinterpret_cast<GLuint *>(&this->FragmentProgram));
  vtkgl::DeleteProgramsARB(1,
                           reinterpret_cast<GLuint *>(&this->FragmentProgramEnd));
}

//----------------------------------------------------------------------------
// Build the lookup tables used for partial pre-integration
void vtkOpenGLHAVSVolumeMapper::InitializeLookupTables(vtkVolume *vol)
{
  this->Superclass::InitializeLookupTables(vol);

  // Create a 1D texture for transfer function look up
  glGenTextures(1, reinterpret_cast<GLuint *>(&this->TransferFunctionTexture));
  glBindTexture(GL_TEXTURE_1D, this->TransferFunctionTexture);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, this->TransferFunctionSize,0,
               GL_RGBA,GL_FLOAT, this->TransferFunction);

  if (!this->PsiTableTexture)
    {
    vtkUnstructuredGridPartialPreIntegration *ppi =
      vtkUnstructuredGridPartialPreIntegration::New();
    ppi->BuildPsiTable();
    int tableSize = 0;
    float *psiTable = ppi->GetPsiTable(tableSize);

    // Create a 2D texture for the PSI lookup table
    glGenTextures(1, reinterpret_cast<GLuint *>(&this->PsiTableTexture));
    glBindTexture(GL_TEXTURE_2D, this->PsiTableTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, vtkgl::CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, tableSize, tableSize,
                 0, GL_LUMINANCE, GL_FLOAT, psiTable);
    ppi->Delete();
    }
}

//----------------------------------------------------------------------------
// Initialize FBO and attach color and depth textures.
void vtkOpenGLHAVSVolumeMapper::InitializeFramebufferObject()
{
  GLint maxRB;
  glGetIntegerv(vtkgl::MAX_RENDERBUFFER_SIZE_EXT, &maxRB);
  int texSize = (maxRB > 1024)? 1024 : maxRB;
  int numBuffers = (this->KBufferState == VTK_KBUFFER_SIZE_2)? 2 : 4;

  if (!this->Initialized)
    {
    // Create FBO
    vtkgl::GenFramebuffersEXT(1,
                              reinterpret_cast<GLuint *>(&this->FramebufferObject));
    this->CheckOpenGLError("creating FBO");
    }
  else
    {
    glDeleteTextures(numBuffers,
                     reinterpret_cast<GLuint *>(this->FramebufferTextures));
    vtkgl::DeleteRenderbuffersEXT(1,
                                  reinterpret_cast<GLuint *>(&this->DepthTexture));
    }

  numBuffers = (this->KBufferSize == VTK_KBUFFER_SIZE_2)? 2 : 4;

  // Create FBO textures
  glGenTextures(numBuffers, reinterpret_cast<GLuint *>(this->FramebufferTextures));
  for (int i = 0; i < numBuffers; i++)
    {
    glBindTexture(GL_TEXTURE_2D, this->FramebufferTextures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, vtkgl::RGBA32F_ARB, texSize, texSize, 0,
                 GL_RGBA, GL_FLOAT, 0);
    }

  this->CheckOpenGLError("creating fbo textures");

  // Bind framebuffer object
  GLint savedFrameBuffer;
  glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&savedFrameBuffer);
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, this->FramebufferObject);
  this->CheckOpenGLError("binding FBO");

  // Generate depth buffer texture for framebuffer
  vtkgl::GenRenderbuffersEXT(1, reinterpret_cast<GLuint *>(&this->DepthTexture));

  // Attach texture to framebuffer color buffer
  vtkgl::FramebufferTexture2DEXT( vtkgl::FRAMEBUFFER_EXT,
                                  vtkgl::COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                                  this->FramebufferTextures[0], 0 );
  vtkgl::FramebufferTexture2DEXT( vtkgl::FRAMEBUFFER_EXT,
                                  vtkgl::COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D,
                                  this->FramebufferTextures[1], 0 );
  if (numBuffers == 4)
    {
    vtkgl::FramebufferTexture2DEXT( vtkgl::FRAMEBUFFER_EXT,
                                    vtkgl::COLOR_ATTACHMENT2_EXT,
                                    GL_TEXTURE_2D,
                                    this->FramebufferTextures[2], 0 );
    vtkgl::FramebufferTexture2DEXT( vtkgl::FRAMEBUFFER_EXT,
                                    vtkgl::COLOR_ATTACHMENT3_EXT,
                                    GL_TEXTURE_2D,
                                    this->FramebufferTextures[3], 0 );
    }

  // Attach depth texture to framebuffer
  vtkgl::BindRenderbufferEXT(vtkgl::RENDERBUFFER_EXT, this->DepthTexture);
  vtkgl::RenderbufferStorageEXT(vtkgl::RENDERBUFFER_EXT,
                                vtkgl::DEPTH_COMPONENT24,
                                texSize, texSize);
  vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT,
                                    vtkgl::DEPTH_ATTACHMENT_EXT,
                                    vtkgl::RENDERBUFFER_EXT,
                                    this->DepthTexture);

  this->CheckOpenGLError("attach textures to FBO");

  // Validate FBO after attaching textures
  if (vtkgl::CheckFramebufferStatusEXT(vtkgl::FRAMEBUFFER_EXT) !=
      vtkgl::FRAMEBUFFER_COMPLETE_EXT && this->GetDebug())
    {
    vtkDebugMacro( << "FBO incomplete" );
    }

  // Disable FBO rendering
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,savedFrameBuffer);

  this->FramebufferObjectSize = texSize;
  this->KBufferState = this->KBufferSize;
}

//----------------------------------------------------------------------------
void vtkOpenGLHAVSVolumeMapper::Render(vtkRenderer *ren,
                                       vtkVolume *vol)
{
  ren->GetRenderWindow()->MakeCurrent();

  this->RenderWindow=ren->GetRenderWindow();

  if (!this->Initialized)
    {
    this->InitializationError =
      vtkHAVSVolumeMapper::NO_INIT_ERROR;
    this->Initialize(ren, vol);
    if (this->CheckInitializationError())
      {
      return;
      }
    }

  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  // Check to see if we need to update the lookup table
  if ((this->ColorTransferFunctionMTime <
       vol->GetProperty()->GetRGBTransferFunction()->GetMTime())
      ||(this->AlphaTransferFunctionMTime <
         vol->GetProperty()->GetScalarOpacity()->GetMTime())
      ||(this->UnitDistance != vol->GetProperty()->GetScalarOpacityUnitDistance()))
    {
    this->InitializeLookupTables(vol);
    this->ColorTransferFunctionMTime.Modified();
    this->AlphaTransferFunctionMTime.Modified();
    }

  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  // Check to see if we need to update the scalars
  if ((this->ScalarsMTime < this->MTime)
      || (this->ScalarsMTime < this->GetInput()->GetMTime())
      || (this->LastVolume != vol))
    {
    this->InitializationError =
      vtkHAVSVolumeMapper::NO_INIT_ERROR;
    this->InitializeScalars();
    this->InitializeGPUDataStructures();
    if (this->CheckInitializationError())
      return;
    this->ScalarsMTime.Modified();
    }

  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  // Check to see if we need to update the geometry
  if ((this->UnstructuredGridMTime < this->GetInput()->GetMTime())
      || (this->UnstructuredGridMTime < this->MTime))
    {
    this->InitializationError =
      vtkHAVSVolumeMapper::NO_INIT_ERROR;
    this->InitializePrimitives(vol);
    this->InitializeLevelOfDetail();
    this->InitializeGPUDataStructures();
    if (this->CheckInitializationError())
      return;
    this->UnstructuredGridMTime.Modified();
    }

  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  this->Timer->StartTimer();

  this->RenderHAVS(ren);
  this->LastVolume = vol;

  this->Timer->StopTimer();
  this->TimeToDraw = static_cast<float>(this->Timer->GetElapsedTime());

  // Update level-of-detail
  this->UpdateLevelOfDetail(TimeToDraw);
}

//----------------------------------------------------------------------------
// The OpenGL rendering
void vtkOpenGLHAVSVolumeMapper::RenderHAVS(vtkRenderer *ren)
{
  glPushAttrib(GL_ENABLE_BIT         |
               GL_CURRENT_BIT        |
               GL_COLOR_BUFFER_BIT   |
               GL_STENCIL_BUFFER_BIT |
               GL_DEPTH_BUFFER_BIT   |
               GL_POLYGON_BIT        |
               GL_TEXTURE_BIT        |
               GL_LIGHTING_BIT       |
               GL_TRANSFORM_BIT      |
               GL_VIEWPORT_BIT);

  // Setup OpenGL state
  glShadeModel(GL_SMOOTH);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  glDisable(GL_NORMALIZE);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_STENCIL_TEST);

  int screenWidth, screenHeight;
  ren->GetTiledSize(&screenWidth, &screenHeight);

  // Keep shaders up to date
  if (this->KBufferSize != this->KBufferState)
    {
    this->DeleteShaders();
    this->InitializeShaders();
    // Keep FBO up to date
    this->InitializeFramebufferObject();
    }

  int vpWidth = screenWidth;
  int vpHeight = screenHeight;
  if (screenWidth > this->FramebufferObjectSize)
    {
    vpWidth = this->FramebufferObjectSize;
    }
  if (screenHeight > this->FramebufferObjectSize)
    {
    vpHeight = this->FramebufferObjectSize;
    }

  // Bind geometry arrays
  if (this->GPUDataStructures)
    {
    glEnableClientState(GL_VERTEX_ARRAY);
    vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER, this->VBOVertexName);
    glVertexPointer(3, GL_FLOAT, 0, static_cast<char *>(NULL));
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER, this->VBOTexCoordName);
    glTexCoordPointer(1, GL_FLOAT, 0, static_cast<char *>(NULL));

    vtkgl::BindBuffer(vtkgl::ELEMENT_ARRAY_BUFFER, this->VBOVertexIndexName);
    }
  else
    {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, this->Vertices);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(1, GL_FLOAT, 0, this->Scalars);
    }

  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  // Object-space sorting
  double *e = ren->GetActiveCamera()->GetPosition();
  float eye[3];
  eye[0] = e[0];
  eye[1] = e[1];
  eye[2] = e[2];
  if (this->GPUDataStructures)
    {
    this->OrderedTriangles =
      static_cast<unsigned int *>(vtkgl::MapBuffer(vtkgl::ELEMENT_ARRAY_BUFFER,
                                                   vtkgl::WRITE_ONLY));
    }

  this->PartialVisibilitySort(eye);

  if (this->GPUDataStructures)
    {
    vtkgl::UnmapBuffer(vtkgl::ELEMENT_ARRAY_BUFFER);
    }

  this->UpdateProgress(0.4);
  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  // Get depth range from OpenGL state for correct z
  GLfloat depthRange[2];
  glGetFloatv(GL_DEPTH_RANGE, depthRange);

  // Get the current z-buffer
  float *zbuffer =
    ren->GetRenderWindow()->GetZbufferData(0,0,screenWidth-1,screenHeight-1);

  // Enable FBO Rendering
  GLint savedFrameBuffer;
  glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&savedFrameBuffer);
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, this->FramebufferObject);

  // Setup z-buffer
  this->SetupFBOZBuffer(vpWidth, vpHeight, depthRange[0], depthRange[1],
                        zbuffer);
  delete [] zbuffer;

  // Setup multiple render targets
  this->SetupFBOMRT();

  // Draw Initialization pass
  this->DrawFBOInit(vpWidth, vpHeight, depthRange[0], depthRange[1]);

  // Draw Geometry pass
  this->DrawFBOGeometry();
  this->UpdateProgress(0.9);

  // Draw Flushing pass
  this->DrawFBOFlush(vpWidth, vpHeight, depthRange[0], depthRange[1]);

  // Disable FBO rendering
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, savedFrameBuffer);

  glPopAttrib();

  // Blend Result into framebuffer
  this->DrawBlend(vpWidth, vpHeight, depthRange[0], depthRange[1]);

  this->UpdateProgress(1.0);
}

//----------------------------------------------------------------------------
// Draw the current z-buffer into the FBO z-buffer for correct compositing
// with existing geometry or widgets.
void vtkOpenGLHAVSVolumeMapper::SetupFBOZBuffer(int screenWidth,
                                                int screenHeight,
                                                float depthNear,
                                                float depthFar,
                                                float *zbuffer)
{
  // Setup view for z-buffer copy
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0, static_cast<GLdouble>(screenWidth), 0.0,
          static_cast<GLdouble>(screenHeight),
          static_cast<GLdouble>(depthNear), static_cast<GLdouble>(depthFar));
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Enable FBO z-buffer
  glEnable(GL_DEPTH_TEST);
  glClearDepth(depthFar);
  glClear(GL_DEPTH_BUFFER_BIT);
  glDepthFunc(GL_LESS);

  glDrawBuffer(vtkgl::DEPTH_ATTACHMENT_EXT);
  glRasterPos2i(0,0);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glDrawPixels(screenWidth, screenHeight, GL_DEPTH_COMPONENT, GL_FLOAT,
               zbuffer);
  glFlush();

  // Make z-buffer Read only
  glDepthMask(GL_FALSE);

  // Reset view state after z-buffer copy
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

//----------------------------------------------------------------------------
// Setup reading and writing into multiple textures from an FBO
void vtkOpenGLHAVSVolumeMapper::SetupFBOMRT()
{
  int numBuffers = (this->KBufferSize == VTK_KBUFFER_SIZE_2)? 2 : 4;
  GLenum buffers[4] = {vtkgl::COLOR_ATTACHMENT0_EXT,
                       vtkgl::COLOR_ATTACHMENT1_EXT,
                       vtkgl::COLOR_ATTACHMENT2_EXT,
                       vtkgl::COLOR_ATTACHMENT3_EXT};
  vtkgl::DrawBuffers(numBuffers, buffers);


  this->CheckOpenGLError("setup MRTs");

  // Bind textures for reading
  glEnable(GL_TEXTURE_2D);
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, this->FramebufferTextures[0]);

  vtkgl::ActiveTexture(vtkgl::TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, this->FramebufferTextures[1]);

  if (numBuffers == 2)
    {
    // Bind lookup tables
    glEnable(GL_TEXTURE_2D);
    vtkgl::ActiveTexture(vtkgl::TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, this->PsiTableTexture);

    glEnable(GL_TEXTURE_1D);
    vtkgl::ActiveTexture(vtkgl::TEXTURE3);
    glBindTexture(GL_TEXTURE_1D,this->TransferFunctionTexture);
    }
  else
    {
    // Bind lookup tables
    vtkgl::ActiveTexture(vtkgl::TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, this->FramebufferTextures[2]);

    vtkgl::ActiveTexture(vtkgl::TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, this->FramebufferTextures[3]);

    glEnable(GL_TEXTURE_2D);
    vtkgl::ActiveTexture(vtkgl::TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, this->PsiTableTexture);

    glEnable(GL_TEXTURE_1D);
    vtkgl::ActiveTexture(vtkgl::TEXTURE5);
    glBindTexture(GL_TEXTURE_1D,this->TransferFunctionTexture);
    }

  this->CheckOpenGLError("setup FBO reading");
}

//----------------------------------------------------------------------------
// Draw a screen-aligned plane with the init fragment shader enabled.  The
// init fragment shader clears the framebuffer to 0 and the k-buffers to -1.
void vtkOpenGLHAVSVolumeMapper::DrawFBOInit(int screenWidth, int screenHeight,
                                            float depthNear, float depthFar)
{
  // Bind initializing fragment shader
  glEnable(vtkgl::FRAGMENT_PROGRAM_ARB);
  vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB,
                        this->FragmentProgramBegin);

  // Setup ortho view
  glViewport(0,0,screenWidth,screenHeight);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0, static_cast<GLdouble>(screenWidth), 0.0,
          static_cast<GLdouble>(screenHeight),
          static_cast<GLdouble>(depthNear), static_cast<GLdouble>(depthFar));
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Draw a quad to initialize the k-buffer
  glBegin(GL_QUADS);
  glVertex3f(0.0, 0.0, 0.0);
  glVertex3f(static_cast<float>(screenWidth), 0.0, 0.0);
  glVertex3f(static_cast<float>(screenWidth),
             static_cast<float>(screenHeight), 0.0);
  glVertex3f(0.0, static_cast<float>(screenHeight), 0.0);
  glEnd();

  vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB, 0);

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

//----------------------------------------------------------------------------
// Draw the geometry using the k-buffer to sort and composite into the
// framebuffer.
void vtkOpenGLHAVSVolumeMapper::DrawFBOGeometry()
{
  // Bind shaders
  glEnable(vtkgl::VERTEX_PROGRAM_ARB);
  vtkgl::BindProgramARB(vtkgl::VERTEX_PROGRAM_ARB, this->VertexProgram);
  vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB, this->FragmentProgram);

  float scale = this->MaxEdgeLength;
  if (this->LevelOfDetail || !this->PartiallyRemoveNonConvexities)
    {
    scale = this->LevelOfDetailMaxEdgeLength;
    }
  float params[4] = {1.0f/this->FramebufferObjectSize,
                     1.0f/this->FramebufferObjectSize,
                     scale,
                     0.0f};
  vtkgl::ProgramLocalParameter4fvARB(vtkgl::FRAGMENT_PROGRAM_ARB, 0, params);

  // Draw geometry
  if (this->GPUDataStructures)
    {
    glDrawElements(GL_TRIANGLES, this->LevelOfDetailTriangleCount*3,
                   GL_UNSIGNED_INT, static_cast<char *>(NULL));
    }
  else
    {
    glDrawElements(GL_TRIANGLES, this->LevelOfDetailTriangleCount*3,
                   GL_UNSIGNED_INT, this->OrderedTriangles);
    }

  vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB, 0);
  vtkgl::BindProgramARB(vtkgl::VERTEX_PROGRAM_ARB, 0);
  glDisable( vtkgl::VERTEX_PROGRAM_ARB);
}

//----------------------------------------------------------------------------
// Draw a k-1 screen-aligned planes to flush the valid entries from the
// k-buffer.
void vtkOpenGLHAVSVolumeMapper::DrawFBOFlush(int screenWidth,
                                             int screenHeight,
                                             float depthNear, float depthFar)
{
  float scale = this->MaxEdgeLength;
  if (this->LevelOfDetail || !this->PartiallyRemoveNonConvexities)
    {
    scale = this->LevelOfDetailMaxEdgeLength;
    }
  float params[4] = {1.0f/this->FramebufferObjectSize,
                     1.0f/this->FramebufferObjectSize,
                     scale,
                     0.0f};

  // Bind fragment shader
  vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB, this->FragmentProgramEnd);
  vtkgl::ProgramLocalParameter4fvARB(vtkgl::FRAGMENT_PROGRAM_ARB, 0, params);


  // Setup ortho view
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0, static_cast<GLdouble>(screenWidth), 0.0,
          static_cast<GLdouble>(screenHeight),
          static_cast<GLdouble>(depthNear), static_cast<GLdouble>(depthFar));

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Draw k-1 quads to flush k-buffer
  int flushCount = (this->KBufferSize == VTK_KBUFFER_SIZE_2)? 1 : 5;
  for (int i = 0; i < flushCount; i++)
    {
    glBegin(GL_QUADS);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, static_cast<float>(screenHeight), 0.0);
    glVertex3f(static_cast<float>(screenWidth),
               static_cast<float>(screenHeight), 0.0);
    glVertex3f(static_cast<float>(screenWidth), 0.0, 0.0);
    glEnd();
    }

  // Disable shader
  vtkgl::BindProgramARB(vtkgl::FRAGMENT_PROGRAM_ARB, 0);
  glDisable( vtkgl::FRAGMENT_PROGRAM_ARB );

  // Reset view
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  // Disable Textures
  if (this->KBufferSize == VTK_KBUFFER_SIZE_2)
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE2);
    glDisable(GL_TEXTURE_2D);
    vtkgl::ActiveTexture(vtkgl::TEXTURE3);
    glDisable(GL_TEXTURE_1D);
    }
  else
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE4);
    glDisable(GL_TEXTURE_2D);
    vtkgl::ActiveTexture(vtkgl::TEXTURE5);
    glDisable(GL_TEXTURE_1D);

    }

  vtkgl::ActiveTexture(vtkgl::TEXTURE1);
  glDisable(GL_TEXTURE_2D);
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  glDisable(GL_TEXTURE_2D);
  vtkgl::ActiveTexture(0);

  glDisable(GL_DEPTH_TEST);

  glFinish();

  // Disable vertex arrays
  if (this->GPUDataStructures)
    {
    vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER, 0);
    vtkgl::BindBuffer(vtkgl::ELEMENT_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
  else
    {
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
  this->CheckOpenGLError("Flushed FBO");
}

//----------------------------------------------------------------------------
// Blend the result from the off screen rendering into the framebuffer by
// drawing a textured screen-aligned plane.  This avoids expensive data
// transfers between GPU and CPU.
void vtkOpenGLHAVSVolumeMapper::DrawBlend(int screenWidth, int screenHeight,
                                          float depthNear, float depthFar)
{
  // Setup draw buffer
  glDrawBuffer(GL_BACK);

  // Setup 2D view
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0, screenWidth, 0.0, screenHeight, depthNear, depthFar);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Bind resulting texture
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);

  glBindTexture(GL_TEXTURE_2D, this->FramebufferTextures[0]);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  float max_u =static_cast<float>(screenWidth)/static_cast<float>(this->FramebufferObjectSize);
  float max_v = static_cast<float>(screenHeight)/static_cast<float>(this->FramebufferObjectSize);
  if (max_u > 1.0) { max_u = 1.0; }
  if (max_v > 1.0) { max_v = 1.0; }

  // Setup blending.  Use the same non-standard blending function as PT to get
  // similar images.
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // Draw textured screen-aligned plane
  glColor4f(0.0, 0.0, 0.0, 0.0);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0);
  glVertex2f(0.0, 0.0);
  glTexCoord2f(max_u, 0.0);
  glVertex2f(screenWidth, 0.0);
  glTexCoord2f(max_u, max_v);
  glVertex2f(screenWidth, screenHeight);
  glTexCoord2f(0.0, max_v);
  glVertex2f(0.0, screenHeight);
  glEnd();

  // Reset view
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  // Restore OpenGL state
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);

  CheckOpenGLError("Final Blend");
}

//----------------------------------------------------------------------------
void vtkOpenGLHAVSVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  if(this->RenderWindow!=0)
    {
    vtkOpenGLExtensionManager *extensions=
      static_cast<vtkOpenGLRenderWindow *>(this->RenderWindow.GetPointer())
      ->GetExtensionManager();
    if ( this->Initialized )
      {
      os << indent << "Supports GL_VERSION_1_3 (edge_clamp (1.2) and"
         << " multitexture (1.3) minimal version required by"
         << " GL_ARB_draw_buffers): "
         << extensions->ExtensionSupported( "GL_VERSION_1_3" );

      os << indent << "Supports GL_VERSION_2_0 (GL_ARB_draw_buffers is a core"
         << "feature): "
         << extensions->ExtensionSupported( "GL_VERSION_2_0" );

      os << indent << "Supports GL_ARB_draw_buffers: "
         << extensions->ExtensionSupported( "GL_ARB_draw_buffers" );

      os << indent << "Supports GL_EXT_framebuffer_object: "
         << extensions->ExtensionSupported( "GL_EXT_framebuffer_object" )
         << endl;
      os << indent << "Supports GL_ARB_vertex_program: "
         << extensions->ExtensionSupported( "GL_ARB_vertex_program" ) << endl;
      os << indent << "Supports GL_ARB_fragment_program: "
         << extensions->ExtensionSupported( "GL_ARB_fragment_program" ) << endl;
      os << indent << "Supports GL_ARB_texture_float"
         << extensions->ExtensionSupported( "GL_ARB_texture_float" ) << endl;
      os << indent << "Supports GL_ATI_texture_float: "
         << extensions->ExtensionSupported( "GL_ATI_texture_float" ) << endl;

      os << indent << "Supports GL_VERSION_1_5 (for optional core feature VBO): "
         << extensions->ExtensionSupported( "GL_VERSION_1_5" ) <<endl;

      os << indent << "Supports (optional) GL_ARB_vertex_buffer_object: "
         << extensions->ExtensionSupported( "GL_ARB_vertex_buffer_object" )
         <<endl;
      }
    }

  os << indent << "Framebuffer Object Size: "
     << this->FramebufferObjectSize << endl;

  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// Check the OpenGL extension manager for GPU features necessary for the
// HAVS algorithm.
bool vtkOpenGLHAVSVolumeMapper::SupportedByHardware(vtkRenderer *r)
{
  vtkOpenGLExtensionManager *extensions=
    static_cast<vtkOpenGLRenderWindow *>(r->GetRenderWindow())
    ->GetExtensionManager();

  // Temporarily filter out the Macs, as this mapper makes the ATI driver crash
  // (RogueResearch2 on VTK, ATI Radeon X1600 OpenGL Engine 2.0 ATI-1.4.56) and
  // makes the Nvidia driver render some corrupted image (kamino on ParaView3
  // dashboard NVIDIA GeForce 7300 GT 2.0 NVIDIA-1.4.56).
  // This mapper does not actually use texture3D but it is known that Macs
  // only support texture3d through OpenGL 1.2 API, not as an extension, so
  // this is a good way to filter them out.
  int iAmAMac=!extensions->ExtensionSupported("GL_EXT_texture3D");

  // OpenGL 1.3 is required by GL_ARB_draw_buffers, GL_ARB_fragment_program
  // and GL_ARB_vertex_program
  // CLAMP_TO_EGDE is core feature of OpenGL 1.2 and
  // multitexture is core feature of OpenGL 1.3.
  int supports_GL_1_3=extensions->ExtensionSupported( "GL_VERSION_1_3" );


  int supports_GL_2_0=extensions->ExtensionSupported( "GL_VERSION_2_0" );
  int supports_draw_buffers;
  if(supports_GL_2_0)
    {
    supports_draw_buffers=1;
    }
  else
    {
    supports_draw_buffers=
      extensions->ExtensionSupported( "GL_ARB_draw_buffers" );
    }

  int supports_GL_ARB_fragment_program =
    extensions->ExtensionSupported( "GL_ARB_fragment_program" );
  int supports_GL_ARB_vertex_program =
    extensions->ExtensionSupported( "GL_ARB_vertex_program" );

  int supports_GL_EXT_framebuffer_object =
    extensions->ExtensionSupported( "GL_EXT_framebuffer_object");

   // GL_ARB_texture_float or GL_ATI_texture_float introduce new tokens but
   // no new function: don't need to call LoadExtension.

  int supports_GL_ARB_texture_float =
    extensions->ExtensionSupported( "GL_ARB_texture_float" );
  int supports_GL_ATI_texture_float =
    extensions->ExtensionSupported( "GL_ATI_texture_float" );

  return !iAmAMac && supports_GL_1_3 && supports_draw_buffers &&
    supports_GL_ARB_fragment_program && supports_GL_ARB_vertex_program &&
    supports_GL_EXT_framebuffer_object &&
    ( supports_GL_ARB_texture_float || supports_GL_ATI_texture_float);
}

//----------------------------------------------------------------------------
int vtkOpenGLHAVSVolumeMapper::FillInputPortInformation(int port,
                                                        vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}
