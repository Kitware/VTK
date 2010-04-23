/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRayCastImageDisplayHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLRayCastImageDisplayHelper.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkCamera.h"
#include "vtkFixedPointRayCastImage.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
# include "vtkOpenGL.h"
#endif

#include <math.h>

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLRayCastImageDisplayHelper);
#endif

// Construct a new vtkOpenGLRayCastImageDisplayHelper with default values
vtkOpenGLRayCastImageDisplayHelper::vtkOpenGLRayCastImageDisplayHelper()
{
}

// Destruct a vtkOpenGLRayCastImageDisplayHelper - clean up any memory used
vtkOpenGLRayCastImageDisplayHelper::~vtkOpenGLRayCastImageDisplayHelper()
{
}

// imageMemorySize   is how big the texture is - this is always a power of two
//
// imageViewportSize is how big the renderer viewport is in pixels
//
// imageInUseSize    is the rendered image - equal or smaller than imageMemorySize
//                   and imageViewportSize
// 
// imageOrigin       is the starting pixel of the imageInUseSize image on the
//                   the imageViewportSize viewport
//
void vtkOpenGLRayCastImageDisplayHelper::RenderTexture( vtkVolume *vol, 
                                                        vtkRenderer *ren,
                                                        vtkFixedPointRayCastImage *image,
                                                        float requestedDepth )
{
  this->RenderTextureInternal( vol, ren, image->GetImageMemorySize(), image->GetImageViewportSize(),
                       image->GetImageInUseSize(), image->GetImageOrigin(),
                       requestedDepth, VTK_UNSIGNED_SHORT, image->GetImage() );
}

void vtkOpenGLRayCastImageDisplayHelper::RenderTexture( vtkVolume *vol, 
                                                        vtkRenderer *ren,
                                                        int imageMemorySize[2],
                                                        int imageViewportSize[2],
                                                        int imageInUseSize[2],
                                                        int imageOrigin[2],
                                                        float requestedDepth,
                                                        unsigned char *image )
{
  this->RenderTextureInternal( vol, ren, imageMemorySize, imageViewportSize,
                               imageInUseSize, imageOrigin, requestedDepth,
                               VTK_UNSIGNED_CHAR, static_cast<void *>(image) );
}

void vtkOpenGLRayCastImageDisplayHelper::RenderTexture( vtkVolume *vol, 
                                                        vtkRenderer *ren,
                                                        int imageMemorySize[2],
                                                        int imageViewportSize[2],
                                                        int imageInUseSize[2],
                                                        int imageOrigin[2],
                                                        float requestedDepth,
                                                        unsigned short *image )
{
  this->RenderTextureInternal( vol, ren, imageMemorySize, imageViewportSize,
                               imageInUseSize, imageOrigin, requestedDepth,
                               VTK_UNSIGNED_SHORT, static_cast<void *>(image) );
}

void vtkOpenGLRayCastImageDisplayHelper::RenderTextureInternal( vtkVolume *vol, 
                                                                vtkRenderer *ren,
                                                                int imageMemorySize[2],
                                                                int imageViewportSize[2],
                                                                int imageInUseSize[2],
                                                                int imageOrigin[2],
                                                                float requestedDepth,
                                                                int imageScalarType,
                                                                void *image )
{
  int i;
  float offsetX, offsetY;
  float xMinOffset, xMaxOffset, yMinOffset, yMaxOffset;
  float tcoords[8];
  
  float depth;
  if ( requestedDepth > 0.0 && requestedDepth <= 1.0 )
    {
    depth = requestedDepth;
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
    depth = ren->GetViewPoint()[2];
    }
  
    // Convert the four corners of the image into world coordinates
  float verts[12];
  vtkMatrix4x4 *viewToWorldMatrix = vtkMatrix4x4::New();
  float in[4], out[4];

  vtkCamera *cam = ren->GetActiveCamera();
  ren->ComputeAspect(); 
  double *aspect = ren->GetAspect();
  
  vtkTransform *perspectiveTransform = vtkTransform::New();
  perspectiveTransform->Identity();
  perspectiveTransform->Concatenate(
    cam->GetProjectionTransformMatrix(aspect[0]/aspect[1], 
                                      0.0, 1.0 ));
  perspectiveTransform->Concatenate(cam->GetViewTransformMatrix());
  
  // get the perspective transformation from the active camera 
  viewToWorldMatrix->DeepCopy( perspectiveTransform->GetMatrix() );
  perspectiveTransform->Delete();
  
  // use the inverse matrix 
  viewToWorldMatrix->Invert();

  // These two values never change
  in[2] = depth;
  in[3] = 1.0;
  
  // This is the lower left corner
  in[0] = (float)imageOrigin[0]/imageViewportSize[0] * 2.0 - 1.0; 
  in[1] = (float)imageOrigin[1]/imageViewportSize[1] * 2.0 - 1.0; 

  viewToWorldMatrix->MultiplyPoint( in, out );
  verts[0] = out[0] / out[3];
  verts[1] = out[1] / out[3];
  verts[2] = out[2] / out[3];
  
  // This is the lower right corner
  in[0] = (float)(imageOrigin[0]+imageInUseSize[0]) / 
    imageViewportSize[0] * 2.0 - 1.0;
  in[1] = (float)imageOrigin[1]/imageViewportSize[1] * 2.0 - 1.0;

  viewToWorldMatrix->MultiplyPoint( in, out );
  verts[3] = out[0] / out[3];
  verts[4] = out[1] / out[3];
  verts[5] = out[2] / out[3];

  // This is the upper right corner
  in[0] = (float)(imageOrigin[0]+imageInUseSize[0]) / 
    imageViewportSize[0] * 2.0 - 1.0;
  in[1] = (float)(imageOrigin[1]+imageInUseSize[1]) / 
    imageViewportSize[1] * 2.0 - 1.0;

  viewToWorldMatrix->MultiplyPoint( in, out );
  verts[6] = out[0] / out[3];
  verts[7] = out[1] / out[3];
  verts[8] = out[2] / out[3];

  // This is the upper left corner
  in[0] = (float)imageOrigin[0]/imageViewportSize[0] * 2.0 - 1.0;
  in[1] = (float)(imageOrigin[1]+imageInUseSize[1]) / 
    imageViewportSize[1] * 2.0 - 1.0;

  viewToWorldMatrix->MultiplyPoint( in, out );
  verts[9]  = out[0] / out[3];
  verts[10] = out[1] / out[3];
  verts[11] = out[2] / out[3];
  
  viewToWorldMatrix->Delete();

  // Save state
  glPushAttrib(GL_ENABLE_BIT         | 
               GL_COLOR_BUFFER_BIT   |
               GL_STENCIL_BUFFER_BIT |
               GL_DEPTH_BUFFER_BIT   | 
               GL_POLYGON_BIT        | 
               GL_PIXEL_MODE_BIT     |
               GL_TEXTURE_BIT);
  
  glPixelTransferf( GL_RED_SCALE,    this->PixelScale );
  glPixelTransferf( GL_GREEN_SCALE,  this->PixelScale );
  glPixelTransferf( GL_BLUE_SCALE,   this->PixelScale );
  glPixelTransferf( GL_ALPHA_SCALE,  this->PixelScale );
  
  if ( this->PreMultipliedColors )
    {
    // Values in the texture map have already been pre-multiplied by alpha
    glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
    }
  else
    {
    // Values in the texture map have not been pre-multiplied by alpha
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    }
  
  // Turn lighting off - the texture already has illumination in it
  glDisable( GL_LIGHTING );

  // Turn texturing on so that we can draw the textured hexagon
  glEnable( GL_TEXTURE_2D );
  
#ifdef GL_VERSION_1_1
  GLuint tempIndex;
  glGenTextures(1, &tempIndex);
  glBindTexture(GL_TEXTURE_2D, tempIndex);
#endif
  
  // Don't write into the Zbuffer - just use it for comparisons
  glDepthMask( 0 );
  
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Specify the texture
  glColor3f(1.0,1.0,1.0);
  int newTextureSize[2];
  
#ifdef GL_VERSION_1_1
  if ( imageScalarType == VTK_UNSIGNED_CHAR )
    {
    // Test the texture to see if it fits in memory
    glTexImage2D( GL_PROXY_TEXTURE_2D, 0, GL_RGBA8, 
                  imageMemorySize[0], imageMemorySize[1], 
                  0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  static_cast<unsigned char *>(image) );
    }
  else
    {
    // Test the texture to see if it fits in memory
    glTexImage2D( GL_PROXY_TEXTURE_2D, 0, GL_RGBA8, 
                  imageMemorySize[0], imageMemorySize[1], 
                  0, GL_RGBA, GL_UNSIGNED_SHORT, 
                  static_cast<unsigned short *>(image) );
    }
  
  GLint params[1];
  glGetTexLevelParameteriv ( GL_PROXY_TEXTURE_2D, 0, 
                             GL_TEXTURE_WIDTH, params ); 

  // if it does, we will render it later. define the texture here
  if ( params[0] != 0 )
    {
    if ( imageScalarType == VTK_UNSIGNED_CHAR )
      {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 
                    imageMemorySize[0], imageMemorySize[1], 
                    0, GL_RGBA, GL_UNSIGNED_BYTE, 
                    static_cast<unsigned char *>(image) );
      }
    else
      {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 
                    imageMemorySize[0], imageMemorySize[1], 
                    0, GL_RGBA, GL_UNSIGNED_SHORT, 
                    static_cast<unsigned short *>(image) );
      }
    }
  // if it doesn't, we are going to break it up now and render it.
  // That's because we want this in the ifdef because this only works in
  // 1.1 and later.
  else
    {
    // Figure out our new texture size. Keep dividing the big one in half until
    // OpenGL says this texture is OK
    newTextureSize[0] = imageMemorySize[0];
    newTextureSize[1] = imageMemorySize[1];
    
    while ( params[0] == 0 && newTextureSize[0] >= 32 && 
            newTextureSize[1] >= 32 )
      {
      if ( newTextureSize[0] > newTextureSize[1] )
        {
        newTextureSize[0] /= 2;
        }
      else
        {
        newTextureSize[1] /= 2;
        }
      
      if ( imageScalarType == VTK_UNSIGNED_CHAR )
        {
        glTexImage2D( GL_PROXY_TEXTURE_2D, 0, GL_RGBA8, 
                      newTextureSize[0], newTextureSize[1],
                      0, GL_RGBA, GL_UNSIGNED_BYTE, 
                      static_cast<unsigned char *>(image) );
        }
      else
        {
        glTexImage2D( GL_PROXY_TEXTURE_2D, 0, GL_RGBA8, 
                      newTextureSize[0], newTextureSize[1],
                      0, GL_RGBA, GL_UNSIGNED_SHORT, 
                      static_cast<unsigned short *>(image) );
        }
      glGetTexLevelParameteriv ( GL_PROXY_TEXTURE_2D, 0, 
                                 GL_TEXTURE_WIDTH, params );
      }
    
    // If we got down to 32 by 32 and OpenGL still doesn't like it, something
    // must be seriously wrong and we will ignore it. Otherwise, we have our
    // new texture size and let's start chopping up the image
    if ( newTextureSize[0] >= 32 && newTextureSize[1] >= 32 )
      {
      // How many tiles in x?
      int xLimit = 1 + static_cast<int>(
        static_cast<float>(imageInUseSize[0]) / 
        static_cast<float>((newTextureSize[0]-2)));
      
      // How many tiles in y?
      int yLimit = 1 + static_cast<int>(
        static_cast<float>(imageInUseSize[1]) / 
        static_cast<float>((newTextureSize[1]-2)));
      
      // Create memory for the new texture
      unsigned char  *newTextureChar  = NULL;
      unsigned short *newTextureShort = NULL;
      
      if ( imageScalarType == VTK_UNSIGNED_CHAR )
        {
        newTextureChar = 
          new unsigned char [newTextureSize[0] * newTextureSize[1] * 4];
        }
      else
        {
        newTextureShort = 
          new unsigned short [newTextureSize[0] * newTextureSize[1] * 4];
        }
      
      
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
                                 imageInUseSize[1]));
        pySize = static_cast<int>(2 - py1 + vy2 * static_cast<float>(
                                    imageInUseSize[1]-1));
        if ( py1 + pySize > imageInUseSize[1] )
          {
          pySize = imageInUseSize[1] - py1;
          }
        
        yMinOffset = 2.0 * offsetY * 
          (vy1*static_cast<float>(imageInUseSize[1]-1)-static_cast<float>(py1));
        
        yMaxOffset = 2.0 * offsetY * 
          (static_cast<float>(py1+pySize-1)-vy2*static_cast<float>(imageInUseSize[1]-1));
        
        // loop through the tiles in x
        for ( ii = 0; ii < xLimit; ii++ )
          {
          vx1 = static_cast<float>(ii) / static_cast<float>(xLimit);
          vx2 = static_cast<float>(ii+1) / static_cast<float>(xLimit);
        
          px1 = static_cast<int>(vx1 * static_cast<float>(
                                   imageInUseSize[0])); 
          pxSize = static_cast<int>(2 - px1 + vx2 * static_cast<float>(
                                      imageInUseSize[0]-1));
          if ( px1 + pxSize > imageInUseSize[0] )
            {
            pxSize = imageInUseSize[0] - px1;
            }
          
          xMinOffset = 2.0 * offsetX * 
            (vx1*static_cast<float>(imageInUseSize[0]-1) -
             static_cast<float>(px1));
          
          xMaxOffset = 2.0 * offsetX * 
            (static_cast<float>(px1+pxSize-1) -
             vx2*static_cast<float>(imageInUseSize[0]-1));
          
          if ( px1 + pxSize > imageInUseSize[0] )
            {
            pxSize = imageInUseSize[0] - px1;
            }
          
          // copy subtexture of image into newTexture
          int loop;
          for ( loop = 0; loop < pySize; loop++ )
            {
            if ( imageScalarType == VTK_UNSIGNED_CHAR )
              {
              memcpy( newTextureChar + 4*loop*newTextureSize[0],
                      static_cast<unsigned char *>(image) + 
                      4*(py1+loop)*imageMemorySize[0] + 4*px1,
                      pxSize * sizeof(unsigned char) * 4 );
              }
            else
              {
              memcpy( newTextureShort + 4*loop*newTextureSize[0],
                      static_cast<unsigned short *>(image) + 
                      4*(py1+loop)*imageMemorySize[0] + 4*px1,
                      pxSize * sizeof(unsigned short) * 4 );
              }
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

          if ( imageScalarType == VTK_UNSIGNED_CHAR )
            {
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 
                          newTextureSize[0], newTextureSize[1],
                          0, GL_RGBA, GL_UNSIGNED_BYTE, newTextureChar );      
            }
          else
            {
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 
                          newTextureSize[0], newTextureSize[1],
                          0, GL_RGBA, GL_UNSIGNED_SHORT, newTextureShort );      
            }
          
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
      delete [] newTextureChar;
      delete [] newTextureShort;
      }
    
    glFlush();
    glFinish();
    glDeleteTextures(1, &tempIndex);  

    // Restore state
    glPopAttrib();

    return;
    }
  
#else
  if ( imageScalarType == VTK_UNSIGNED_CHAR )
    {
    glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                  imageMemorySize[0], imageMemorySize[1], 
                  0, GL_RGBA, GL_UNSIGNED_BYTE, 
                  static_cast<unsigned char *>(image) );
    }
  else
    {
    glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                  imageMemorySize[0], imageMemorySize[1], 
                  0, GL_RGBA, GL_UNSIGNED_SHORT, 
                  static_cast<unsigned short *>(image) );
    }
  
#endif
  offsetX = .5 / static_cast<float>(imageMemorySize[0]);
  offsetY = .5 / static_cast<float>(imageMemorySize[1]);
  
  tcoords[0]  = 0.0 + offsetX;
  tcoords[1]  = 0.0 + offsetY;
  tcoords[2]  = 
    (float)imageInUseSize[0]/(float)imageMemorySize[0] - offsetX;
  tcoords[3]  = offsetY;
  tcoords[4]  = 
    (float)imageInUseSize[0]/(float)imageMemorySize[0] - offsetX;
  tcoords[5]  = 
    (float)imageInUseSize[1]/(float)imageMemorySize[1] - offsetY;
  tcoords[6]  = offsetX;
  tcoords[7]  = 
    (float)imageInUseSize[1]/(float)imageMemorySize[1] - offsetY;
  
  // Render the polygon
  glBegin( GL_POLYGON );
  
  for ( i = 0; i < 4; i++ )
    {
    glTexCoord2fv( tcoords+i*2 );
    glVertex3fv( verts+i*3 );
    }
  glEnd();

  
#ifdef GL_VERSION_1_1
  glFlush();
  glFinish();
  glDeleteTextures(1, &tempIndex);
#endif
  
  // Restore state
  glPopAttrib();
}

void vtkOpenGLRayCastImageDisplayHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
