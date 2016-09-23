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
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVisibilitySort.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "vtkOpenGL.h"
#include "vtkOpenGLError.h"

#include "vtkgl.h"

#include <cmath>
#include <algorithm>

// static int tet_faces[4][3] = { {1,2,3}, {2,0,3}, {0,1,3}, {0,2,1} };
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
    this->OpacityTexture = 0;
  }
  GLuint FrameBufferObjectId;
  GLuint RenderBufferObjectIds[2];
  GLuint OpacityTexture;
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
}

//-----------------------------------------------------------------------------
vtkOpenGLProjectedTetrahedraMapper::~vtkOpenGLProjectedTetrahedraMapper()
{
  this->ReleaseGraphicsResources(NULL);
  this->TransformedPoints->Delete();
  this->Colors->Delete();
  delete this->Internals;
  delete[] this->SqrtTable;
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

  vtkOpenGLExtensionManager *extensions = context->GetExtensionManager();
  bool texSupport
    = (extensions->ExtensionSupported("GL_VERSION_1_3") != 0) ||
      (extensions->ExtensionSupported("GL_ARB_multitexture") != 0);

  // use render to FBO when it's supported
  this->CanDoFloatingPointFrameBuffer = false;
  if (this->UseFloatingPointFrameBuffer)
  {
    this->CanDoFloatingPointFrameBuffer
      = (extensions->ExtensionSupported("GL_ARB_framebuffer_object") != 0)
      && (extensions->ExtensionSupported("GL_ARB_draw_buffers") != 0)
      && (extensions->ExtensionSupported("GL_ARB_texture_float") != 0);
#ifndef NDEBUG
    if (!this->CanDoFloatingPointFrameBuffer)
    {
      vtkWarningMacro(
        "Missing FBO support. The algorithm may produce visual artifacts.");
    }
#endif
  }

  // exclude ATI Radeon HD, except on Apple, because there seems to
  // be a bug in ATI's legacy fixed function texturing support in recent
  // drivers. The Radeon HD cards are identified here by the OpenGL version
  // because the renderer string is inconsistent across platforms.
  // win7 RHD5800, win7 RHD6750M, Linux RHD7870 all exhibit the bug.
  bool driverSupport
  #if defined(__APPLE__)
    = true;
  #else
    = !(extensions->DriverIsATI()
    && (extensions->GetDriverGLVersionMajor() >= 3))
    || extensions->GetIgnoreDriverBugs("ATI texturing bug");
  #endif

  return texSupport && driverSupport;
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

  // load required extensions
  vtkOpenGLExtensionManager *extensions = renwin->GetExtensionManager();
  if (extensions->LoadSupportedExtension("GL_VERSION_1_3") == 0) // for multitexture
  {
    extensions->LoadCorePromotedExtension("GL_ARB_multitexture");
  }
  // used GL_ARB_texture_float but nothing to load for it
  if ( this->UseFloatingPointFrameBuffer
     && this->CanDoFloatingPointFrameBuffer)
  {
    extensions->LoadExtension("GL_ARB_framebuffer_object");
    extensions->LoadExtension("GL_ARB_draw_buffers");
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
      vtkgl::GenFramebuffers(1, &this->Internals->FrameBufferObjectId);
      vtkOpenGLCheckErrorMacro("failed at glGenFramebuffers");

      vtkgl::GenRenderbuffers(2, this->Internals->RenderBufferObjectIds);
      vtkOpenGLCheckErrorMacro("failed at glGenRenderBuffers");

      this->FloatingPointFrameBufferResourcesAllocated = true;
    }
    // handle multisampling
    // the ARB says if SAMPLE_BUFFERS is greater than 1
    // on both READ and DRAW FBO then SAMPLES has to match.
    // but if either have SAMPLE_BUFFERS zero then conversions
    // are made.
    vtkgl::BindFramebuffer(vtkgl::FRAMEBUFFER, 0);

    GLint winSampleBuffers = 0;
    glGetIntegerv(vtkgl::SAMPLE_BUFFERS, &winSampleBuffers);

    GLint winSamples = 0;
    glGetIntegerv(vtkgl::SAMPLES, &winSamples);

    vtkgl::BindFramebuffer(
          vtkgl::FRAMEBUFFER,
          this->Internals->FrameBufferObjectId);

    GLint fboSampleBuffers = 0;
    glGetIntegerv(vtkgl::SAMPLE_BUFFERS, &fboSampleBuffers);

    vtkDebugMacro(
      << "mutisample enabled "
      << (glIsEnabled(vtkgl::MULTISAMPLE)?"yes":"no")
      << " winSampleBuffers=" << winSampleBuffers
      << " winSamples=" << winSamples
      << " fboSampleBuffers=" << fboSampleBuffers);

    int fboSamples
      = ((fboSampleBuffers >= 1)
      && (winSampleBuffers >= 1)
      && (winSamples >= 1))?winSamples:0;

    // allocate storage for renderbuffers
    vtkgl::BindRenderbuffer(
          vtkgl::RENDERBUFFER,
          this->Internals->RenderBufferObjectIds[0]);
    vtkOpenGLCheckErrorMacro("failed at glBindRenderBuffer color");

    vtkgl::RenderbufferStorageMultisample(
          vtkgl::RENDERBUFFER,
          fboSamples,
          vtkgl::RGBA32F_ARB,
          this->CurrentFBOWidth,
          this->CurrentFBOHeight);
    vtkOpenGLCheckErrorMacro("failed at glRenderBufferStorage color");

    vtkgl::BindRenderbuffer(
          vtkgl::RENDERBUFFER,
          this->Internals->RenderBufferObjectIds[1]);
    vtkOpenGLCheckErrorMacro("failed at glBindRenderBuffer depth");

    vtkgl::RenderbufferStorageMultisample(
          vtkgl::RENDERBUFFER,
          fboSamples,
          GL_DEPTH_COMPONENT,
          this->CurrentFBOWidth,
          this->CurrentFBOHeight);
    vtkOpenGLCheckErrorMacro("failed at glRenderBufferStorage depth");

   // best way to make it complete: bind the fbo for both draw+read
   // durring setup
   vtkgl::BindFramebuffer(
         vtkgl::FRAMEBUFFER,
         this->Internals->FrameBufferObjectId);
   vtkOpenGLCheckErrorMacro("failed at glBindFramebuffer");

   vtkgl::FramebufferRenderbuffer(
         vtkgl::FRAMEBUFFER,
         vtkgl::COLOR_ATTACHMENT0,
         vtkgl::RENDERBUFFER,
         this->Internals->RenderBufferObjectIds[0]);
    vtkOpenGLCheckErrorMacro("failed at glFramebufferRenderBuffer for color");

    vtkgl::FramebufferRenderbuffer(
          vtkgl::FRAMEBUFFER,
          vtkgl::DEPTH_ATTACHMENT,
          vtkgl::RENDERBUFFER,
          this->Internals->RenderBufferObjectIds[1]);
    vtkOpenGLCheckErrorMacro("failed at glFramebufferRenderBuffer for depth");

    // verify that it is usable
    GLenum status = vtkgl::CheckFramebufferStatus(vtkgl::FRAMEBUFFER);
    if(status != vtkgl::FRAMEBUFFER_COMPLETE)
    {
      vtkgl::BindFramebuffer(vtkgl::FRAMEBUFFER, 0);
      vtkWarningMacro(
        "Missing FBO support. The algorithm may produce visual artifacts.");
      this->CanDoFloatingPointFrameBuffer = false;
      return false;
    }
    vtkgl::BindFramebuffer(vtkgl::FRAMEBUFFER, 0);
    this->CanDoFloatingPointFrameBuffer = true;
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkOpenGLProjectedTetrahedraMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Initialized = false;

  if (this->Internals->OpacityTexture)
  {
    glDeleteTextures(1, &this->Internals->OpacityTexture);
    vtkOpenGLCheckErrorMacro("failed at glDeleteTextures");
    this->Internals->OpacityTexture = 0;
  }

  if (this->FloatingPointFrameBufferResourcesAllocated)
  {
      this->FloatingPointFrameBufferResourcesAllocated = false;

      vtkgl::DeleteFramebuffers(1, &this->Internals->FrameBufferObjectId);
      vtkOpenGLCheckErrorMacro("failed at glDeleteFramebuffers");
      this->Internals->FrameBufferObjectId = 0;

      vtkgl::DeleteRenderbuffers(2, this->Internals->RenderBufferObjectIds);
      vtkOpenGLCheckErrorMacro("failed at glDeleteRenderbuffers");
      this->Internals->RenderBufferObjectIds[0] = 0;
      this->Internals->RenderBufferObjectIds[1] = 0;
  }

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

  vtkUnstructuredGridBase *input = this->GetInput();
  vtkVolumeProperty *property = volume->GetProperty();

  float last_max_cell_size = this->MaxCellSize;

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

  // Check to see if we need to rebuild opacity texture.
  if (   !this->Internals->OpacityTexture
      || (last_max_cell_size != this->MaxCellSize)
      || (this->LastProperty != property)
      || (this->OpacityTextureTime < property->GetMTime()) )
  {
    if (!this->Internals->OpacityTexture)
    {
      glGenTextures(1, &this->Internals->OpacityTexture);
    }
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->Internals->OpacityTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    float unit_distance = property->GetScalarOpacityUnitDistance();

#define TEXRES  258
    float *texture = new float[TEXRES*TEXRES];
    for (int depthi = 0; depthi < TEXRES; depthi++)
    {
      float depth = depthi*this->MaxCellSize/(TEXRES);
      for (int attenuationi = 0; attenuationi < TEXRES; attenuationi++)
      {
        float attenuation = (float)attenuationi/(TEXRES);
        float alpha = 1 - (float)exp(-attenuation*depth/unit_distance);
        texture[(depthi*TEXRES + attenuationi)] = alpha;
      }
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_INTENSITY, TEXRES, TEXRES, 1, GL_RED,
                 GL_FLOAT, texture);
    delete[] texture;

    glBindTexture(GL_TEXTURE_2D, 0);

    this->OpacityTextureTime.Modified();
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

  this->ProjectTetrahedra(renderer, volume);

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

#if 0
    invw = 1/(  inverse_projection_mat[ 3]*x
              + inverse_projection_mat[ 7]*y
              + inverse_projection_mat[11]*z1
              + inverse_projection_mat[15] );
    eye1[0] = invw*(  inverse_projection_mat[ 0]*x
                    + inverse_projection_mat[ 4]*y
                    + inverse_projection_mat[ 8]*z1
                    + inverse_projection_mat[12] );
    eye1[1] = invw*(  inverse_projection_mat[ 1]*x
                    + inverse_projection_mat[ 5]*y
                    + inverse_projection_mat[ 9]*z1
                    + inverse_projection_mat[13] );
    eye1[2] = invw*(  inverse_projection_mat[ 2]*x
                    + inverse_projection_mat[ 6]*y
                    + inverse_projection_mat[10]*z1
                    + inverse_projection_mat[14] );

    invw = 1/(  inverse_projection_mat[ 3]*x
              + inverse_projection_mat[ 7]*y
              + inverse_projection_mat[11]*z2
              + inverse_projection_mat[15] );
    eye2[0] = invw*(  inverse_projection_mat[ 0]*x
                    + inverse_projection_mat[ 4]*y
                    + inverse_projection_mat[ 8]*z2
                    + inverse_projection_mat[12] );
    eye2[1] = invw*(  inverse_projection_mat[ 1]*x
                    + inverse_projection_mat[ 5]*y
                    + inverse_projection_mat[ 9]*z2
                    + inverse_projection_mat[13] );
    eye2[2] = invw*(  inverse_projection_mat[ 2]*x
                    + inverse_projection_mat[ 6]*y
                    + inverse_projection_mat[10]*z2
                    + inverse_projection_mat[14] );
#else
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
#endif

    float dist2 = vtkMath::Distance2BetweenPoints(eye1, eye2);
    return this->SqrtTable[(int)(dist2*this->SqrtTableBias)];
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLProjectedTetrahedraMapper::ProjectTetrahedra(vtkRenderer *renderer,
                                                     vtkVolume *volume)
{
  vtkOpenGLClearErrorMacro();

  // after mucking about with FBO bindings be sure
  // we're saving the default fbo attributes/blend function
  glPushAttrib(GL_COLOR_BUFFER_BIT);
  vtkOpenGLCheckErrorMacro("failed at glPushAttrib");

  this->AllocateFBOResources(renderer);

  if (this->UseFloatingPointFrameBuffer
    && this->CanDoFloatingPointFrameBuffer)
  {
    // bind draw+read to set it up
    vtkgl::BindFramebuffer(
          vtkgl::FRAMEBUFFER,
          this->Internals->FrameBufferObjectId);

    glReadBuffer(GL_NONE);
    GLenum dbuf = vtkgl::COLOR_ATTACHMENT0;
    vtkgl::DrawBuffersARB(1, &dbuf);

    GLenum status = vtkgl::CheckFramebufferStatus(vtkgl::DRAW_FRAMEBUFFER);
    if (status!=vtkgl::FRAMEBUFFER_COMPLETE)
    {
      vtkErrorMacro("FBO is incomplete " << status);
    }

    // read from default
    vtkgl::BindFramebuffer(vtkgl::READ_FRAMEBUFFER, 0);
    // draw to fbo
    vtkgl::BindFramebuffer(vtkgl::DRAW_FRAMEBUFFER,
                           this->Internals->FrameBufferObjectId);

    vtkgl::BlitFramebuffer(0, 0,
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

  float projection_mat[16];
  float modelview_mat[16];
  glGetFloatv(GL_PROJECTION_MATRIX, projection_mat);
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview_mat);

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

  glDisable(GL_LIGHTING);
  glDepthMask(GL_FALSE);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, this->Internals->OpacityTexture);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glShadeModel(GL_SMOOTH);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDisable(GL_CULL_FACE);

  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // Establish vertex arrays.
  // tets have 4 points, 5th point here is used
  // to insert a point in case of intersections
  float tet_points[5*3] = {0.0f};
  glVertexPointer(3, GL_FLOAT, 0, tet_points);
  glEnableClientState(GL_VERTEX_ARRAY);

  unsigned char tet_colors[5*3] = {'\0'};
  glColorPointer(3, GL_UNSIGNED_BYTE, 0, tet_colors);
  glEnableClientState(GL_COLOR_ARRAY);

  float tet_texcoords[5*2] = {0.0f};
  glTexCoordPointer(2, GL_FLOAT, 0, tet_texcoords);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  // Since we had to transform the points on the CPU, replace the OpenGL
  // transforms with the identity matrix.
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  unsigned char *colors = this->Colors->GetPointer(0);
  vtkIdType totalnumcells = input->GetNumberOfCells();
  vtkIdType numcellsrendered = 0;
  vtkNew<vtkIdList> cellPointIds;
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
        tet_texcoords[2*4 + 1] = depth/this->MaxCellSize;

        // Establish the order in which the points should be rendered.
        unsigned char indices[6];
        indices[0] = 4;
        indices[1] = segment1[0];
        indices[2] = segment2[0];
        indices[3] = segment1[1];
        indices[4] = segment2[1];
        indices[5] = segment1[0];
        // Render
        glDrawElements(GL_TRIANGLE_FAN, 6, GL_UNSIGNED_BYTE, indices);
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
        T2[1] = depth/this->MaxCellSize;

        // Establish the order in which the points should be rendered.
        unsigned char indices[5];
        indices[0] = segment1[1];
        indices[1] = segment1[0];
        indices[2] = segment2[0];
        indices[3] = segment2[1];
        indices[4] = segment1[0];
        // Render
        glDrawElements(GL_TRIANGLE_FAN, 5, GL_UNSIGNED_BYTE, indices);
      }
    }
    numcellsrendered += num_cell_ids;
  }

  if (this->UseFloatingPointFrameBuffer
    && this->CanDoFloatingPointFrameBuffer)
  {
    // copy from our fbo to the default one
    vtkgl::BindFramebuffer(
          vtkgl::FRAMEBUFFER,
          this->Internals->FrameBufferObjectId);

    glReadBuffer(vtkgl::COLOR_ATTACHMENT0);
    glDrawBuffer(GL_NONE);

    GLenum status = vtkgl::CheckFramebufferStatus(vtkgl::READ_FRAMEBUFFER);
    if (status!=vtkgl::FRAMEBUFFER_COMPLETE)
    {
      vtkErrorMacro("FBO is incomplete " << status);
    }

    // read from fbo
    vtkgl::BindFramebuffer(vtkgl::READ_FRAMEBUFFER,
                           this->Internals->FrameBufferObjectId);
    // draw to default fbo
    vtkgl::BindFramebuffer(vtkgl::DRAW_FRAMEBUFFER, 0);

    vtkgl::BlitFramebuffer(0, 0, this->CurrentFBOWidth, this->CurrentFBOHeight,
                         0, 0, this->CurrentFBOWidth, this->CurrentFBOHeight,
                         GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    vtkOpenGLCheckErrorMacro("failed at glBlitFramebuffer");

    // restore default fbo for both read+draw
    vtkgl::BindFramebuffer(vtkgl::FRAMEBUFFER, 0);
  }

  // Restore OpenGL state.
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(projection_mat);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(modelview_mat);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);

  // Restore the blend function.
  glPopAttrib();
  vtkOpenGLCheckErrorMacro("failed at glPopAttrib");

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);

  glDepthMask(GL_TRUE);
  glEnable(GL_LIGHTING);

  vtkOpenGLCheckErrorMacro("failed after ProjectTetrahedra");
  this->UpdateProgress(1.0);
}
