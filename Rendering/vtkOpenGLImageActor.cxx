/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLImageActor.h"

#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenGLRenderWindow.h"

#include <math.h>

#include "vtkOpenGL.h"
#include "vtkgl.h" // vtkgl namespace

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLImageActor);
#endif

// Initializes an instance, generates a unique index.
vtkOpenGLImageActor::vtkOpenGLImageActor()
{
  this->Index = 0;
  this->RenderWindow = 0;
  this->TextureSize[0] = 0;
  this->TextureSize[1] = 0;
}

vtkOpenGLImageActor::~vtkOpenGLImageActor()
{
  this->RenderWindow = NULL;
}

// Release the graphics resources used by this texture.  
void vtkOpenGLImageActor::ReleaseGraphicsResources(vtkWindow *renWin)
{
  if (this->Index && renWin && renWin->GetMapped())
    {
    static_cast<vtkRenderWindow *>(renWin)->MakeCurrent();
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
    this->TextureSize[0] = 0;
    this->TextureSize[1] = 0;
    }
  this->Index = 0;
  this->RenderWindow = NULL;
  this->Modified();
}

unsigned char *vtkOpenGLImageActor::MakeDataSuitable(int &xsize, int &ysize,
                                                     int &release,
                                                     int &reuseTexture)
{
  int contiguous = 0;
  unsigned short xs,ys;
  int powOfTwo = 0;
  int numComp = this->Input->GetNumberOfScalarComponents();
  int xdim, ydim;

  reuseTexture = 0;
  
  // it must be a power of two and contiguous
  // find the two used dimensions
  // this assumes a 2D image, no lines here folk
  if (this->ComputedDisplayExtent[0] != this->ComputedDisplayExtent[1])
    {
    xdim = 0;
    if (this->ComputedDisplayExtent[2] != this->ComputedDisplayExtent[3])
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
  
  double *spacing = this->Input->GetSpacing();
  double *origin = this->Input->GetOrigin();
  
  // compute the world coordinates
  this->Coords[0] = this->ComputedDisplayExtent[0]*spacing[0] + origin[0];
  this->Coords[1] = this->ComputedDisplayExtent[2]*spacing[1] + origin[1];
  this->Coords[2] = this->ComputedDisplayExtent[4]*spacing[2] + origin[2];
  this->Coords[3] = this->ComputedDisplayExtent[1]*spacing[0] + origin[0];
  this->Coords[4] = 
    this->ComputedDisplayExtent[2 + (xdim == 1)]*spacing[1] + origin[1];
  this->Coords[5] = this->ComputedDisplayExtent[4]*spacing[2] + origin[2];
  this->Coords[6] = this->ComputedDisplayExtent[1]*spacing[0] + origin[0];
  this->Coords[7] = this->ComputedDisplayExtent[3]*spacing[1] + origin[1];
  this->Coords[8] = this->ComputedDisplayExtent[5]*spacing[2] + origin[2];
  this->Coords[9] = this->ComputedDisplayExtent[0]*spacing[0] + origin[0];
  this->Coords[10] = 
    this->ComputedDisplayExtent[2 + (ydim == 1)]*spacing[1] + origin[1];
  this->Coords[11] = this->ComputedDisplayExtent[5]*spacing[2] + origin[2];
  
  // now contiguous would require that xdim = 0 and ydim = 1
  // OR xextent = 1 pixel and xdim = 1 and ydim = 2 
  // OR xdim = 0 and ydim = 2 and yextent = i pixel. In addition
  // the corresponding x display extents must match the 
  // extent of the data
  int *ext = this->Input->GetExtent();
  
  if ( ( xdim == 0 && ydim == 1 && 
         this->ComputedDisplayExtent[0] == ext[0] && 
         this->ComputedDisplayExtent[1] == ext[1] )||
       ( ext[0] == ext[1] && xdim == 1 && 
         this->ComputedDisplayExtent[2] == ext[2] && 
         this->ComputedDisplayExtent[3] == ext[3] ) ||
       ( ext[2] == ext[3] && xdim == 0 && ydim == 2 &&
         this->ComputedDisplayExtent[0] == ext[0] && 
         this->ComputedDisplayExtent[1] == ext[1] ) )
    {
    contiguous = 1;
    }
      
  // if contiguous is it a pow of 2
  if (contiguous)
    {
    xsize = ext[xdim*2+1] - ext[xdim*2] + 1;
    // xsize and ysize must be a power of 2 in OpenGL
    xs = static_cast<unsigned short>(xsize);
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
    ysize = (this->ComputedDisplayExtent[ydim*2+1] -
             this->ComputedDisplayExtent[ydim*2] + 1);
    ys = static_cast<unsigned short>(ysize);
    while (!(ys & 0x01))
      {
      ys = ys >> 1;
      }
    // yes it is a power of two already
    if (ys == 1)
      {
      release = 0;
      this->TCoords[0] = (this->ComputedDisplayExtent[xdim*2] - 
                          ext[xdim*2] + 0.5)/xsize;
      this->TCoords[1] = 0.5/ysize;  
      this->TCoords[2] = (this->ComputedDisplayExtent[xdim*2+1] -
                          ext[xdim*2] + 0.5)/xsize;
      this->TCoords[3] = this->TCoords[1];  
      this->TCoords[4] = this->TCoords[2];
      this->TCoords[5] = 1.0 - 0.5/ysize;  
      this->TCoords[6] = this->TCoords[0];
      this->TCoords[7] = this->TCoords[5];

#ifdef GL_VERSION_1_1
      // if texture size hasn't changed, reuse old texture
      if (xsize == this->TextureSize[0] && ysize == this->TextureSize[1])
        {
        reuseTexture = 1;
        }
#endif
      return static_cast<unsigned char *>(
        this->Input->GetScalarPointerForExtent(this->ComputedDisplayExtent));
      }
    }
  
  // if we made it here then we must copy the data and possibly pad 
  // it as well

  // find the target size
  xsize = 1;
  while (xsize <
         this->ComputedDisplayExtent[xdim*2+1] -
         this->ComputedDisplayExtent[xdim*2] + 1)
    {
    xsize *= 2;
    }
  ysize = 1;
  while (ysize < 
         this->ComputedDisplayExtent[ydim*2+1] -
         this->ComputedDisplayExtent[ydim*2] + 1)
    {
    ysize *= 2;
    }
  
  // compute the tcoords
  this->TCoords[0] = 0.5/xsize;
  this->TCoords[1] = 0.5/ysize;  
  this->TCoords[2] = (this->ComputedDisplayExtent[xdim*2+1] -
                      this->ComputedDisplayExtent[xdim*2] + 0.5)/xsize;
  this->TCoords[3] = this->TCoords[1];  
  this->TCoords[4] = this->TCoords[2];
  this->TCoords[5] = (this->ComputedDisplayExtent[ydim*2+1] -
                      this->ComputedDisplayExtent[ydim*2] + 0.5)/ysize;  
  this->TCoords[6] = this->TCoords[0];
  this->TCoords[7] = this->TCoords[5];  

#ifdef GL_VERSION_1_1
  // reuse texture if texture size has not changed
  if (xsize == this->TextureSize[0] && ysize == this->TextureSize[1])
    {
    reuseTexture = 1;
    xsize = this->ComputedDisplayExtent[xdim*2+1] -
      this->ComputedDisplayExtent[xdim*2] + 1;
    ysize = this->ComputedDisplayExtent[ydim*2+1] -
      this->ComputedDisplayExtent[ydim*2] + 1;
    }
#endif

  // if contiguous and texture size hasn't changed, don't copy or pad
  if (reuseTexture && contiguous)
    {
    release = 0;
    return static_cast<unsigned char *>(
      this->Input->GetScalarPointerForExtent(this->ComputedDisplayExtent));
    }

  // allocate the memory
  unsigned char *res = new unsigned char [ysize*xsize*numComp];
  release = 1;
  
  // copy the input data to the memory
  vtkIdType inIncX, inIncY, inIncZ;
  int idxZ, idxY, idxR;
  unsigned char *inPtr = static_cast<unsigned char *>(
    this->Input->GetScalarPointerForExtent(this->ComputedDisplayExtent));
  this->Input->GetContinuousIncrements(this->ComputedDisplayExtent, 
                                       inIncX, inIncY, inIncZ);
  int rowLength = numComp*(this->ComputedDisplayExtent[1] -
                           this->ComputedDisplayExtent[0] +1);
  unsigned char *outPtr = res;
  vtkIdType outIncY, outIncZ;
  if (ydim == 2)
    {
    if (xdim == 0)
      {
      outIncZ = numComp * 
        (xsize - (this->ComputedDisplayExtent[1] -
                  this->ComputedDisplayExtent[0] + 1));
      }
    else
      {
      outIncZ = numComp * 
        (xsize - (this->ComputedDisplayExtent[3] -
                  this->ComputedDisplayExtent[2] + 1));
      }
    outIncY = 0;
    }
  else
    {
    outIncY = numComp * 
      (xsize - (this->ComputedDisplayExtent[1] -
                this->ComputedDisplayExtent[0] + 1));
    outIncZ = 0;    
    }
  
      
  for (idxZ = this->ComputedDisplayExtent[4];
       idxZ <= this->ComputedDisplayExtent[5]; idxZ++)
    {
    for (idxY = this->ComputedDisplayExtent[2];
         idxY <= this->ComputedDisplayExtent[3]; idxY++)
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
      ren->GetRenderWindow() != this->RenderWindow ||
      static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow())->GetContextCreationTime() >
      this->LoadTime)
    {
    int xsize, ysize;
    int release, reuseTexture;
    unsigned char *data = this->MakeDataSuitable(xsize,ysize,
                                                 release, reuseTexture);
    int bytesPerPixel = this->Input->GetNumberOfScalarComponents();
    GLuint tempIndex=0;

    if (reuseTexture)
      {
#ifdef GL_VERSION_1_1
      glBindTexture(GL_TEXTURE_2D, this->Index);
#endif
      }
    else
      {
      // free any old display lists
      this->ReleaseGraphicsResources(ren->GetRenderWindow());
      this->RenderWindow = ren->GetRenderWindow();

      // define a display list for this texture
      // get a unique display list id
#ifdef GL_VERSION_1_1
      glGenTextures(1, &tempIndex);
      this->Index = static_cast<long>(tempIndex);
      glBindTexture(GL_TEXTURE_2D, this->Index);
#else
      this->Index = glGenLists(1);
      glDeleteLists (static_cast<GLuint>(this->Index),
                     static_cast<GLsizei>(0));
      glNewList (static_cast<GLuint>(this->Index), GL_COMPILE);
#endif

      static_cast<vtkOpenGLRenderWindow *>(ren->GetRenderWindow())
        ->RegisterTextureResource( this->Index );
      }
    
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

    if (reuseTexture)
      {
#ifdef GL_VERSION_1_1
      glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
      glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);
      glTexSubImage2D(GL_TEXTURE_2D, 0,
                      0, 0, xsize, ysize, format, 
                      GL_UNSIGNED_BYTE,
                      static_cast<const GLvoid *>(data));
#endif
      }
    else
      {
      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                   xsize, ysize, 0, format, 
                   GL_UNSIGNED_BYTE, static_cast<const GLvoid *>(data));
      this->TextureSize[0] = xsize;
      this->TextureSize[1] = ysize;
      }

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
  glCallList(static_cast<GLuint>(this->Index));
#endif
  
  // don't accept fragments if they have zero opacity. this will stop the
  // zbuffer from be blocked by totally transparent texture fragments.
  glAlphaFunc (GL_GREATER, static_cast<GLclampf>(0));
  glEnable (GL_ALPHA_TEST);

  // now bind it 
  glEnable(GL_TEXTURE_2D);
  
  GLint uUseTexture=-1;
  GLint uTexture=-1;
  
  vtkOpenGLRenderer *oRenderer=static_cast<vtkOpenGLRenderer *>(ren);
  
  if(oRenderer->GetDepthPeelingHigherLayer())
    {
    uUseTexture=oRenderer->GetUseTextureUniformVariable();
    uTexture=oRenderer->GetTextureUniformVariable();
    vtkgl::Uniform1i(uUseTexture,1);
    vtkgl::Uniform1i(uTexture,0); // active texture 0
    }

  
  // draw the quad
  if ( vtkMapper::GetResolveCoincidentTopology() )
    {
    if ( vtkMapper::GetResolveCoincidentTopology() == 
         VTK_RESOLVE_SHIFT_ZBUFFER )
      {
      }
    else
      {
#ifdef GL_VERSION_1_1
      double f, u;
      glEnable(GL_POLYGON_OFFSET_FILL);
      vtkMapper::GetResolveCoincidentTopologyPolygonOffsetParameters(f,u);
      glPolygonOffset(f,u);
#endif      
      }
    }
  glDisable(GL_COLOR_MATERIAL);
  glDisable (GL_CULL_FACE);
  glDisable( GL_LIGHTING );
  glColor4f( 1.0, 1.0, 1.0, this->Opacity );
  glBegin( GL_QUADS );
  for (int i = 0; i < 4; i++ )
    {
    glTexCoord2dv( this->TCoords + i*2 );
    glVertex3dv(this->Coords + i*3);
    }  
  glEnd();
  // Turn lighting back on
  glEnable( GL_LIGHTING );
}

// Determine if a given texture size is supported by the
// video card
int vtkOpenGLImageActor::TextureSizeOK( int size[2] )
{
  // In version 1.1 or later, use proxy texture to figure out if
  // the texture is too big
#ifdef GL_VERSION_1_1
  
  GLint maxSize;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxSize);
  
  // Do a quick test to see if we are too large
  if ( size[0] > maxSize ||
       size[1] > maxSize )
    {
    return 0;
    }
  
  // Test the texture to see if it fits in memory
  glTexImage2D( GL_PROXY_TEXTURE_2D, 0, GL_RGBA8, 
                size[0], size[1],
                0, GL_RGBA, GL_UNSIGNED_BYTE,NULL );
  
  GLint params = 0;
  glGetTexLevelParameteriv ( GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,
                             &params ); 

  // if it does, we will render it later. define the texture here
  if ( params == 0 )
    {
    // Can't use that texture
    return 0;
    }
  else
    {
    return 1;
    }
#else
  
  // Otherwise we are version 1.0 and we'll just assume the card
  // can do 1024x1024
  if ( size[0] > 1024 || size[1] > 1024 ) 
    {
    return 0;
    }
  else
    {
    return 1;
    }
#endif
}


// Actual actor render method.
// Recursive to handle larger textures than can be rendered by
// a given video card. Assumes all video cards can render a texture
// of 256x256 so will fail if card reports that it cannot render
// a texture of this size rather than recursing further
void vtkOpenGLImageActor::Render(vtkRenderer *ren)
{
  glPushAttrib( GL_ENABLE_BIT );
  
  // Save the current display extent since we might change it
  int savedDisplayExtent[6];
  this->GetDisplayExtent( savedDisplayExtent );
 
  // What is the power of two texture big enough to fit the display extent?
  // This should be 1 in some direction.
  int i;
  int pow2[3] = {1,1,1};
  int baseSize[3];
  for ( i = 0; i < 3; i++ )
    {
    baseSize[i] = (this->ComputedDisplayExtent[i*2+1] -
                   this->ComputedDisplayExtent[i*2] + 1);
    while( pow2[i] < baseSize[i] )
      {
      pow2[i] *= 2;
      }
    }
  
  // Find the 2d texture in the 3d pow2 structure
  int size[2];
  if ( pow2[0] == 1 )
    {
    size[0] = pow2[1];
    size[1] = pow2[2];
    }
  else if ( pow2[1] == 1 )
    {
    size[0] = pow2[0];
    size[1] = pow2[2];
    }
  else
    {
    size[0] = pow2[0];
    size[1] = pow2[1];
    }
  
  // Check if we can fit this texture in memory
  if ( this->TextureSizeOK(size) )
    {
    // We can fit it - render
    this->InternalRender( ren );
    }
  else
    {
    // If we can't handle a 256x256 or smaller texture,
    // just give up and don't render anything. Something
    // must be horribly wrong...
    if ( size[0] <= 256 && size[1] <= 256 )
      {
      return;
      }
    
    // We can't fit it - subdivide
    int newDisplayExtent[6];
    int idx;

    // Find the biggest side
    if ( baseSize[0] >= baseSize[1] && baseSize[0] >= baseSize[2] )
      {
      idx = 0;
      }
    else if ( baseSize[1] >= baseSize[0] && baseSize[1] >= baseSize[2] )
      {
      idx = 1;
      }
    else 
      {
      idx = 2;
      }
    
    // For the other two sides, just copy in the display extent
    for ( i = 0; i < 3; i++ )
      {
      if ( i != idx )
        {
        newDisplayExtent[i*2] = this->ComputedDisplayExtent[i*2];
        newDisplayExtent[i*2+1] = this->ComputedDisplayExtent[i*2+1];        
        }
      }
    
    // For the biggest side - divide the power of two size in 1/2
    // This is the first half
    int tempDisplayExtent = this->ComputedDisplayExtent[idx*2+1];
    newDisplayExtent[idx*2] = this->ComputedDisplayExtent[idx*2];
    newDisplayExtent[idx*2+1] = (newDisplayExtent[idx*2] +
                                 baseSize[idx]/2 - 1);
    
    // Set it as the display extent and render
    this->SetDisplayExtent( newDisplayExtent );
    this->Render(ren);

    // This is the remaining side (since the display extent is not 
    // necessarily a power of 2, this is likely to be less than half
    newDisplayExtent[idx*2] = (this->ComputedDisplayExtent[idx*2] +
                               baseSize[idx]/2 - 1);
    newDisplayExtent[idx*2+1] = tempDisplayExtent;
    
    // Set it as the display extent and render
    this->SetDisplayExtent( newDisplayExtent );
    this->Render(ren);
    }
  
  // Restore the old display extent
  this->SetDisplayExtent( savedDisplayExtent ); 
  
  glPopAttrib();
}

// This is the non-recursive render that will not check the
// size of the image (it has already been determined to
// be fine)
void vtkOpenGLImageActor::InternalRender(vtkRenderer *ren)
{
  // for picking
  glDepthMask (GL_TRUE);

  // build transformation 
  if (!this->IsIdentity)
    {
    double *mat = this->GetMatrix()->Element[0];
    double mat2[16];
    mat2[0] = mat[0];
    mat2[1] = mat[4];
    mat2[2] = mat[8];
    mat2[3] = mat[12];
    mat2[4] = mat[1];
    mat2[5] = mat[5];
    mat2[6] = mat[9];
    mat2[7] = mat[13];
    mat2[8] = mat[2];
    mat2[9] = mat[6];
    mat2[10] = mat[10];
    mat2[11] = mat[14];
    mat2[12] = mat[3];
    mat2[13] = mat[7];
    mat2[14] = mat[11];
    mat2[15] = mat[15];
    
    // insert model transformation 
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixd(mat2);
    }
  
  // Render the texture
  this->Load(ren);

  // pop transformation matrix
  if (!this->IsIdentity)
    {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    }
}

void vtkOpenGLImageActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
