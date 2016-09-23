/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLTexture.h"

#include "vtkHomogeneousTransform.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTransform.h"
#include "vtkPixelBufferObject.h"
#include "vtkOpenGL.h"
#include "vtkOpenGLError.h"
#include "vtkgl.h" // vtkgl namespace

#include <cmath>


// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLTexture);

// ----------------------------------------------------------------------------
vtkOpenGLTexture::vtkOpenGLTexture()
{
  this->Index = 0;
  this->RenderWindow = 0;
  this->CheckedHardwareSupport = false;
  this->SupportsNonPowerOfTwoTextures = false;
  this->SupportsPBO = false;
  this->PBO = 0;
}

// ----------------------------------------------------------------------------
vtkOpenGLTexture::~vtkOpenGLTexture()
{
  if (this->RenderWindow)
  {
    this->ReleaseGraphicsResources(this->RenderWindow);
    this->RenderWindow = 0;
  }
  if (this->PBO)
  {
    vtkErrorMacro(<< "PBO should have been deleted in ReleaseGraphicsResources()");
  }
}

// ----------------------------------------------------------------------------
void vtkOpenGLTexture::Initialize(vtkRenderer* vtkNotUsed(ren))
{
}

// ----------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void vtkOpenGLTexture::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->Index && win)
  {
    vtkRenderWindow *renWin = dynamic_cast<vtkRenderWindow *>(win);
    renWin->MakeCurrent();
    vtkOpenGLClearErrorMacro();
#ifdef GL_VERSION_1_1
    // free any textures
    if (glIsTexture(static_cast<GLuint>(this->Index)))
    {
      GLuint tempIndex;
      tempIndex = this->Index;
      // NOTE: Sun's OpenGL seems to require disabling of texture before delete
      glDisable(GL_TEXTURE_2D);
      glDeleteTextures(1, &tempIndex);
    }
#else
    if (glIsList(this->Index))
    {
      glDeleteLists(this->Index,1);
    }
#endif
    vtkOpenGLCheckErrorMacro("failed after ReleaseGraphicsResources");
  }

  this->Index = 0;
  this->RenderWindow = NULL;
  this->CheckedHardwareSupport = false;
  this->SupportsNonPowerOfTwoTextures = false;
  this->SupportsPBO = false;
  if(this->PBO)
  {
    this->PBO->Delete();
    this->PBO = 0;
  }
  this->Modified();
}

// ----------------------------------------------------------------------------
// Implement base class method.
void vtkOpenGLTexture::Load(vtkRenderer *ren)
{
  GLenum format = GL_LUMINANCE;
  vtkImageData *input = this->GetInput();

  this->Initialize(ren);
  // Need to reload the texture.
  // There used to be a check on the render window's mtime, but
  // this is too broad of a check (e.g. it would cause all textures
  // to load when only the desired update rate changed).
  // If a better check is required, check something more specific,
  // like the graphics context.
  vtkOpenGLRenderWindow* renWin =
    static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());

  vtkOpenGLClearErrorMacro();

  if (this->BlendingMode != VTK_TEXTURE_BLENDING_MODE_NONE
     && vtkgl::ActiveTexture)
  {
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, vtkgl::COMBINE);

    switch (this->BlendingMode)
    {
      case VTK_TEXTURE_BLENDING_MODE_REPLACE:
      {
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_RGB, GL_REPLACE);
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_ALPHA, GL_REPLACE);
        break;
      }
      case VTK_TEXTURE_BLENDING_MODE_MODULATE:
      {
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_RGB, GL_MODULATE);
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_ALPHA, GL_MODULATE);
        break;
      }
      case VTK_TEXTURE_BLENDING_MODE_ADD:
      {
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_RGB, GL_ADD);
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_ALPHA, GL_ADD);
        break;
      }
      case VTK_TEXTURE_BLENDING_MODE_ADD_SIGNED:
      {
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_RGB, vtkgl::ADD_SIGNED);
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_ALPHA, vtkgl::ADD_SIGNED);
        break;
      }
      case VTK_TEXTURE_BLENDING_MODE_INTERPOLATE:
      {
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_RGB, vtkgl::INTERPOLATE);
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_ALPHA, vtkgl::INTERPOLATE);
        break;
      }
      case VTK_TEXTURE_BLENDING_MODE_SUBTRACT:
      {
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_RGB, vtkgl::SUBTRACT);
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_ALPHA, vtkgl::SUBTRACT);
        break;
      }
      default:
      {
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_RGB, GL_ADD);
        glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_ALPHA, GL_ADD);
      }
    }
    vtkOpenGLCheckErrorMacro("failed at glTexEnvf");
  }

  if (this->GetMTime() > this->LoadTime.GetMTime() ||
      input->GetMTime() > this->LoadTime.GetMTime() ||
      (this->GetLookupTable() && this->GetLookupTable()->GetMTime () >
       this->LoadTime.GetMTime()) ||
       renWin != this->RenderWindow.GetPointer() ||
       renWin->GetContextCreationTime() > this->LoadTime)
  {
    int size[3];
    unsigned char *dataPtr;
    unsigned char *resultData = 0;
    int xsize, ysize;
    GLuint tempIndex = 0;

    // Get the scalars the user choose to color with.
    vtkDataArray* scalars = this->GetInputArrayToProcess(0, input);

    // make sure scalars are non null
    if (!scalars)
    {
      vtkErrorMacro(<< "No scalar values found for texture input!");
      return;
    }

    // free any old display lists (from the old context)
    // make the new context current before we mess with opengl
    if (this->RenderWindow)
    {
      this->ReleaseGraphicsResources(this->RenderWindow);
    }
    this->RenderWindow = renWin;
    this->RenderWindow->MakeCurrent();

    // get some info
    input->GetDimensions(size);

    if (input->GetNumberOfCells() == scalars->GetNumberOfTuples())
    {
      // we are using cell scalars. Adjust image size for cells.
      for (int kk = 0; kk < 3; kk++)
      {
        if (size[kk]>1)
        {
          size[kk]--;
        }
      }
    }

    int bytesPerPixel = scalars->GetNumberOfComponents();

    // make sure using unsigned char data of color scalars type
    if (this->MapColorScalarsThroughLookupTable ||
       scalars->GetDataType() != VTK_UNSIGNED_CHAR )
    {
      dataPtr = this->MapScalarsToColors (scalars);
      bytesPerPixel = 4;
    }
    else
    {
      dataPtr = static_cast<vtkUnsignedCharArray *>(scalars)->GetPointer(0);
    }

    // we only support 2d texture maps right now
    // so one of the three sizes must be 1, but it
    // could be any of them, so lets find it
    if (size[0] == 1)
    {
      xsize = size[1]; ysize = size[2];
    }
    else
    {
      xsize = size[0];
      if (size[1] == 1)
      {
        ysize = size[2];
      }
      else
      {
        ysize = size[1];
        if (size[2] != 1)
        {
          vtkErrorMacro(<< "3D texture maps currently are not supported!");
          return;
        }
      }
    }

    if (!this->CheckedHardwareSupport)
    {
      vtkOpenGLExtensionManager *m = renWin->GetExtensionManager();
      this->CheckedHardwareSupport = true;
      this->SupportsNonPowerOfTwoTextures =
        m->ExtensionSupported("GL_VERSION_2_0")
        || m->ExtensionSupported("GL_ARB_texture_non_power_of_two");
      this->SupportsPBO=vtkPixelBufferObject::IsSupported(renWin);
    }

    // -- decide whether the texture needs to be resampled --

    GLint maxDimGL;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxDimGL);
    vtkOpenGLCheckErrorMacro("failed at glGetIntegerv");
    // if larger than permitted by the graphics library then must resample
    bool resampleNeeded=xsize > maxDimGL || ysize > maxDimGL;
    if(resampleNeeded)
    {
      vtkDebugMacro( "Texture too big for gl, maximum is " << maxDimGL);
    }

    if(!resampleNeeded && !this->SupportsNonPowerOfTwoTextures)
    {
      // if not a power of two then resampling is required
      resampleNeeded = !(vtkMath::IsPowerOfTwo(xsize) &&
                         vtkMath::IsPowerOfTwo(ysize));
    }

    if (resampleNeeded)
    {
      vtkDebugMacro(<< "Resampling texture to power of two for OpenGL");
      resultData = this->ResampleToPowerOfTwo(xsize, ysize, dataPtr,
                                              bytesPerPixel);
    }

    if (!resultData)
    {
        resultData = dataPtr;
    }


    // define a display list for this texture
    // get a unique display list id

#ifdef GL_VERSION_1_1
    glGenTextures(1, &tempIndex);
    vtkOpenGLCheckErrorMacro("failed at glGenTextures");
    this->Index = static_cast<long>(tempIndex);
    glBindTexture(GL_TEXTURE_2D, this->Index);
    vtkOpenGLCheckErrorMacro("failed at glBindTexture");
#else
    this->Index = glGenLists(1);
    glDeleteLists(static_cast<GLuint>(this->Index), static_cast<GLsizei>(0));
    vtkOpenGLCheckErrorMacro("failed at glDeleteLists");
    glNewList(static_cast<GLuint>(this->Index), GL_COMPILE);
    vtkOpenGLCheckErrorMacro("failed at glNewList");
#endif
    //seg fault protection for those wackos that don't use an
    //opengl render window
    if (this->RenderWindow->IsA("vtkOpenGLRenderWindow"))
    {
      static_cast<vtkOpenGLRenderWindow *>(ren->GetRenderWindow())->
        RegisterTextureResource( this->Index );
    }

    if (this->Interpolate)
    {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    if (this->Repeat)
    {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else
    {
      vtkOpenGLExtensionManager* manager = renWin->GetExtensionManager();
      if (this->EdgeClamp &&
           (manager->ExtensionSupported("GL_VERSION_1_2") ||
            manager->ExtensionSupported("GL_EXT_texture_edge_clamp")))
      {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, vtkgl::CLAMP_TO_EDGE);
      }
      else
      {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      }
    }
    vtkOpenGLCheckErrorMacro("failed at glTexParameterf");
    int internalFormat = bytesPerPixel;
    switch (bytesPerPixel)
    {
      case 1: format = GL_LUMINANCE; break;
      case 2: format = GL_LUMINANCE_ALPHA; break;
      case 3: format = GL_RGB; break;
      case 4: format = GL_RGBA; break;
    }
    // if we are using OpenGL 1.1, you can force 32 or16 bit textures
#ifdef GL_VERSION_1_1
    if (this->Quality == VTK_TEXTURE_QUALITY_32BIT)
    {
      switch (bytesPerPixel)
      {
        case 1: internalFormat = GL_LUMINANCE8; break;
        case 2: internalFormat = GL_LUMINANCE8_ALPHA8; break;
        case 3: internalFormat = GL_RGB8; break;
        case 4: internalFormat = GL_RGBA8; break;
      }
    }
    else if (this->Quality == VTK_TEXTURE_QUALITY_16BIT)
    {
      switch (bytesPerPixel)
      {
        case 1: internalFormat = GL_LUMINANCE4; break;
        case 2: internalFormat = GL_LUMINANCE4_ALPHA4; break;
        case 3: internalFormat = GL_RGB4; break;
        case 4: internalFormat = GL_RGBA4; break;
      }
    }
#endif
    if (this->SupportsPBO)
    {
      if (!this->PBO)
      {
        this->PBO = vtkPixelBufferObject::New();
        this->PBO->SetContext(renWin);
      }
      unsigned int dims[2];
      dims[0] = static_cast<unsigned int>(xsize);
      dims[1] = static_cast<unsigned int>(ysize);
      vtkIdType increments[2];
      increments[0] = 0;
      increments[1] = 0;
      this->PBO->Upload2D(VTK_UNSIGNED_CHAR, resultData, dims, bytesPerPixel, increments);
      // non-blocking call
      this->PBO->Bind(vtkPixelBufferObject::UNPACKED_BUFFER);
      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                   xsize, ysize, 0, format,
                   GL_UNSIGNED_BYTE, 0);
      vtkOpenGLCheckErrorMacro("failed at glTexImage2D");
      this->PBO->UnBind();
    }
    else
    {
      // blocking call
      glTexImage2D(GL_TEXTURE_2D, 0 , internalFormat,
                   xsize, ysize, 0, format, GL_UNSIGNED_BYTE,
                   static_cast<const GLvoid *>(resultData));
      vtkOpenGLCheckErrorMacro("failed at glTexImage2D");
    }
#ifndef GL_VERSION_1_1
    glEndList ();
    vtkOpenGLCheckErrorMacro("failed at glEndList");
#endif
    // modify the load time to the current time
    this->LoadTime.Modified();

    // free memory
    if (resultData != dataPtr)
    {
      delete [] resultData;
      resultData = 0;
    }
  }

  // execute the display list that uses creates the texture
#ifdef GL_VERSION_1_1
  glBindTexture(GL_TEXTURE_2D, this->Index);
  vtkOpenGLCheckErrorMacro("failed at glBindTexture");
#else
  glCallList(this->Index);
  vtkOpenGLCheckErrorMacro("failed at glCallList");
#endif

  // don't accept fragments if they have zero opacity. this will stop the
  // zbuffer from be blocked by totally transparent texture fragments.
  glAlphaFunc(GL_GREATER, static_cast<GLclampf>(0.f));
  glEnable(GL_ALPHA_TEST);

  if (this->PremultipliedAlpha)
  {
    // save the blend function.
    glPushAttrib(GL_COLOR_BUFFER_BIT);

    // make the blend function correct for textures premultiplied by alpha.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  }

  // now bind it
  glEnable(GL_TEXTURE_2D);

  // clear any texture transform
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  // build transformation
  if (this->Transform)
  {
    double mat[16];
    vtkMatrix4x4::Transpose(*this->Transform->GetMatrix()->Element, mat);
    // insert texture transformation
    glMultMatrixd(mat);
  }
  glMatrixMode(GL_MODELVIEW);

  GLint uUseTexture = -1;
  GLint uTexture = -1;

  vtkOpenGLRenderer *oRenderer=static_cast<vtkOpenGLRenderer *>(ren);

  if(oRenderer->GetDepthPeelingHigherLayer())
  {
    uUseTexture = oRenderer->GetUseTextureUniformVariable();
    uTexture = oRenderer->GetTextureUniformVariable();
    vtkgl::Uniform1i(uUseTexture, 1);
    vtkgl::Uniform1i(uTexture, 0); // active texture 0
  }

  vtkOpenGLCheckErrorMacro("failed after Load");
}

// ----------------------------------------------------------------------------
void vtkOpenGLTexture::PostRender(vtkRenderer *ren)
{
  (void)ren;
  if (this->GetInput() && this->PremultipliedAlpha)
  {
    // restore the blend function
    glPopAttrib();
    vtkOpenGLCheckErrorMacro("failed after PostRender");
  }
}

// ----------------------------------------------------------------------------
static int FindPowerOfTwo(int i)
{
  int size = vtkMath::NearestPowerOfTwo(i);

  // [these lines added by Tim Hutton (implementing Joris Vanden Wyngaerd's
  // suggestions)]
  // limit the size of the texture to the maximum allowed by OpenGL
  // (slightly more graceful than texture failing but not ideal)
  GLint maxDimGL;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxDimGL);
  if (size < 0 || size > maxDimGL)
  {
    size = maxDimGL ;
  }
  // end of Tim's additions

  return size;
}

// ----------------------------------------------------------------------------
// Creates resampled unsigned char texture map that is a power of two in both
// x and y.
unsigned char *vtkOpenGLTexture::ResampleToPowerOfTwo(int &xs,
                                                      int &ys,
                                                      unsigned char *dptr,
                                                      int bpp)
{
  unsigned char *tptr, *p, *p1, *p2, *p3, *p4;
  int jOffset, iIdx, jIdx;
  double pcoords[3], rm, sm, w0, w1, w2, w3;
  int yInIncr = xs;
  int xInIncr = 1;

  int xsize = FindPowerOfTwo(xs);
  int ysize = FindPowerOfTwo(ys);
  if (this->RestrictPowerOf2ImageSmaller)
  {
    if (xsize > xs)
    {
      xsize /= 2;
    }
    if (ysize > ys)
    {
      ysize /= 2;
    }
  }
  double hx = xsize > 1 ? (xs - 1.0) / (xsize - 1.0) : 0;
  double hy = ysize > 1 ? (ys - 1.0) / (ysize - 1.0) : 0;

  tptr = p = new unsigned char[xsize*ysize*bpp];

  // Resample from the previous image. Compute parametric coordinates and
  // interpolate
  for (int j = 0; j < ysize; j++)
  {
    pcoords[1] = j*hy;

    jIdx = static_cast<int>(pcoords[1]);
    if (jIdx >= (ys-1)) //make sure to interpolate correctly at edge
    {
      if (ys == 1)
      {
        jIdx = 0;
        yInIncr = 0;
      }
      else
      {
        jIdx = ys - 2;
      }
      pcoords[1] = 1.0;
    }
    else
    {
      pcoords[1] = pcoords[1] - jIdx;
    }
    jOffset = jIdx*xs;
    sm = 1.0 - pcoords[1];

    for (int i = 0; i < xsize; i++)
    {
      pcoords[0] = i*hx;
      iIdx = static_cast<int>(pcoords[0]);
      if (iIdx >= (xs-1))
      {
        if (xs == 1)
        {
          iIdx = 0;
          xInIncr = 0;
        }
        else
        {
          iIdx = xs - 2;
        }
        pcoords[0] = 1.0;
      }
      else
      {
        pcoords[0] = pcoords[0] - iIdx;
      }
      rm = 1.0 - pcoords[0];

      // Get pointers to 4 surrounding pixels
      p1 = dptr + bpp*(iIdx + jOffset);
      p2 = p1 + bpp*xInIncr;
      p3 = p1 + bpp*yInIncr;
      p4 = p3 + bpp*xInIncr;

      // Compute interpolation weights interpolate components
      w0 = rm*sm;
      w1 = pcoords[0]*sm;
      w2 = rm*pcoords[1];
      w3 = pcoords[0]*pcoords[1];
      for (int k = 0; k < bpp; k++)
      {
        *p++ = static_cast<unsigned char>(p1[k]*w0 + p2[k]*w1 + p3[k]*w2
                                          + p4[k]*w3);
      }
    }
  }

  xs = xsize;
  ys = ysize;

  return tptr;
}

// ----------------------------------------------------------------------------
void vtkOpenGLTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Index: " << this->Index << endl;
}
