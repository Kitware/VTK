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
#include "vtkTextureObject.h"

#include "vtkOpenGLHelper.h"

#include "vtkHomogeneousTransform.h"

#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"

#include <cmath>


// ---------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLTexture);

// ---------------------------------------------------------------------------
vtkOpenGLTexture::vtkOpenGLTexture()
{
  this->RenderWindow = 0;
  this->IsDepthTexture = 0;
  this->TextureType = GL_TEXTURE_2D;
  this->ExternalTextureObject = false;
  this->TextureObject = 0;
}

// ---------------------------------------------------------------------------
vtkOpenGLTexture::~vtkOpenGLTexture()
{
  if (this->RenderWindow)
  {
    this->ReleaseGraphicsResources(this->RenderWindow);
    this->RenderWindow = 0;
  }
  if (this->TextureObject)
  {
    this->TextureObject->Delete();
    this->TextureObject = NULL;
  }
}

// ---------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void vtkOpenGLTexture::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->TextureObject && win)
  {
    this->TextureObject->ReleaseGraphicsResources(win);
  }

  this->RenderWindow = NULL;
  this->Modified();
}

// ---------------------------------------------------------------------------
void vtkOpenGLTexture::SetTextureObject(vtkTextureObject *textureObject)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting TextureObject to " << textureObject );
  if (this->TextureObject != textureObject)
  {
    vtkTextureObject* temp = this->TextureObject;
    this->TextureObject = textureObject;
    if (this->TextureObject != NULL)
    {
      this->TextureObject->Register(this);
    }
    if (temp != NULL)
    {
      temp->UnRegister(this);
    }
    this->ExternalTextureObject = true;
    this->Modified();
  }
}

// ---------------------------------------------------------------------------
int vtkOpenGLTexture::GetTextureUnit()
{
  if (this->TextureObject)
  {
    return this->TextureObject->GetTextureUnit();
  }
  return -1;
}

// ---------------------------------------------------------------------------
void vtkOpenGLTexture::CopyTexImage(int x, int y, int width, int height)
{
  this->TextureObject->CopyFromFrameBuffer(x, y, x, y, width, height);
}

// ----------------------------------------------------------------------------
// Implement base class method.
void vtkOpenGLTexture::Render(vtkRenderer *ren)
{
  if (this->ExternalTextureObject)
  {
    this->Load(ren);
    return;
  }

  this->Superclass::Render(ren);
}

// ----------------------------------------------------------------------------
// Implement base class method.
void vtkOpenGLTexture::Load(vtkRenderer *ren)
{
  if (!this->ExternalTextureObject)
  {
    vtkImageData *input = this->GetInput();

    // Need to reload the texture.
    // There used to be a check on the render window's mtime, but
    // this is too broad of a check (e.g. it would cause all textures
    // to load when only the desired update rate changed).
    // If a better check is required, check something more specific,
    // like the graphics context.
    vtkOpenGLRenderWindow* renWin =
      static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());

    // has something changed so that we need to rebuild the texture?
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

      this->RenderWindow = renWin;
      if (this->TextureObject == 0)
      {
        this->TextureObject = vtkTextureObject::New();
      }
      this->TextureObject->ResetFormatAndType();
      this->TextureObject->SetContext(renWin);

      // Get the scalars the user choose to color with.
      vtkDataArray* scalars = this->GetInputArrayToProcess(0, input);

      // make sure scalars are non null
      if (!scalars)
      {
        vtkErrorMacro(<< "No scalar values found for texture input!");
        return;
      }

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
      if (this->IsDepthTexture != 1 &&
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

      // create the texture
      if (this->IsDepthTexture)
      {
        this->TextureObject->CreateDepthFromRaw(
          xsize, ysize, vtkTextureObject::Float32, scalars->GetDataType(), resultData);
      }
      else
      {
        this->TextureObject->Create2DFromRaw(
          xsize, ysize, bytesPerPixel, VTK_UNSIGNED_CHAR, resultData);
      }

      // activate a free texture unit for this texture
      this->TextureObject->Activate();

      // update parameters
      if (this->Interpolate)
      {
        this->TextureObject->SetMinificationFilter(vtkTextureObject::Linear);
        this->TextureObject->SetMagnificationFilter(vtkTextureObject::Linear);
      }
      else
      {
        this->TextureObject->SetMinificationFilter(vtkTextureObject::Nearest);
        this->TextureObject->SetMagnificationFilter(vtkTextureObject::Nearest);
      }
      if (this->Repeat)
      {
        this->TextureObject->SetWrapS(vtkTextureObject::Repeat);
        this->TextureObject->SetWrapT(vtkTextureObject::Repeat);
        this->TextureObject->SetWrapR(vtkTextureObject::Repeat);
      }
      else
      {
        this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
        this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
        this->TextureObject->SetWrapR(vtkTextureObject::ClampToEdge);
      }

      // modify the load time to the current time
      this->LoadTime.Modified();

      // free memory
      if (resultData != dataPtr)
      {
        delete [] resultData;
        resultData = 0;
      }
    }
  }
  else
  {
    vtkOpenGLRenderWindow* renWin =
      static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());

      // has something changed so that we need to rebuild the texture?
      if (this->GetMTime() > this->LoadTime.GetMTime() ||
         renWin != this->RenderWindow.GetPointer() ||
         renWin->GetContextCreationTime() > this->LoadTime)
      {
        this->RenderWindow = renWin;
        this->TextureObject->SetContext(renWin);
      }
  }

  // activate a free texture unit for this texture
  this->TextureObject->Activate();

  if (this->PremultipliedAlpha)
    {
    // save off current state of src / dst blend functions
    glGetIntegerv(GL_BLEND_SRC_RGB, &this->PrevBlendParams[0]);
    glGetIntegerv(GL_BLEND_DST_RGB, &this->PrevBlendParams[1]);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &this->PrevBlendParams[2]);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &this->PrevBlendParams[3]);

    // make the blend function correct for textures premultiplied by alpha.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

  vtkOpenGLCheckErrorMacro("failed after Load");
}

// ----------------------------------------------------------------------------
void vtkOpenGLTexture::PostRender(vtkRenderer *vtkNotUsed(ren))
{
  this->TextureObject->Deactivate();

  if (this->GetInput() && this->PremultipliedAlpha)
    {
    // restore the blend function
    glBlendFuncSeparate(
      this->PrevBlendParams[0], this->PrevBlendParams[1],
      this->PrevBlendParams[2], this->PrevBlendParams[3]);
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
}

// ----------------------------------------------------------------------------
int vtkOpenGLTexture::IsTranslucent()
{
  if (this->ExternalTextureObject)
  {
    // If number of components are 1, 2, or 4 then mostly
    // we can assume that the data can be used as alpha values.
    if (this->TextureObject->GetComponents() == 1 ||
        this->TextureObject->GetComponents() == 2 ||
        this->TextureObject->GetComponents() == 4)
    {
      return 1;
    }

      return 0;
  }

  return this->Superclass::IsTranslucent();
}
