/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>
#include <string.h>

#include "vtkRenderWindow.h"
#ifdef VTK_USE_QUARTZ
#include "vtkQuartzRenderWindow.h"
#else
#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#else
#include "vtkOpenGLRenderWindow.h"
#endif
#endif
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLImageActor.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include "vtkObjectFactory.h"


#ifndef VTK_IMPLEMENT_MESA_CXX
//-----------------------------------------------------------------------------
vtkOpenGLImageActor* vtkOpenGLImageActor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLImageActor");
  if(ret)
    {
    return (vtkOpenGLImageActor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLImageActor;
}
#endif

// Initializes an instance, generates a unique index.
vtkOpenGLImageActor::vtkOpenGLImageActor()
{
  this->Index = 0;
  this->RenderWindow = 0;
}

vtkOpenGLImageActor::~vtkOpenGLImageActor()
{
  this->RenderWindow = NULL;
}

// Release the graphics resources used by this texture.  
void vtkOpenGLImageActor::ReleaseGraphicsResources(vtkWindow *renWin)
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

unsigned char *vtkOpenGLImageActor::MakeDataSuitable(int &xsize, int &ysize,
                                                     int &release)
{
  int contiguous = 0;
  unsigned short xs,ys;
  int powOfTwo = 0;
  int numComp = this->Input->GetNumberOfScalarComponents();
  int xdim, ydim;
  
  // it must be a power of two and contiguous
  // find the two used dimensions
  // this assumes a 2D image, no lines here folk
  if (this->DisplayExtent[0] != this->DisplayExtent[1])
    {
    xdim = 0;
    if (this->DisplayExtent[2] != this->DisplayExtent[3])
      {
      ydim = 1;
      }
    else
      {
      ydim = 2;
      }
    }
  else
    {
    xdim = 1;
    ydim = 2;
    }
  
  float *spacing = this->Input->GetSpacing();
  float *origin = this->Input->GetOrigin();
  
  // compute the world coordinates
  this->Coords[0] = this->DisplayExtent[0]*spacing[0] + origin[0];
  this->Coords[1] = this->DisplayExtent[2]*spacing[1] + origin[1];
  this->Coords[2] = this->DisplayExtent[4]*spacing[2] + origin[2];
  this->Coords[3] = this->DisplayExtent[1]*spacing[0] + origin[0];
  this->Coords[4] = 
    this->DisplayExtent[2 + (xdim == 1)]*spacing[1] + origin[1];
  this->Coords[5] = this->DisplayExtent[4]*spacing[2] + origin[2];
  this->Coords[6] = this->DisplayExtent[1]*spacing[0] + origin[0];
  this->Coords[7] = this->DisplayExtent[3]*spacing[1] + origin[1];
  this->Coords[8] = this->DisplayExtent[5]*spacing[2] + origin[2];
  this->Coords[9] = this->DisplayExtent[0]*spacing[0] + origin[0];
  this->Coords[10] = 
    this->DisplayExtent[2 + (ydim == 1)]*spacing[1] + origin[1];
  this->Coords[11] = this->DisplayExtent[5]*spacing[2] + origin[2];
  
  // now contiguous would require that xdim = 0 and ydim = 1
  // OR xextent = 1 pixel and xdim = 1 and ydim = 2 
  // OR xdim = 0 and ydim = 2 and yextent = i pixel
  int *ext = this->Input->GetExtent();
  if ((xdim ==0 && ydim == 1)||
      (ext[0] == ext[1] && xdim == 1) ||
      (ext[2] == ext[3] && xdim == 0 && ydim == 2))
    {
    contiguous = 1;
    }
      
  // if contiguous is it a pow of 2
  if (contiguous)
    {
    xsize = ext[xdim*2+1] - ext[xdim*2] + 1;
    // xsize and ysize must be a power of 2 in OpenGL
    xs = (unsigned short)xsize;
    while (!(xs & 0x01))
      {
      xs = xs >> 1;
      }
    if (xs == 1)
      {
      powOfTwo = 1;
      }
    }
  
  if (contiguous && powOfTwo)
    {
    // can we make y a power of two also ?
    ysize = this->DisplayExtent[ydim*2+1] - this->DisplayExtent[ydim*2] + 1;
    ys = (unsigned short)ysize;
    while (!(ys & 0x01))
      {
      ys = ys >> 1;
      }
    // yes it is a power of two already
    if (ys == 1)
      {
      release = 0;
      this->TCoords[0] = (this->DisplayExtent[xdim*2] - ext[xdim*2])/(xsize - 1.0);
      this->TCoords[1] = 0.0;  
      this->TCoords[2] = (this->DisplayExtent[xdim*2+1] - ext[xdim*2] + 1.0)/xsize;
      this->TCoords[3] = 0.0;  
      this->TCoords[4] = this->TCoords[2];
      this->TCoords[5] = 1.0;  
      this->TCoords[6] = this->TCoords[0];
      this->TCoords[7] = 1.0;  
      return (unsigned char *)
        this->Input->GetScalarPointerForExtent(this->DisplayExtent);
      }
    }
  
  // if we made it here then we must copy the data and possibly pad 
  // it as well
  release = 1;
  // find the target size
  xsize = 1;
  while (xsize < 
         this->DisplayExtent[xdim*2+1] - this->DisplayExtent[xdim*2] + 1)
    {
    xsize *= 2;
    }
  ysize = 1;
  while (ysize < 
         this->DisplayExtent[ydim*2+1] - this->DisplayExtent[ydim*2] + 1)
    {
    ysize *= 2;
    }
  
  // compute the tcoords
  this->TCoords[0] = 0.0;
  this->TCoords[1] = 0.0;  
  this->TCoords[2] = (this->DisplayExtent[xdim*2+1] - this->DisplayExtent[xdim*2] + 0.5)/xsize;
  this->TCoords[3] = 0.0;  
  this->TCoords[4] = this->TCoords[2];
  this->TCoords[5] = (this->DisplayExtent[ydim*2+1] - this->DisplayExtent[ydim*2] + 0.5)/ysize;  
  this->TCoords[6] = 0.0;
  this->TCoords[7] = this->TCoords[5];  

  // allocate the memory
  unsigned char *res = new unsigned char [ysize*xsize*numComp];
  
  // copy the input data to the memory
  int inIncX, inIncY, inIncZ;
  int idxZ, idxY, idxR;
  unsigned char *inPtr = (unsigned char *)
    this->Input->GetScalarPointerForExtent(this->DisplayExtent);
  this->Input->GetContinuousIncrements(this->DisplayExtent, 
                                       inIncX, inIncY, inIncZ);
  int rowLength = numComp*(this->DisplayExtent[1] -this->DisplayExtent[0] +1);
  unsigned char *outPtr = res;
  int outIncY, outIncZ;
  if (ydim == 2)
    {
    if (xdim == 0)
      {
      outIncZ = numComp * 
        (xsize - (this->DisplayExtent[1] - this->DisplayExtent[0] + 1));
      }
    else
      {
      outIncZ = numComp * 
        (xsize - (this->DisplayExtent[3] - this->DisplayExtent[2] + 1));
      }
    outIncY = 0;
    }
  else
    {
    outIncY = numComp * 
      (xsize - (this->DisplayExtent[1] - this->DisplayExtent[0] + 1));
    outIncZ = 0;    
    }
  
      
  for (idxZ = this->DisplayExtent[4]; idxZ <= this->DisplayExtent[5]; idxZ++)
    {
    for (idxY = this->DisplayExtent[2]; idxY <= this->DisplayExtent[3]; idxY++)
      {
      for (idxR = 0; idxR < rowLength; idxR++)
        {
        // Pixel operation
        *outPtr = *inPtr;
        outPtr++;
        inPtr++;
	}
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
  
  return res;
}

// Implement base class method.
void vtkOpenGLImageActor::Load(vtkRenderer *ren)
{
  GLenum format = GL_LUMINANCE;

  // need to reload the texture
  if (this->GetMTime() > this->LoadTime.GetMTime() ||
      this->Input->GetMTime() > this->LoadTime.GetMTime() ||
      ren->GetRenderWindow() != this->RenderWindow)
    {
    int xsize, ysize;
    int release;
    unsigned char *data = this->MakeDataSuitable(xsize,ysize, release);
    int bytesPerPixel = this->Input->GetNumberOfScalarComponents();
    GLuint tempIndex=0;

    // free any old display lists
    this->ReleaseGraphicsResources(ren->GetRenderWindow());
    this->RenderWindow = ren->GetRenderWindow();

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

#ifdef VTK_USE_QUARTZ
    ((vtkQuartzRenderWindow *)(ren->GetRenderWindow()))->RegisterTextureResource( this->Index );
#else
  #ifdef _WIN32
    ((vtkWin32OpenGLRenderWindow *)(ren->GetRenderWindow()))->RegisterTextureResource( this->Index );
  #else
    ((vtkOpenGLRenderWindow *)(ren->GetRenderWindow()))->RegisterTextureResource( this->Index );
  #endif
#endif
    
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
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP );

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
    switch (bytesPerPixel)
      {
      case 1: internalFormat = GL_LUMINANCE8; break;
      case 2: internalFormat = GL_LUMINANCE8_ALPHA8; break;
      case 3: internalFormat = GL_RGB8; break;
      case 4: internalFormat = GL_RGBA8; break;
      }
#endif
    glTexImage2D( GL_TEXTURE_2D, 0 , internalFormat,
                  xsize, ysize, 0, format, 
                  GL_UNSIGNED_BYTE, (const GLvoid *)data );
#ifndef GL_VERSION_1_1
    glEndList ();
#endif
    // modify the load time to the current time
    this->LoadTime.Modified();
    if (release)
      {
      delete [] data;
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

  // draw the quad
  glDisable(GL_COLOR_MATERIAL);
  glDisable (GL_CULL_FACE);
  glDisable( GL_LIGHTING );
  glColor3f( 1.0, 1.0, 1.0 );
  glBegin( GL_QUADS );
  for (int i = 0; i < 4; i++ )
    {
    glTexCoord2fv( this->TCoords + i*2 );
    glVertex3fv(this->Coords + i*3);
    }  
  glEnd();
  // Turn lighting back on
  glEnable( GL_LIGHTING );
}



