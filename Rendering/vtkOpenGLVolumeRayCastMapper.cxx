/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeRayCastMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <math.h>

#include "vtkOpenGLVolumeRayCastMapper.h"
#include "vtkObjectFactory.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include "vtkRenderer.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLVolumeRayCastMapper, "1.13");
vtkStandardNewMacro(vtkOpenGLVolumeRayCastMapper);
#endif

// Construct a new vtkOpenGLVolumeRayCastMapper with default values
vtkOpenGLVolumeRayCastMapper::vtkOpenGLVolumeRayCastMapper()
{
}

// Destruct a vtkOpenGLVolumeRayCastMapper - clean up any memory used
vtkOpenGLVolumeRayCastMapper::~vtkOpenGLVolumeRayCastMapper()
{
}

void vtkOpenGLVolumeRayCastMapper::RenderTexture( vtkVolume *vol, 
                                                  vtkRenderer *ren )
{
  int i;
  float offsetX, offsetY;
  float xMinOffset, xMaxOffset, yMinOffset, yMaxOffset;
  float tcoords[8];
  
  // Where should we draw the rectangle? If intermixing is on, then do it
  // at the center of the volume, otherwise do it fairly close to the near
  // near plane.
  float depthVal;
  if ( this->IntermixIntersectingGeometry )
    {
    depthVal = this->MinimumViewDistance;
    }
  else
    {
    // Pass the center of the volume through the world to view function
    // of the renderer to get the z view coordinate to use for the
    // view to world transformation of the image bounds. This way we
    // will draw the image at the depth of the center of the volume
    ren->SetWorldPoint( vol->GetCenter()[0],
                        vol->GetCenter()[1],
                        vol->GetCenter()[2],
                        1.0 );
    ren->WorldToView();
    depthVal = ren->GetViewPoint()[2];
    }
  
    // Convert the four corners of the image into world coordinates
  float verts[12];
  vtkMatrix4x4 *viewToWorldMatrix = vtkMatrix4x4::New();
  float in[4], out[4];

  // get the perspective transformation from the active camera 
  viewToWorldMatrix->DeepCopy( this->PerspectiveMatrix );
  
  // use the inverse matrix 
  viewToWorldMatrix->Invert();

  // These two values never change
  in[2] = depthVal;
  in[3] = 1.0;
  
  // This is the lower left corner
  in[0] = (float)this->ImageOrigin[0]/this->ImageViewportSize[0] * 2.0 - 1.0; 
  in[1] = (float)this->ImageOrigin[1]/this->ImageViewportSize[1] * 2.0 - 1.0; 

  viewToWorldMatrix->MultiplyPoint( in, out );
  verts[0] = out[0] / out[3];
  verts[1] = out[1] / out[3];
  verts[2] = out[2] / out[3];
  
  // This is the lower right corner
  in[0] = (float)(this->ImageOrigin[0]+this->ImageInUseSize[0]) / 
    this->ImageViewportSize[0] * 2.0 - 1.0;
  in[1] = (float)this->ImageOrigin[1]/this->ImageViewportSize[1] * 2.0 - 1.0;

  viewToWorldMatrix->MultiplyPoint( in, out );
  verts[3] = out[0] / out[3];
  verts[4] = out[1] / out[3];
  verts[5] = out[2] / out[3];

  // This is the upper right corner
  in[0] = (float)(this->ImageOrigin[0]+this->ImageInUseSize[0]) / 
    this->ImageViewportSize[0] * 2.0 - 1.0;
  in[1] = (float)(this->ImageOrigin[1]+this->ImageInUseSize[1]) / 
    this->ImageViewportSize[1] * 2.0 - 1.0;

  viewToWorldMatrix->MultiplyPoint( in, out );
  verts[6] = out[0] / out[3];
  verts[7] = out[1] / out[3];
  verts[8] = out[2] / out[3];

  // This is the upper left corner
  in[0] = (float)this->ImageOrigin[0]/this->ImageViewportSize[0] * 2.0 - 1.0;
  in[1] = (float)(this->ImageOrigin[1]+this->ImageInUseSize[1]) / 
    this->ImageViewportSize[1] * 2.0 - 1.0;

  viewToWorldMatrix->MultiplyPoint( in, out );
  verts[9]  = out[0] / out[3];
  verts[10] = out[1] / out[3];
  verts[11] = out[2] / out[3];
  
  viewToWorldMatrix->Delete();

  // Turn lighting off - the hexagon texture already has illumination in it
  glDisable( GL_LIGHTING );

  // Turn texturing on so that we can draw the textured hexagon
  glEnable( GL_TEXTURE_2D );

  // Don't write into the Zbuffer - just use it for comparisons
  glDepthMask( 0 );
  
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Specify the texture
  glColor3f(1.0,1.0,1.0);
  int newTextureSize[2];
  
#ifdef GL_VERSION_1_1
  // Test the texture to see if it fits in memory
  glTexImage2D( GL_PROXY_TEXTURE_2D, 0, GL_RGBA8, 
                this->ImageMemorySize[0], this->ImageMemorySize[1], 
                0, GL_RGBA, GL_UNSIGNED_BYTE, this->Image );
  
  GLint params[1];
  glGetTexLevelParameteriv ( GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, params ); 

  // if it does, we will render it later. define the texture here
  if ( params[0] != 0 )
    {
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 
                  this->ImageMemorySize[0], this->ImageMemorySize[1], 
                  0, GL_RGBA, GL_UNSIGNED_BYTE, this->Image );
    }
  // if it doesn't, we are going to break it up now and render it.
  // That's because we want this in the ifdef because this only works in
  // 1.1 and later.
  else
    {
    // Figure out our new texture size. Keep dividing the big one in half until
    // OpenGL says this texture is OK
    newTextureSize[0] = this->ImageMemorySize[0];
    newTextureSize[1] = this->ImageMemorySize[1];
    
    while ( params[0] == 0 && newTextureSize[0] >= 32 && newTextureSize[1] >= 32 )
      {
      if ( newTextureSize[0] > newTextureSize[1] )
        {
        newTextureSize[0] /= 2;
        }
      else
        {
        newTextureSize[1] /= 2;
        }
      
      glTexImage2D( GL_PROXY_TEXTURE_2D, 0, GL_RGBA8, 
                    newTextureSize[0], newTextureSize[1],
                    0, GL_RGBA, GL_UNSIGNED_BYTE, this->Image );      
      glGetTexLevelParameteriv ( GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, params );
      }
    
    // If we got down to 32 by 32 and OpenGL still doesn't like it, something
    // must be seriously wrong and we will ignore it. Otherwise, we have our
    // new texture size and let's start chopping up the image
    if ( newTextureSize[0] >= 32 && newTextureSize[1] >= 32 )
      {
      // How many tiles in x?
      int xLimit = 1 + static_cast<int>(
        static_cast<float>(this->ImageInUseSize[0]) / 
        static_cast<float>((newTextureSize[0]-2)));
      
      // How many tiles in y?
      int yLimit = 1 + static_cast<int>(
        static_cast<float>(this->ImageInUseSize[1]) / 
        static_cast<float>((newTextureSize[1]-2)));
      
      // Create memory for the new texture
      unsigned char *newTexture = 
        new unsigned char [newTextureSize[0] * newTextureSize[1] * 4];

      // This is the 1/2 pixel offset for texture coordinates
      offsetX = .5 / static_cast<float>(newTextureSize[0]);
      offsetY = .5 / static_cast<float>(newTextureSize[1]);
      
      int ii, jj;
      float newVerts[12];
      float vx1, vx2, vy1, vy2;
      int px1, py1, pxSize, pySize;
      
      
      // loop through the tiles in y
      for ( jj = 0; jj < yLimit; jj++ )
        {
        vy1 = static_cast<float>(jj) / static_cast<float>(yLimit);
        vy2 = static_cast<float>(jj+1) / static_cast<float>(yLimit);
        
        py1 = static_cast<int>(vy1 * static_cast<float>(
                                 this->ImageInUseSize[1]));
        pySize = static_cast<int>(2 - py1 + vy2 * static_cast<float>(
                                    this->ImageInUseSize[1]-1));
        if ( py1 + pySize > this->ImageInUseSize[1] )
          {
          pySize = this->ImageInUseSize[1] - py1;
          }
        
        yMinOffset = 2.0 * offsetY * 
          (vy1*static_cast<float>(this->ImageInUseSize[1]-1)-static_cast<float>(py1));
        
        yMaxOffset = 2.0 * offsetY * 
          (static_cast<float>(py1+pySize-1)-vy2*static_cast<float>(this->ImageInUseSize[1]-1));
        
        // loop through the tiles in x
        for ( ii = 0; ii < xLimit; ii++ )
          {
          vx1 = static_cast<float>(ii) / static_cast<float>(xLimit);
          vx2 = static_cast<float>(ii+1) / static_cast<float>(xLimit);
        
          px1 = static_cast<int>(vx1 * static_cast<float>(
                                   this->ImageInUseSize[0]);        
          pxSize = static_cast<int>(2 - px1 + vx2 * static_cast<float>(
                                      this->ImageInUseSize[0]-1));
          if ( px1 + pxSize > this->ImageInUseSize[0] )
            {
            pxSize = this->ImageInUseSize[0] - px1;
            }
          
          xMinOffset = 2.0 * offsetX * 
            (vx1*static_cast<float>(this->ImageInUseSize[0]-1) -
             static_cast<float>(px1));
          
          xMaxOffset = 2.0 * offsetX * 
            (static_cast<float>(px1+pxSize-1) -
             vx2*static_cast<float>(this->ImageInUseSize[0]-1));
          
          if ( px1 + pxSize > this->ImageInUseSize[0] )
            {
            pxSize = this->ImageInUseSize[0] - px1;
            }
          
          // copy subtexture of this->Image into newTexture
          int loop;
          for ( loop = 0; loop < pySize; loop++ )
            {
            memcpy( newTexture + 4*loop*newTextureSize[0],
                    this->Image + 4*(py1+loop)*this->ImageMemorySize[0] + 4*px1,
                    pxSize * sizeof(unsigned char) * 4 );
            }
          
          newVerts[ 0] = verts[0] + vx1*(verts[3]-verts[0]) + vy1*(verts[ 9]-verts[0]);
          newVerts[ 1] = verts[1] + vx1*(verts[4]-verts[1]) + vy1*(verts[10]-verts[1]);
          newVerts[ 2] = verts[2] + vx1*(verts[5]-verts[2]) + vy1*(verts[11]-verts[2]);

          newVerts[ 3] = verts[0] + vx2*(verts[3]-verts[0]) + vy1*(verts[ 9]-verts[0]);
          newVerts[ 4] = verts[1] + vx2*(verts[4]-verts[1]) + vy1*(verts[10]-verts[1]);
          newVerts[ 5] = verts[2] + vx2*(verts[5]-verts[2]) + vy1*(verts[11]-verts[2]);

          newVerts[ 6] = verts[0] + vx2*(verts[3]-verts[0]) + vy2*(verts[ 9]-verts[0]);
          newVerts[ 7] = verts[1] + vx2*(verts[4]-verts[1]) + vy2*(verts[10]-verts[1]);
          newVerts[ 8] = verts[2] + vx2*(verts[5]-verts[2]) + vy2*(verts[11]-verts[2]);
          
          newVerts[ 9] = verts[0] + vx1*(verts[3]-verts[0]) + vy2*(verts[ 9]-verts[0]);
          newVerts[10] = verts[1] + vx1*(verts[4]-verts[1]) + vy2*(verts[10]-verts[1]);
          newVerts[11] = verts[2] + vx1*(verts[5]-verts[2]) + vy2*(verts[11]-verts[2]);
          
          tcoords[0]  = offsetX + xMinOffset;
          tcoords[1]  = offsetY + yMinOffset;
          tcoords[2]  = (float)pxSize/(float)newTextureSize[0] - offsetX - xMaxOffset;
          tcoords[3]  = offsetY + yMinOffset;
          tcoords[4]  = (float)pxSize/(float)newTextureSize[0] - offsetX - xMaxOffset;
          tcoords[5]  = (float)pySize/(float)newTextureSize[1] - offsetY - yMaxOffset;
          tcoords[6]  = offsetX + xMaxOffset;
          tcoords[7]  = (float)pySize/(float)newTextureSize[1] - offsetY - yMaxOffset;

          glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 
                        newTextureSize[0], newTextureSize[1],
                        0, GL_RGBA, GL_UNSIGNED_BYTE, newTexture );      

          // Render the polygon
          glBegin( GL_POLYGON );
          
          for ( i = 0; i < 4; i++ )
            {
            glTexCoord2fv( tcoords+i*2 );
            glVertex3fv( newVerts+i*3 );
            }
          
          glEnd();
          }
        }
      
      // Delete the memory we created
      delete [] newTexture;
      }
    
    
    glDisable( GL_TEXTURE_2D );
    
    glDisable( GL_ALPHA_TEST );
    glDepthMask( 1 );
    
    // Turn lighting back on
    glEnable( GL_LIGHTING );  
    
    return;
    }
  
#else
  glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                this->ImageMemorySize[0], this->ImageMemorySize[1], 
                0, GL_RGBA, GL_UNSIGNED_BYTE, this->Image );
#endif
  offsetX = .5 / static_cast<float>(this->ImageMemorySize[0]);
  offsetY = .5 / static_cast<float>(this->ImageMemorySize[1]);
  
  tcoords[0]  = 0.0 + offsetX;
  tcoords[1]  = 0.0 + offsetY;
  tcoords[2]  = 
    (float)this->ImageInUseSize[0]/(float)this->ImageMemorySize[0] - offsetX;
  tcoords[3]  = offsetY;
  tcoords[4]  = 
    (float)this->ImageInUseSize[0]/(float)this->ImageMemorySize[0] - offsetX;
  tcoords[5]  = 
    (float)this->ImageInUseSize[1]/(float)this->ImageMemorySize[1] - offsetY;
  tcoords[6]  = offsetX;
  tcoords[7]  = 
    (float)this->ImageInUseSize[1]/(float)this->ImageMemorySize[1] - offsetY;
  
  // Render the polygon
  glBegin( GL_POLYGON );
  
  for ( i = 0; i < 4; i++ )
    {
    glTexCoord2fv( tcoords+i*2 );
    glVertex3fv( verts+i*3 );
    }
  glEnd();

  glDisable( GL_ALPHA_TEST );
  glDisable( GL_TEXTURE_2D );
  glDepthMask( 1 );

  // Turn lighting back on
  glEnable( GL_LIGHTING );  
}

