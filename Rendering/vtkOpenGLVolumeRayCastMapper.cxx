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
vtkCxxRevisionMacro(vtkOpenGLVolumeRayCastMapper, "1.11");
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
#ifdef GL_VERSION_1_1
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 
                this->ImageMemorySize[0], this->ImageMemorySize[1], 
                0, GL_RGBA, GL_UNSIGNED_BYTE, this->Image );
#else
  glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                this->ImageMemorySize[0], this->ImageMemorySize[1], 
                0, GL_RGBA, GL_UNSIGNED_BYTE, this->Image );
#endif
  float offsetX = .5 / static_cast<float>(this->ImageMemorySize[0]);
  float offsetY = .5 / static_cast<float>(this->ImageMemorySize[1]);
  
  float tcoords[8];
  tcoords[0]  = 0.0 + offsetX;
  tcoords[1]  = 0.0 + offsetY;
  tcoords[2]  = 
    (float)this->ImageInUseSize[0]/(float)this->ImageMemorySize[0] - offsetX;
  tcoords[3]  = + offsetY;
  tcoords[4]  = 
    (float)this->ImageInUseSize[0]/(float)this->ImageMemorySize[0] - offsetX;
  tcoords[5]  = 
    (float)this->ImageInUseSize[1]/(float)this->ImageMemorySize[1] - offsetY;
  tcoords[6]  = offsetX;
  tcoords[7]  = 
    (float)this->ImageInUseSize[1]/(float)this->ImageMemorySize[1] - offsetY;
  
  // Render the polygon
  glBegin( GL_POLYGON );
  int i;
  
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

