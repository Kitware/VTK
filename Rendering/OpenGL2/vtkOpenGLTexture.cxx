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

#include "vtkglVBOHelper.h"

#include "vtkHomogeneousTransform.h"

#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPixelBufferObject.h"
#include "vtkOpenGLError.h"

#include <math.h>


// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLTexture);

// ----------------------------------------------------------------------------
vtkOpenGLTexture::vtkOpenGLTexture()
{
  this->Index = 0;
  this->RenderWindow = 0;
  this->TextureFormat = GL_RGBA;
  this->TextureType = GL_TEXTURE_2D;
}

// ----------------------------------------------------------------------------
vtkOpenGLTexture::~vtkOpenGLTexture()
{
  if (this->RenderWindow)
    {
    this->ReleaseGraphicsResources(this->RenderWindow);
    this->RenderWindow = 0;
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
  if (this->Index && win && win->GetMapped())
    {
    vtkRenderWindow *renWin = dynamic_cast<vtkRenderWindow *>(win);
    renWin->MakeCurrent();
    vtkOpenGLClearErrorMacro();

    // free any textures
    if (glIsTexture(static_cast<GLuint>(this->Index)))
      {
      GLuint tempIndex;
      tempIndex = this->Index;
      glDeleteTextures(1, &tempIndex);
      }
    vtkOpenGLCheckErrorMacro("failed after ReleaseGraphicsResources");
    }

  this->Index = 0;
  this->RenderWindow = NULL;
  this->Modified();
}

void vtkOpenGLTexture::CopyTexImage(vtkRenderer *ren, int x, int y, int width, int height)
{
  vtkOpenGLRenderWindow* renWin =
    static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());
  renWin->ActivateTexture(this);
  if (this->TextureFormat == GL_DEPTH)
    {
    glCopyTexImage2D(this->TextureType, 0, GL_DEPTH_COMPONENT,
      x, y, width, height, 0);
    }
  else
    {
    glCopyTexImage2D(this->TextureType, 0, GL_RGBA,
      x, y, width, height, 0);
    }
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

  // TODO: need something for blending modes


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
    if (this->TextureFormat != GL_DEPTH &&
      (this->MapColorScalarsThroughLookupTable ||
       scalars->GetDataType() != VTK_UNSIGNED_CHAR ))
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

    // -- decide whether the texture needs to be resampled --
    GLint maxDimGL;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxDimGL);
    vtkOpenGLCheckErrorMacro("failed at glGetIntegerv");
    // if larger than permitted by the graphics library then must resample
    bool resampleNeeded = xsize > maxDimGL || ysize > maxDimGL;
    if(resampleNeeded)
      {
      vtkDebugMacro( "Texture too big for gl, maximum is " << maxDimGL);
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
    renWin->ActivateTexture(this);
    glGenTextures(1, &tempIndex);
    vtkOpenGLCheckErrorMacro("failed at glGenTextures");
    this->Index = static_cast<long>(tempIndex);
    glBindTexture(this->TextureType, this->Index);
    vtkOpenGLCheckErrorMacro("failed at glBindTexture");

    if (this->Interpolate)
      {
      glTexParameterf(this->TextureType , GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameterf(this->TextureType , GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
    else
      {
      glTexParameterf(this->TextureType , GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(this->TextureType , GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
    if (this->Repeat)
      {
      glTexParameterf(this->TextureType , GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameterf(this->TextureType , GL_TEXTURE_WRAP_T, GL_REPEAT);
      }
    else
      {
      if (this->EdgeClamp)
        {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
      else
        {
        glTexParameterf(this->TextureType , GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf(this->TextureType , GL_TEXTURE_WRAP_T, GL_CLAMP);
        }
      }
    vtkOpenGLCheckErrorMacro("failed at glTexParameterf");
    int internalFormat = bytesPerPixel;
    int dataType = GL_UNSIGNED_BYTE;
    switch (bytesPerPixel)
      {
      case 1: format = GL_LUMINANCE; break;
      case 2: format = GL_LUMINANCE_ALPHA; break;
      case 3: format = GL_RGB; break;
      case 4: format = GL_RGBA; break;
      }
    // if we are using OpenGL 1.1, you can force 32 or16 bit textures
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

    // handle depth textures
    if (this->TextureFormat == GL_DEPTH)
      {
      format = GL_DEPTH_COMPONENT;
      internalFormat = GL_DEPTH_COMPONENT32F;
      dataType = GL_FLOAT;
      }

    // blocking call
    glTexImage2D(this->TextureType , 0 , internalFormat,
                 xsize, ysize, 0, format, dataType,
                 static_cast<const GLvoid *>(resultData));

    vtkOpenGLCheckErrorMacro("failed at glTexImage2D");
    // modify the load time to the current time
    this->LoadTime.Modified();

    // free memory
    if (resultData != dataPtr)
      {
      delete [] resultData;
      resultData = 0;
      }
    }

  // activate a free texture unit for this texture
  renWin->ActivateTexture(this);
  glBindTexture(this->TextureType , this->Index);

  if (this->PremultipliedAlpha)
    {
    // save the blend function.
    glPushAttrib(GL_COLOR_BUFFER_BIT);

    // make the blend function correct for textures premultiplied by alpha.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

  vtkOpenGLCheckErrorMacro("failed after Load");
}

// ----------------------------------------------------------------------------
void vtkOpenGLTexture::PostRender(vtkRenderer *ren)
{
  vtkOpenGLRenderWindow* renWin =
    static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());
  // deactivate the texture
  renWin->DeactivateTexture(this);
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
