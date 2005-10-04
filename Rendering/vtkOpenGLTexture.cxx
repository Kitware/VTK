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

#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkOpenGL.h"

#include <math.h>

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLTexture, "1.57");
vtkStandardNewMacro(vtkOpenGLTexture);
#endif

// Initializes an instance, generates a unique index.
vtkOpenGLTexture::vtkOpenGLTexture()
{
  this->Index = 0;
  this->RenderWindow = 0;
}

vtkOpenGLTexture::~vtkOpenGLTexture()
{
  this->RenderWindow = NULL;
}

// Release the graphics resources used by this texture.  
void vtkOpenGLTexture::ReleaseGraphicsResources(vtkWindow *renWin)
{
  if (this->Index && renWin)
    {
    ((vtkRenderWindow *) renWin)->MakeCurrent();
#ifdef GL_VERSION_1_1
    // free any textures
    if (glIsTexture(this->Index))
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
    }
  this->Index = 0;
  this->RenderWindow = NULL;
  this->Modified();
}

// Implement base class method.
void vtkOpenGLTexture::Load(vtkRenderer *ren)
{
  GLenum format = GL_LUMINANCE;
  vtkImageData *input = this->GetInput();
  
  // need to reload the texture
  if (this->GetMTime() > this->LoadTime.GetMTime() ||
      input->GetMTime() > this->LoadTime.GetMTime() ||
      (this->GetLookupTable() && this->GetLookupTable()->GetMTime () >  
       this->LoadTime.GetMTime()) || 
       ren->GetRenderWindow() != this->RenderWindow)
    {
    int bytesPerPixel;
    int *size;
    vtkDataArray *scalars;
    unsigned char *dataPtr;
    int rowLength;
    unsigned char *resultData=NULL;
    int xsize, ysize;
    unsigned short xs,ys;
    GLuint tempIndex=0;

    // get some info
    size = input->GetDimensions();
    scalars = input->GetPointData()->GetScalars();

    // make sure scalars are non null
    if (!scalars) 
      {
      vtkErrorMacro(<< "No scalar values found for texture input!");
      return;
      }

    bytesPerPixel = scalars->GetNumberOfComponents();

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

    // xsize and ysize must be a power of 2 in OpenGL
    xs = (unsigned short)xsize;
    ys = (unsigned short)ysize;
    while (!(xs & 0x01))
      {
      xs = xs >> 1;
      }
    while (!(ys & 0x01))
      {
      ys = ys >> 1;
      }

    // -- decide whether the texture needs to be resampled --
    int resampleNeeded = 0;
    // if not a power of two then resampling is required
    if ((xs > 1)||(ys > 1))
      {
      resampleNeeded = 1;
      }
    GLint maxDimGL;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxDimGL);
    // if larger than permitted by the graphics library then must resample
    if ( xsize > maxDimGL || ysize > maxDimGL )
      {
      vtkDebugMacro( "Texture too big for gl, maximum is " << maxDimGL);
      resampleNeeded = 1;
      }

    if ( resampleNeeded )
      {
      vtkDebugMacro(<< "Resampling texture to power of two for OpenGL");
      resultData = this->ResampleToPowerOfTwo(xsize, ysize, dataPtr, 
                                              bytesPerPixel);
      }

    // format the data so that it can be sent to opengl
    // each row must be a multiple of 4 bytes in length
    // the best idea is to make your size a multiple of 4
    // so that this conversion will never be done.
    rowLength = ((xsize*bytesPerPixel +3 )/4)*4;
    if (rowLength == xsize*bytesPerPixel)
      {
      if ( resultData == NULL )
        {
        resultData = dataPtr;
        }
      }
    else
      {
      int col;
      unsigned char *src,*dest;
      int srcLength;

      srcLength = xsize*bytesPerPixel;
      resultData = new unsigned char [rowLength*ysize];
      
      src = dataPtr;
      dest = resultData;

      for (col = 0; col < ysize; col++)
        {
        memcpy(dest,src,srcLength);
        src += srcLength;
        dest += rowLength;
        }
      }

    // free any old display lists (from the old context)
    if (this->RenderWindow)
      {
      this->ReleaseGraphicsResources(this->RenderWindow);
      }
    
     this->RenderWindow = ren->GetRenderWindow();
     
    // make the new context current before we mess with opengl
    this->RenderWindow->MakeCurrent();
 
    // define a display list for this texture
    // get a unique display list id
#ifdef GL_VERSION_1_1
    glGenTextures(1, &tempIndex);
    this->Index = (long) tempIndex;
    glBindTexture(GL_TEXTURE_2D, this->Index);
#else
    this->Index = glGenLists(1);
    glDeleteLists ((GLuint) this->Index, (GLsizei) 0);
    glNewList ((GLuint) this->Index, GL_COMPILE);
#endif

    ((vtkOpenGLRenderWindow *)(ren->GetRenderWindow()))->RegisterTextureResource( this->Index );
    
    if (this->Interpolate)
      {
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                       GL_LINEAR);
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                       GL_LINEAR );
      }
    else
      {
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
      }
    if (this->Repeat)
      {
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT );
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT );
      }
    else
      {
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP );
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP );
      }
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
    glTexImage2D( GL_TEXTURE_2D, 0 , internalFormat,
                  xsize, ysize, 0, format, 
                  GL_UNSIGNED_BYTE, (const GLvoid *)resultData );
#ifndef GL_VERSION_1_1
    glEndList ();
#endif
    // modify the load time to the current time
    this->LoadTime.Modified();
    
    // free memory
    if (resultData != dataPtr)
      {
      delete [] resultData;
      }
    }

  // execute the display list that uses creates the texture
#ifdef GL_VERSION_1_1
  glBindTexture(GL_TEXTURE_2D, this->Index);
#else
  glCallList ((GLuint) this->Index);
#endif
  
  // don't accept fragments if they have zero opacity. this will stop the
  // zbuffer from be blocked by totally transparent texture fragments.
  glAlphaFunc (GL_GREATER, (GLclampf) 0);
  glEnable (GL_ALPHA_TEST);

  // now bind it 
  glEnable(GL_TEXTURE_2D);
}



static int FindPowerOfTwo(int i)
{
  int size;

  for ( i--, size=1; i > 0; size*=2 )
    {
    i /= 2;
    }

  // [these lines added by Tim Hutton (implementing Joris Vanden Wyngaerd's suggestions)]
  // limit the size of the texture to the maximum allowed by OpenGL
  // (slightly more graceful than texture failing but not ideal)
  GLint maxDimGL;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxDimGL);
  if ( size > maxDimGL )
    {
        size = maxDimGL ;
    }
  // end of Tim's additions
 
  return size;
}

// Creates resampled unsigned char texture map that is a power of two in bith x and y.
unsigned char *vtkOpenGLTexture::ResampleToPowerOfTwo(int &xs, int &ys, unsigned char *dptr,
                                                      int bpp)
{
  unsigned char *tptr, *p, *p1, *p2, *p3, *p4;
  int xsize, ysize, i, j, k, jOffset, iIdx, jIdx;
  float pcoords[3], hx, hy, rm, sm, w0, w1, w2, w3;

  xsize = FindPowerOfTwo(xs);
  ysize = FindPowerOfTwo(ys);
  
  hx = (float)(xs - 1.0) / (xsize - 1.0);
  hy = (float)(ys - 1.0) / (ysize - 1.0);

  tptr = p = new unsigned char[xsize*ysize*bpp];

  //Resample from the previous image. Compute parametric coordinates and interpolate
  for (j=0; j < ysize; j++)
    {
    pcoords[1] = j*hy;

    jIdx = (int)pcoords[1];
    if ( jIdx >= (ys-1) ) //make sure to interpolate correctly at edge
      {
      jIdx = ys - 2;
      pcoords[1] = 1.0;
      }
    else
      {
      pcoords[1] = pcoords[1] - jIdx;
      }
    jOffset = jIdx*xs;
    sm = 1.0 - pcoords[1];

    for (i=0; i < xsize; i++)
      {
      pcoords[0] = i*hx;
      iIdx = (int)pcoords[0];
      if ( iIdx >= (xs-1) ) 
        {
        iIdx = xs - 2;
        pcoords[0] = 1.0;
        }
      else
        {
        pcoords[0] = pcoords[0] - iIdx;
        }
      rm = 1.0 - pcoords[0];

      // Get pointers to 4 surrounding pixels
      p1 = dptr + bpp*(iIdx + jOffset);
      p2 = p1 + bpp;
      p3 = p1 + bpp*xs;
      p4 = p3 + bpp;

      // Compute interpolation weights interpolate components
      w0 = rm*sm; 
      w1 = pcoords[0]*sm;
      w2 = rm*pcoords[1];
      w3 = pcoords[0]*pcoords[1];
      for (k=0; k < bpp; k++)
        {
        *p++ = (unsigned char) (p1[k]*w0 + p2[k]*w1 + p3[k]*w2 + p4[k]*w3);
        }
      }
    }

  xs = xsize;
  ys = ysize;
  
  return tptr;
}

void vtkOpenGLTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Index: " << this->Index << endl;
}
