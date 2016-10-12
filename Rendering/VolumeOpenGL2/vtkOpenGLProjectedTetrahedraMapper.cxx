/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkOpenGLProjectedTetrahedraMapper.h"

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVisibilitySort.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "vtkOpenGLError.h"

#include <cmath>
#include <algorithm>

// bring in shader code
#include "vtkglProjectedTetrahedraVS.h"
#include "vtkglProjectedTetrahedraFS.h"

static int tet_edges[6][2] = { {0,1}, {1,2}, {2,0},
                               {0,3}, {1,3}, {2,3} };

const int SqrtTableSize = 2048;

//-----------------------------------------------------------------------------
class vtkOpenGLProjectedTetrahedraMapper::vtkInternals
{
public:
  vtkInternals()
  {
    this->FrameBufferObjectId = 0;
    this->RenderBufferObjectIds[0] = 0;
    this->RenderBufferObjectIds[1] = 0;
  }
  GLuint FrameBufferObjectId;
  GLuint RenderBufferObjectIds[2];
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLProjectedTetrahedraMapper);

//-----------------------------------------------------------------------------
vtkOpenGLProjectedTetrahedraMapper::vtkOpenGLProjectedTetrahedraMapper()
{
  this->TransformedPoints = vtkFloatArray::New();
  this->Colors = vtkUnsignedCharArray::New();
  this->LastProperty = NULL;
  this->MaxCellSize = 0;
  this->GaveError = 0;
  this->SqrtTable = new float[SqrtTableSize];
  this->SqrtTableBias = 0.0;
  this->Initialized = false;
  this->CurrentFBOWidth = -1;
  this->CurrentFBOHeight = -1;
  this->FloatingPointFrameBufferResourcesAllocated = false;
  this->Internals = new vtkOpenGLProjectedTetrahedraMapper::vtkInternals;
  this->UseFloatingPointFrameBuffer = true;
  this->CanDoFloatingPointFrameBuffer = false;
  this->HasHardwareSupport = false;
  this->VBO = vtkOpenGLVertexBufferObject::New();
}

//-----------------------------------------------------------------------------
vtkOpenGLProjectedTetrahedraMapper::~vtkOpenGLProjectedTetrahedraMapper()
{
  this->ReleaseGraphicsResources(NULL);
  this->TransformedPoints->Delete();
  this->Colors->Delete();
  delete this->Internals;
  delete[] this->SqrtTable;
  this->VBO->Delete();
}

//-----------------------------------------------------------------------------
void vtkOpenGLProjectedTetrahedraMapper::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VisibilitySort: " << this->VisibilitySort << endl;
  os << indent << "UseFloatingPointFrameBuffer: "
     << (this->UseFloatingPointFrameBuffer ? "True" : "False") << endl;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLProjectedTetrahedraMapper::IsSupported(vtkRenderWindow *rwin)
{
  vtkOpenGLRenderWindow *context = vtkOpenGLRenderWindow::SafeDownCast(rwin);
  if (!context)
  {
    vtkErrorMacro(
      << "Support for " << rwin->GetClassName() << " not implemented");
    return false;
  }

  // use render to FBO when it's supported
  this->CanDoFloatingPointFrameBuffer = false;
  if (this->UseFloatingPointFrameBuffer)
  {
#if GL_ES_VERSION_2_0 != 1
    if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
    {
      this->CanDoFloatingPointFrameBuffer = true;
      return true;
    }
#endif

    this->CanDoFloatingPointFrameBuffer
#if GL_ES_VERSION_2_0 != 1
      = (glewIsSupported("GL_EXT_framebuffer_object") != 0)
        && (glewIsSupported("GL_ARB_texture_float") != 0);
#else
      = true;
#endif
    if (!this->CanDoFloatingPointFrameBuffer)
    {
      vtkWarningMacro(
        "Missing FBO support. The algorithm may produce visual artifacts.");
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkOpenGLProjectedTetrahedraMapper::Initialize(vtkRenderer *renderer)
{
  if (this->Initialized)
  {
    return;
  }

  this->Initialized = true;

  vtkOpenGLRenderWindow *renwin
    = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
  this->HasHardwareSupport = renwin != NULL && this->IsSupported(renwin);
  if (!this->HasHardwareSupport)
  {
    // this is an error since there's no fallback.
    vtkErrorMacro("The required extensions are not supported.");
  }
}

//-----------------------------------------------------------------------------
bool vtkOpenGLProjectedTetrahedraMapper::AllocateFBOResources(vtkRenderer *r)
{
  vtkOpenGLClearErrorMacro();

  int *size = r->GetSize();
  if ( this->UseFloatingPointFrameBuffer
    && this->CanDoFloatingPointFrameBuffer
    && (!this->FloatingPointFrameBufferResourcesAllocated
    || (size[0] != this->CurrentFBOWidth)
    || (size[0] != this->CurrentFBOHeight)) )
  {
    this->CurrentFBOWidth = size[0];
    this->CurrentFBOHeight = size[1];

    // reserver handles fbo and renderbuffers
    if (!this->FloatingPointFrameBufferResourcesAllocated)
    {
      glGenFramebuffers(1, &this->Internals->FrameBufferObjectId);
      vtkOpenGLCheckErrorMacro("failed at glGenFramebuffers");

      glGenRenderbuffers(2, this->Internals->RenderBufferObjectIds);
      vtkOpenGLCheckErrorMacro("failed at glGenRenderBuffers");

      this->FloatingPointFrameBufferResourcesAllocated = true;
    }

    GLint winSampleBuffers = 0;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &winSampleBuffers);

    GLint winSamples = 0;
    glGetIntegerv(GL_SAMPLES, &winSamples);

    GLint fboSampleBuffers = 0;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &fboSampleBuffers);

    int fboSamples
      = ((fboSampleBuffers >= 1)
      && (winSampleBuffers >= 1)
      && (winSamples >= 1))?winSamples:0;


    // do not special handle multisampling, use the default.
    // Multisampling is becoming less common as it
    // is replaced with other techniques
    glBindFramebuffer(GL_FRAMEBUFFER,
      this->Internals->FrameBufferObjectId);

    // allocate storage for renderbuffers
    glBindRenderbuffer(
      GL_RENDERBUFFER,
      this->Internals->RenderBufferObjectIds[0]);
    vtkOpenGLCheckErrorMacro("failed at glBindRenderBuffer color");
    glRenderbufferStorageMultisample(
      GL_RENDERBUFFER,
      fboSamples,
      GL_RGBA32F,
      this->CurrentFBOWidth,
      this->CurrentFBOHeight);
    vtkOpenGLCheckErrorMacro("failed at glRenderBufferStorage color");


    glBindRenderbuffer(
      GL_RENDERBUFFER,
      this->Internals->RenderBufferObjectIds[1]);
    vtkOpenGLCheckErrorMacro("failed at glBindRenderBuffer depth");
    glRenderbufferStorageMultisample(
      GL_RENDERBUFFER,
      fboSamples,
      GL_DEPTH_COMPONENT,
      this->CurrentFBOWidth,
      this->CurrentFBOHeight);

    // Best way to make it complete: bind the fbo for both draw+read
    // durring setup
    glBindFramebuffer(
      GL_FRAMEBUFFER,
      this->Internals->FrameBufferObjectId);

    glFramebufferRenderbuffer(
      GL_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_RENDERBUFFER,
      this->Internals->RenderBufferObjectIds[0]);
    vtkOpenGLCheckErrorMacro("failed at glFramebufferRenderBuffer for color");

    glFramebufferRenderbuffer(
      GL_FRAMEBUFFER,
      GL_DEPTH_ATTACHMENT,
      GL_RENDERBUFFER,
      this->Internals->RenderBufferObjectIds[1]);
    vtkOpenGLCheckErrorMacro("failed at glFramebufferRenderBuffer for depth");

    // verify that it is usable
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      vtkWarningMacro(
        "Missing FBO support. The algorithm may produce visual artifacts.");
      this->CanDoFloatingPointFrameBuffer = false;
      return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    this->CanDoFloatingPointFrameBuffer = true;
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkOpenGLProjectedTetrahedraMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Initialized = false;

  if (this->FloatingPointFrameBufferResourcesAllocated)
  {
      this->FloatingPointFrameBufferResourcesAllocated = false;

      glDeleteFramebuffers(1, &this->Internals->FrameBufferObjectId);
      vtkOpenGLCheckErrorMacro("failed at glDeleteFramebuffers");
      this->Internals->FrameBufferObjectId = 0;

      glDeleteRenderbuffers(2, this->Internals->RenderBufferObjectIds);
      vtkOpenGLCheckErrorMacro("failed at glDeleteRenderbuffers");
      this->Internals->RenderBufferObjectIds[0] = 0;
      this->Internals->RenderBufferObjectIds[1] = 0;
  }

  this->VBO->ReleaseGraphicsResources();
  this->Tris.ReleaseGraphicsResources(win);

  this->Superclass::ReleaseGraphicsResources(win);
}

//-----------------------------------------------------------------------------
void vtkOpenGLProjectedTetrahedraMapper::Render(vtkRenderer *renderer,
                                                vtkVolume *volume)
{
  vtkOpenGLClearErrorMacro();

  // load required extensions
  this->Initialize(renderer);

  if (!this->HasHardwareSupport)
  {
    return;
  }

  // make sure our shader program is loaded and ready to go
  vtkOpenGLRenderWindow *renWin =
    vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());

  if (renWin == NULL)
  {
    vtkErrorMacro("Invalid vtkOpenGLRenderWindow");
  }

  vtkUnstructuredGridBase *input = this->GetInput();
  vtkVolumeProperty *property = volume->GetProperty();

  // has something changed that would require us to recreate the shader?
  if (!this->Tris.Program)
  {
    // build the shader source code
    std::string VSSource = vtkglProjectedTetrahedraVS;
    std::string FSSource = vtkglProjectedTetrahedraFS;
    std::string GSSource;

    // compile and bind it if needed
    vtkShaderProgram *newShader =
      renWin->GetShaderCache()->ReadyShaderProgram(VSSource.c_str(),
                                            FSSource.c_str(),
                                            GSSource.c_str());

    // if the shader changed reinitialize the VAO
    if (newShader != this->Tris.Program)
    {
      this->Tris.Program = newShader;
      this->Tris.VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
    }

    this->Tris.ShaderSourceTime.Modified();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->Tris.Program);
  }

  // Check to see if input changed.
  if (   (this->InputAnalyzedTime < this->MTime)
      || (this->InputAnalyzedTime < input->GetMTime()) )
  {
    this->GaveError = 0;
    float max_cell_size2 = 0;

    if (input->GetNumberOfCells() == 0)
    {
      // Apparently, the input has no cells.  Just do nothing.
      return;
    }

    vtkSmartPointer<vtkCellIterator> cellIter =
        vtkSmartPointer<vtkCellIterator>::Take(input->NewCellIterator());
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
         cellIter->GoToNextCell())
    {
      vtkIdType npts = cellIter->GetNumberOfPoints();
      if (npts != 4)
      {
        if (!this->GaveError)
        {
          vtkErrorMacro("Encountered non-tetrahedra cell!");
          this->GaveError = 1;
        }
        continue;
      }
      vtkIdType *pts = cellIter->GetPointIds()->GetPointer(0);
      for (int j = 0; j < 6; j++)
      {
        double p1[3], p2[3];
        input->GetPoint(pts[tet_edges[j][0]], p1);
        input->GetPoint(pts[tet_edges[j][1]], p2);
        float size2 = (float)vtkMath::Distance2BetweenPoints(p1, p2);
        if (size2 > max_cell_size2)
        {
          max_cell_size2 = size2;
        }
      }
    }

    this->MaxCellSize = (float)sqrt(max_cell_size2);

    // Build a sqrt lookup table for measuring distances.  During perspective
    // modes we have to take a lot of square roots, and a table is much faster
    // than calling the sqrt function.
    this->SqrtTableBias = (SqrtTableSize-1)/max_cell_size2;
    for (int i = 0; i < SqrtTableSize; i++)
    {
      this->SqrtTable[i] = (float)sqrt(i/this->SqrtTableBias);
    }

    this->InputAnalyzedTime.Modified();
  }

  if (renderer->GetRenderWindow()->CheckAbortStatus() || this->GaveError)
  {
    vtkOpenGLCheckErrorMacro("failed during Render");
    return;
  }

  if (renderer->GetRenderWindow()->CheckAbortStatus())
  {
    vtkOpenGLCheckErrorMacro("failed during Render");
    return;
  }

  // Check to see if we need to remap colors.
  if (   (this->ColorsMappedTime < this->MTime)
      || (this->ColorsMappedTime < input->GetMTime())
      || (this->LastProperty != property)
      || (this->ColorsMappedTime < property->GetMTime()) )
  {
    vtkDataArray *scalars = this->GetScalars(input, this->ScalarMode,
                                             this->ArrayAccessMode,
                                             this->ArrayId, this->ArrayName,
                                             this->UsingCellColors);
    if (!scalars)
    {
      vtkErrorMacro(<< "Can't use projected tetrahedra without scalars!");
      vtkOpenGLCheckErrorMacro("failed during Render");
      return;
    }

    vtkProjectedTetrahedraMapper::MapScalarsToColors(this->Colors, property,
                                                     scalars);

    this->ColorsMappedTime.Modified();
    this->LastProperty = property;
  }
  if (renderer->GetRenderWindow()->CheckAbortStatus())
  {
    vtkOpenGLCheckErrorMacro("failed during Render");
    return;
  }

  this->Timer->StartTimer();

  this->ProjectTetrahedra(renderer, volume, renWin);

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();
  vtkOpenGLCheckErrorMacro("failed after Render");
}

//-----------------------------------------------------------------------------

inline float vtkOpenGLProjectedTetrahedraMapper::GetCorrectedDepth(
                                         float x, float y, float z1, float z2,
                                         const float inverse_projection_mat[16],
                                         int use_linear_depth_correction,
                                         float linear_depth_correction)
{
  if (use_linear_depth_correction)
  {
    float depth = linear_depth_correction*(z1 - z2);
    if (depth < 0) depth = -depth;
    return depth;
  }
  else
  {
    float eye1[3], eye2[3], invw;

    // This code does the same as the commented code above, but also collects
    // common arithmetic between the two matrix x vector operations.  An
    // optimizing compiler may or may not pick up on that.
    float common[4];

    common[0] = (  inverse_projection_mat[ 0]*x
                 + inverse_projection_mat[ 4]*y
                 + inverse_projection_mat[12] );
    common[1] = (  inverse_projection_mat[ 1]*x
                 + inverse_projection_mat[ 5]*y
                 + inverse_projection_mat[13] );
    common[2] = (  inverse_projection_mat[ 2]*x
                 + inverse_projection_mat[ 6]*y
                 + inverse_projection_mat[10]*z1
                 + inverse_projection_mat[14] );
    common[3] = (  inverse_projection_mat[ 3]*x
                 + inverse_projection_mat[ 7]*y
                 + inverse_projection_mat[15] );

    invw = 1/(common[3] + inverse_projection_mat[11]*z1);
    eye1[0] = invw*(common[0] + inverse_projection_mat[ 8]*z1);
    eye1[1] = invw*(common[1] + inverse_projection_mat[ 9]*z1);
    eye1[2] = invw*(common[2] + inverse_projection_mat[10]*z1);

    invw = 1/(common[3] + inverse_projection_mat[11]*z2);
    eye2[0] = invw*(common[0] + inverse_projection_mat[ 8]*z2);
    eye2[1] = invw*(common[1] + inverse_projection_mat[ 9]*z2);
    eye2[2] = invw*(common[2] + inverse_projection_mat[10]*z2);

    float dist2 = vtkMath::Distance2BetweenPoints(eye1, eye2);
    return this->SqrtTable[(int)(dist2*this->SqrtTableBias)];
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLProjectedTetrahedraMapper::ProjectTetrahedra(vtkRenderer *renderer,
  vtkVolume *volume, vtkOpenGLRenderWindow* renWin)
{
  vtkOpenGLClearErrorMacro();
  unsigned int const defaultFBO = renWin->GetFrameBufferObject();

  // after mucking about with FBO bindings be sure
  // we're saving the default fbo attributes/blend function
  this->AllocateFBOResources(renderer);

  if (this->UseFloatingPointFrameBuffer
    && this->CanDoFloatingPointFrameBuffer)
  {
    // bind draw+read to set it up
    glBindFramebuffer(GL_FRAMEBUFFER,
          this->Internals->FrameBufferObjectId);

    glReadBuffer(GL_NONE);
    GLenum dbuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &dbuf);

    GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (status!= GL_FRAMEBUFFER_COMPLETE)
    {
      vtkErrorMacro("FBO is incomplete " << status);
    }

    // read from default
    glBindFramebuffer(GL_READ_FRAMEBUFFER, defaultFBO);

    // draw to fbo
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                           this->Internals->FrameBufferObjectId);

    glBlitFramebuffer(0, 0,
                      this->CurrentFBOWidth, this->CurrentFBOHeight,
                      0, 0,
                      this->CurrentFBOWidth, this->CurrentFBOHeight,
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                      GL_NEAREST);

    vtkOpenGLCheckErrorMacro("failed at glBlitFramebuffer");
  }

  // TODO:
  // There are some caching optimizations that could be used
  // here to skip various expensive operations (eg sorting
  // cells could be skipped if input data and MVP matrices
  // haven't changed).

  vtkUnstructuredGridBase *input = this->GetInput();
  this->VisibilitySort->SetInput(input);
  this->VisibilitySort->SetDirectionToBackToFront();
  this->VisibilitySort->SetModelTransform(volume->GetMatrix());
  this->VisibilitySort->SetCamera(renderer->GetActiveCamera());
  this->VisibilitySort->SetMaxCellsReturned(1000);

  this->VisibilitySort->InitTraversal();

  if (renderer->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }

  vtkMatrix4x4 *wcdc;
  vtkMatrix4x4 *wcvc;
  vtkMatrix3x3 *norms;
  vtkMatrix4x4 *vcdc;
  vtkOpenGLCamera *cam = (vtkOpenGLCamera *)(renderer->GetActiveCamera());
  cam->GetKeyMatrices(renderer,wcvc,norms,vcdc,wcdc);
  float projection_mat[16];
  for(int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      projection_mat[i*4+j] = vcdc->GetElement(i, j);
    }
  }

  float modelview_mat[16];
  if (!volume->GetIsIdentity())
  {
    vtkMatrix4x4 *tmpMat = vtkMatrix4x4::New();
    vtkMatrix4x4 *tmpMat2 = vtkMatrix4x4::New();
    vtkMatrix4x4 *mcwc = volume->GetMatrix();
    tmpMat2->DeepCopy(wcvc);
    tmpMat2->Transpose();
    vtkMatrix4x4::Multiply4x4(tmpMat2, mcwc, tmpMat);
    tmpMat->Transpose();
    for(int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        modelview_mat[i*4+j] = tmpMat->GetElement(i, j);
      }
    }
    tmpMat->Delete();
    tmpMat2->Delete();
  }
  else
  {
    for(int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        modelview_mat[i*4+j] = wcvc->GetElement(i, j);
      }
    }
  }

  // Get the inverse projection matrix so that we can convert distances in
  // clipping space to distances in world or eye space.
  float inverse_projection_mat[16];
  float linear_depth_correction = 1;
  int use_linear_depth_correction;
  double tmp_mat[16];

  // VTK's matrix functions use doubles.
  std::copy(projection_mat, projection_mat+16, tmp_mat);
  // VTK and OpenGL store their matrices differently.  Correct.
  vtkMatrix4x4::Transpose(tmp_mat, tmp_mat);
  // Take the inverse.
  vtkMatrix4x4::Invert(tmp_mat, tmp_mat);
  // Restore back to OpenGL form.
  vtkMatrix4x4::Transpose(tmp_mat, tmp_mat);
  // Copy back to float for faster computation.
  std::copy(tmp_mat, tmp_mat+16, inverse_projection_mat);

  // Check to see if we can just do a linear depth correction from clipping
  // space to eye space.
  use_linear_depth_correction = (   (projection_mat[ 3] == 0.0)
                                 && (projection_mat[ 7] == 0.0)
                                 && (projection_mat[11] == 0.0)
                                 && (projection_mat[15] == 1.0) );
  if (use_linear_depth_correction)
  {
    float pos1[3], *pos2;

    pos1[0] = inverse_projection_mat[8] + inverse_projection_mat[12];
    pos1[1] = inverse_projection_mat[9] + inverse_projection_mat[13];
    pos1[2] = inverse_projection_mat[10] + inverse_projection_mat[14];

    pos2 = inverse_projection_mat + 12;

    linear_depth_correction = sqrt(vtkMath::Distance2BetweenPoints(pos1, pos2));
  }
  // Transform all the points.
  vtkProjectedTetrahedraMapper::TransformPoints(input->GetPoints(),
                                                projection_mat, modelview_mat,
                                                this->TransformedPoints);
  float *points = this->TransformedPoints->GetPointer(0);

  if (renderer->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }

  glDepthMask(GL_FALSE);

  glDisable(GL_CULL_FACE);

  GLint blendSrcA = GL_ONE;
  GLint blendDstA = GL_ONE_MINUS_SRC_ALPHA;
  GLint blendSrcC = GL_SRC_ALPHA;
  GLint blendDstC = GL_ONE_MINUS_SRC_ALPHA;
  glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcA);
  glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstA);
  glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcC);
  glGetIntegerv(GL_BLEND_DST_RGB, &blendDstC);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
    GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  float unit_distance = volume->GetProperty()->GetScalarOpacityUnitDistance();

  // build the VBO and IBOs,  we so these in chuncks as based on
  // the settings of the VisibilitySort tclass

  this->VBO->VertexOffset = 0;
  this->VBO->NormalOffset = 0;
  this->VBO->ColorOffset = 3*sizeof(float);
  this->VBO->ColorComponents = 3;
  this->VBO->TCoordOffset = 4*sizeof(float);
  this->VBO->TCoordComponents = 2;
  this->VBO->Stride = 6*sizeof(float);

  // Establish vertex arrays.
  // tets have 4 points, 5th point here is used
  // to insert a point in case of intersections
  float tet_points[5*3] = {0.0f};
  unsigned char tet_colors[5*3] = {0};
  float tet_texcoords[5*2] = {0.0f};

  unsigned char *colors = this->Colors->GetPointer(0);
  vtkIdType totalnumcells = input->GetNumberOfCells();
  vtkIdType numcellsrendered = 0;
  vtkNew<vtkIdList> cellPointIds;

  std::vector<float> packedVBO;
  packedVBO.reserve(6 * 5 * this->VisibilitySort->GetMaxCellsReturned());

  std::vector<unsigned int> indexArray;
  indexArray.reserve(3 * 4 * this->VisibilitySort->GetMaxCellsReturned());

  // Let's do it!
  for (vtkIdTypeArray *sorted_cell_ids = this->VisibilitySort->GetNextCells();
       sorted_cell_ids != NULL;
       sorted_cell_ids = this->VisibilitySort->GetNextCells())
  {
    this->UpdateProgress((double)numcellsrendered/totalnumcells);

    if (renderer->GetRenderWindow()->CheckAbortStatus())
    {
      break;
    }
    vtkIdType *cell_ids = sorted_cell_ids->GetPointer(0);
    vtkIdType num_cell_ids = sorted_cell_ids->GetNumberOfTuples();

    packedVBO.resize(6 * 5 * num_cell_ids);
    std::vector<float>::iterator it = packedVBO.begin();
    int numPts = 0;
    indexArray.resize(0);

    for (vtkIdType i = 0; i < num_cell_ids; i++)
    {
      vtkIdType cell = cell_ids[i];
      input->GetCellPoints(cell, cellPointIds.GetPointer());
      int j;

      // Get the data for the tetrahedra.
      for (j = 0; j < 4; j++)
      {
        // Assuming we only have tetrahedra, each entry in cells has 5
        // components.
        const float *p = points + 3 * cellPointIds->GetId(j);
        tet_points[j*3 + 0] = p[0];
        tet_points[j*3 + 1] = p[1];
        tet_points[j*3 + 2] = p[2];

        const unsigned char *c;
        if (this->UsingCellColors)
        {
          c = colors + 4*cell;
        }
        else
        {
          c = colors + 4 * cellPointIds->GetId(j);
        }

        tet_colors[j*3 + 0] = c[0];
        tet_colors[j*3 + 1] = c[1];
        tet_colors[j*3 + 2] = c[2];

        tet_texcoords[j*2 + 0] = static_cast<float>(c[3])/255.0f;
        tet_texcoords[j*2 + 1] = 0;
      }

      // Do not render this cell if it is outside of the cutting planes.  For
      // most planes, cut if all points are outside.  For the near plane, cut if
      // any points are outside because things can go very wrong if one of the
      // points is behind the view.
      if (   (   (tet_points[0*3+0] >  1.0f) && (tet_points[1*3+0] >  1.0f)
              && (tet_points[2*3+0] >  1.0f) && (tet_points[3*3+0] >  1.0f) )
          || (   (tet_points[0*3+0] < -1.0f) && (tet_points[1*3+0] < -1.0f)
              && (tet_points[2*3+0] < -1.0f) && (tet_points[3*3+0] < -1.0f) )
          || (   (tet_points[0*3+1] >  1.0f) && (tet_points[1*3+1] >  1.0f)
              && (tet_points[2*3+1] >  1.0f) && (tet_points[3*3+1] >  1.0f) )
          || (   (tet_points[0*3+1] < -1.0f) && (tet_points[1*3+1] < -1.0f)
              && (tet_points[2*3+1] < -1.0f) && (tet_points[3*3+1] < -1.0f) )
          || (   (tet_points[0*3+2] >  1.0f) && (tet_points[1*3+2] >  1.0f)
              && (tet_points[2*3+2] >  1.0f) && (tet_points[3*3+2] >  1.0f) )
          || (   (tet_points[0*3+2] < -1.0f) || (tet_points[1*3+2] < -1.0f)
              || (tet_points[2*3+2] < -1.0f) || (tet_points[3*3+2] < -1.0f) ) )
      {
        continue;
      }

      // The classic PT algorithm uses face normals to determine the
      // projection class and then do calculations individually.  However,
      // Wylie 2002 shows how to use the intersection of two segments to
      // calculate the depth of the thick part for any case.  Here, we use
      // face normals to determine which segments to use.  One segment
      // should be between two faces that are either both front facing or
      // back facing.  Obviously, we only need to test three faces to find
      // two such faces.  We test the three faces connected to point 0.
      vtkIdType segment1[2];
      vtkIdType segment2[2];

      float v1[2], v2[2], v3[3];
      v1[0] = tet_points[1*3 + 0] - tet_points[0*3 + 0];
      v1[1] = tet_points[1*3 + 1] - tet_points[0*3 + 1];
      v2[0] = tet_points[2*3 + 0] - tet_points[0*3 + 0];
      v2[1] = tet_points[2*3 + 1] - tet_points[0*3 + 1];
      v3[0] = tet_points[3*3 + 0] - tet_points[0*3 + 0];
      v3[1] = tet_points[3*3 + 1] - tet_points[0*3 + 1];

      float face_dir1 = v3[0]*v2[1] - v3[1]*v2[0];
      float face_dir2 = v1[0]*v3[1] - v1[1]*v3[0];
      float face_dir3 = v2[0]*v1[1] - v2[1]*v1[0];

      if (   (face_dir1 * face_dir2 >= 0)
          && (   (face_dir1 != 0)       // Handle a special case where 2 faces
              || (face_dir2 != 0) ) )   // are perpendicular to the view plane.
      {
        segment1[0] = 0;  segment1[1] = 3;
        segment2[0] = 1;  segment2[1] = 2;
      }
      else if (face_dir1 * face_dir3 >= 0)
      {
        segment1[0] = 0;  segment1[1] = 2;
        segment2[0] = 1;  segment2[1] = 3;
      }
      else      // Unless the tet is degenerate, face_dir2*face_dir3 >= 0
      {
        segment1[0] = 0;  segment1[1] = 1;
        segment2[0] = 2;  segment2[1] = 3;
      }

#define VEC3SUB(Z,X,Y)          \
  (Z)[0] = (X)[0] - (Y)[0];     \
  (Z)[1] = (X)[1] - (Y)[1];     \
  (Z)[2] = (X)[2] - (Y)[2];
#define P1 (tet_points + 3*segment1[0])
#define P2 (tet_points + 3*segment1[1])
#define P3 (tet_points + 3*segment2[0])
#define P4 (tet_points + 3*segment2[1])
#define C1 (tet_colors + 3*segment1[0])
#define C2 (tet_colors + 3*segment1[1])
#define C3 (tet_colors + 3*segment2[0])
#define C4 (tet_colors + 3*segment2[1])
#define T1 (tet_texcoords + 2*segment1[0])
#define T2 (tet_texcoords + 2*segment1[1])
#define T3 (tet_texcoords + 2*segment2[0])
#define T4 (tet_texcoords + 2*segment2[1])
      // Find the intersection of the projection of the two segments in the
      // XY plane.  This algorithm is based on that given in Graphics Gems
      // III, pg. 199-202.
      float A[3], B[3], C[3];
      // We can define the two lines parametrically as:
      //        P1 + alpha(A)
      //        P3 + beta(B)
      // where A = P2 - P1
      // and   B = P4 - P3.
      // alpha and beta are in the range [0,1] within the line segment.
      VEC3SUB(A, P2, P1);
      VEC3SUB(B, P4, P3);
      // The lines intersect when the values of the two parameteric equations
      // are equal.  Setting them equal and moving everything to one side:
      //        0 = C + beta(B) - alpha(A)
      // where C = P3 - P1.
      VEC3SUB(C, P3, P1);
      // When we project the lines to the xy plane (which we do by throwing
      // away the z value), we have two equations and two unkowns.  The
      // following are the solutions for alpha and beta.
      float denominator = (A[0]*B[1]-A[1]*B[0]);
      if (denominator == 0) continue;   // Must be degenerated tetrahedra.
      float alpha = (B[1]*C[0]-B[0]*C[1])/denominator;
      float beta = (A[1]*C[0]-A[0]*C[1])/denominator;

      if ((alpha >= 0) && (alpha <= 1))
      {
        // The two segments intersect.  This corresponds to class 2 in
        // Shirley and Tuchman (or one of the degenerate cases).

        // Make new point at intersection.
        tet_points[3*4 + 0] = P1[0] + alpha*A[0];
        tet_points[3*4 + 1] = P1[1] + alpha*A[1];
        tet_points[3*4 + 2] = P1[2] + alpha*A[2];

        // Find depth at intersection.
        float depth = this->GetCorrectedDepth(
              tet_points[3*4 + 0],
              tet_points[3*4 + 1],
              tet_points[3*4 + 2],
              P3[2] + beta*B[2],
              inverse_projection_mat,
              use_linear_depth_correction,
              linear_depth_correction);

        // Find color at intersection.
        tet_colors[3*4 + 0] = static_cast<unsigned char>
              (0.5f*(C1[0] + alpha*(C2[0]-C1[0])
              + C3[0] + beta*(C4[0]-C3[0])));

        tet_colors[3*4 + 1] = static_cast<unsigned char>
              (0.5f*(C1[1] + alpha*(C2[1]-C1[1])
              + C3[1] + beta*(C4[1]-C3[1])));

        tet_colors[3*4 + 2] = static_cast<unsigned char>
              (0.5f*(C1[2] + alpha*(C2[2]-C1[2])
              + C3[2] + beta*(C4[2]-C3[2])));

//         tet_colors[3*0 + 0] = 255;
//         tet_colors[3*0 + 1] = 0;
//         tet_colors[3*0 + 2] = 0;
//         tet_colors[3*1 + 0] = 255;
//         tet_colors[3*1 + 1] = 0;
//         tet_colors[3*1 + 2] = 0;
//         tet_colors[3*2 + 0] = 255;
//         tet_colors[3*2 + 1] = 0;
//         tet_colors[3*2 + 2] = 0;
//         tet_colors[3*3 + 0] = 255;
//         tet_colors[3*3 + 1] = 0;
//         tet_colors[3*3 + 2] = 0;
//         tet_colors[3*4 + 0] = 255;
//         tet_colors[3*4 + 1] = 0;
//         tet_colors[3*4 + 2] = 0;

        // Find the opacity at intersection.
        tet_texcoords[2*4 + 0] = 0.5f*(  T1[0] + alpha*(T2[0]-T1[0])
                                       + T3[0] + alpha*(T4[0]-T3[0]));

        // Record the depth at the intersection.
        tet_texcoords[2*4 + 1] = depth/unit_distance;

        // Establish the order in which the points should be rendered.
        unsigned char indices[6];
        indices[0] = 4;
        indices[1] = segment1[0];
        indices[2] = segment2[0];
        indices[3] = segment1[1];
        indices[4] = segment2[1];
        indices[5] = segment1[0];
        // add the cells to the IBO
        for (int cellIdx = 0; cellIdx < 4; cellIdx++)
        {
          indexArray.push_back(indices[0]+numPts);
          indexArray.push_back(indices[cellIdx+1]+numPts);
          indexArray.push_back(indices[cellIdx+2]+numPts);
        }
      }
      else
      {
        // The two segments do not intersect.  This corresponds to class 1
        // in Shirley and Tuchman.
        if (alpha <= 0)
        {
          // Flip segment1 so that alpha is >= 1.  P1 and P2 are also
          // flipped as are C1-C2 and T1-T2.  Note that this will
          // invalidate A.  B and beta are unaffected.
          std::swap(segment1[0], segment1[1]);
          alpha = 1 - alpha;
        }
        // From here on, we can assume P2 is the "thick" point.

        // Find the depth under the thick point.  Use the alpha and beta
        // from intersection to determine location of face under thick
        // point.
        float edgez = P3[2] + beta*B[2];
        float pointz = P1[2];
        float facez = (edgez + (alpha-1)*pointz)/alpha;
        float depth = GetCorrectedDepth(P2[0], P2[1], P2[2], facez,
                                        inverse_projection_mat,
                                        use_linear_depth_correction,
                                        linear_depth_correction);

        // Fix color at thick point.  Average color with color of opposite
        // face.
        for (j = 0; j < 3; j++)
        {
          float edgec = C3[j] + beta*(C4[j]-C3[j]);
          float pointc = C1[j];
          float facec = (edgec + (alpha-1)*pointc)/alpha;
          C2[j] = (unsigned char)(0.5f*(facec + C2[j]));
        }

//         tet_colors[3*segment1[0] + 0] = 0;
//         tet_colors[3*segment1[0] + 1] = 255;
//         tet_colors[3*segment1[0] + 2] = 0;
//         tet_colors[3*segment1[1] + 0] = 0;
//         tet_colors[3*segment1[1] + 1] = 255;
//         tet_colors[3*segment1[1] + 2] = 0;
//         tet_colors[3*segment2[0] + 0] = 0;
//         tet_colors[3*segment2[0] + 1] = 255;
//         tet_colors[3*segment2[0] + 2] = 0;
//         tet_colors[3*segment2[1] + 0] = 0;
//         tet_colors[3*segment2[1] + 1] = 255;
//         tet_colors[3*segment2[1] + 2] = 0;

        // Fix opacity at thick point.  Average opacity with opacity of
        // opposite face.
        float edgea = T3[0] + beta*(T4[0]-T3[0]);
        float pointa = T1[0];
        float facea = (edgea + (alpha-1)*pointa)/alpha;
        T2[0] = 0.5f*(facea + T2[0]);

        // Record thickness at thick point.
        T2[1] = depth/unit_distance;

        // Establish the order in which the points should be rendered.
        unsigned char indices[5];
        indices[0] = segment1[1];
        indices[1] = segment1[0];
        indices[2] = segment2[0];
        indices[3] = segment2[1];
        indices[4] = segment1[0];

        // add the cells to the IBO
        for (int cellIdx = 0; cellIdx < 3; cellIdx++)
        {
          indexArray.push_back(indices[0]+numPts);
          indexArray.push_back(indices[cellIdx+1]+numPts);
          indexArray.push_back(indices[cellIdx+2]+numPts);
        }
      }

      // add the points to the VBO
      union { unsigned char c[4]; float f; } v = { { 0, 0, 0, 255 } };
      for (int ptIdx = 0; ptIdx < 5; ptIdx++)
      {
        *(it++) = tet_points[ptIdx*3];
        *(it++) = tet_points[ptIdx*3+1];
        *(it++) = tet_points[ptIdx*3+2];
        v.c[0] = tet_colors[ptIdx*3];
        v.c[1] = tet_colors[ptIdx*3+1];
        v.c[2] = tet_colors[ptIdx*3+2];
        *(it++) = v.f;
        *(it++) = tet_texcoords[ptIdx*2]; // attenuation
        *(it++) = tet_texcoords[ptIdx*2+1]; // depth
      }
      numPts += 5;
    }

    this->VBO->Upload(packedVBO, vtkOpenGLBufferObject::ArrayBuffer);
    this->VBO->VertexCount = numPts;
    this->VBO->Bind();

    this->Tris.VAO->Bind();
    if (this->Tris.IBO->IndexCount && (
        this->Tris.ShaderSourceTime > this->Tris.AttributeUpdateTime))
    {
      if (!this->Tris.VAO->AddAttributeArray(this->Tris.Program, this->VBO,
                                      "vertexDC", this->VBO->VertexOffset,
                                      this->VBO->Stride, VTK_FLOAT, 3, false))
      {
        vtkErrorMacro(<< "Error setting 'vertexDC' in shader VAO.");
      }
      if (!this->Tris.VAO->AddAttributeArray(this->Tris.Program, this->VBO,
                                      "scalarColor", this->VBO->ColorOffset,
                                      this->VBO->Stride, VTK_UNSIGNED_CHAR,
                                      this->VBO->ColorComponents, true))
      {
        vtkErrorMacro(<< "Error setting 'scalarColor' in shader VAO.");
      }
      if (!this->Tris.VAO->AddAttributeArray(this->Tris.Program, this->VBO,
                                      "attenuationArray", this->VBO->TCoordOffset,
                                      this->VBO->Stride, VTK_FLOAT,
                                      1, false))
      {
        vtkErrorMacro(<< "Error setting attenuation in shader VAO.");
      }
      if (!this->Tris.VAO->AddAttributeArray(this->Tris.Program, this->VBO,
                                      "depthArray", this->VBO->TCoordOffset+sizeof(float),
                                      this->VBO->Stride, VTK_FLOAT,
                                      1, false))
      {
        vtkErrorMacro(<< "Error setting depth in shader VAO.");
      }
      this->Tris.AttributeUpdateTime.Modified();
    }

    this->Tris.IBO->Upload(indexArray, vtkOpenGLBufferObject::ElementArrayBuffer);
    this->Tris.IBO->IndexCount = indexArray.size();
    this->Tris.IBO->Bind();
    glDrawRangeElements(GL_TRIANGLES, 0,
                        static_cast<GLuint>(this->VBO->VertexCount - 1),
                        static_cast<GLsizei>(this->Tris.IBO->IndexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Tris.IBO->Release();
    this->Tris.VAO->Release();
    this->VBO->Release();
    numcellsrendered += num_cell_ids;
  }

  if (this->UseFloatingPointFrameBuffer
    && this->CanDoFloatingPointFrameBuffer)
  {
    // copy from our fbo to the default one
    glBindFramebuffer(GL_FRAMEBUFFER,
          this->Internals->FrameBufferObjectId);

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
      vtkErrorMacro("FBO is incomplete " << status);
    }

    // read from fbo
    glBindFramebuffer(GL_READ_FRAMEBUFFER,
                      this->Internals->FrameBufferObjectId);
    // draw to default fbo
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFBO);

    glBlitFramebuffer(0, 0, this->CurrentFBOWidth, this->CurrentFBOHeight,
                      0, 0, this->CurrentFBOWidth, this->CurrentFBOHeight,
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    vtkOpenGLCheckErrorMacro("failed at glBlitFramebuffer");

    // restore default fbo for both read+draw
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
  }

  // Restore the blend function.
  vtkOpenGLCheckErrorMacro("failed at glPopAttrib");

  glDepthMask(GL_TRUE);
  glBlendFuncSeparate(blendSrcC, blendDstC, blendSrcA, blendDstA);

  vtkOpenGLCheckErrorMacro("failed after ProjectTetrahedra");
  this->UpdateProgress(1.0);
}
