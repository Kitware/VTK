/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeRayCastMapper.cxx
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

#include "vtkOpenGLVolumeRayCastMapper.h"
#include "vtkObjectFactory.h"
#include <GL/gl.h>

//---------------------------------------------------------------------------
vtkOpenGLVolumeRayCastMapper* vtkOpenGLVolumeRayCastMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLVolumeRayCastMapper");
  if(ret)
    {
    return (vtkOpenGLVolumeRayCastMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLVolumeRayCastMapper;
}


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

  // Turn blending on so that the translucent geometry of the hexagon can
  // be blended with other geoemtry
  glEnable( GL_BLEND );

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

  float tcoords[8];
  tcoords[0]  = 0.0;
  tcoords[1]  = 0.0;
  tcoords[2]  = (float)this->ImageInUseSize[0]/(float)this->ImageMemorySize[0];
  tcoords[3]  = 0.0;
  tcoords[4]  = (float)this->ImageInUseSize[0]/(float)this->ImageMemorySize[0];
  tcoords[5]  = (float)this->ImageInUseSize[1]/(float)this->ImageMemorySize[1];
  tcoords[6]  = 0.0;
  tcoords[7]  = (float)this->ImageInUseSize[1]/(float)this->ImageMemorySize[1];
  
  // Render the polygon
  glBegin( GL_POLYGON );
  int i;
  
  for ( i = 0; i < 4; i++ )
    {
    glTexCoord2fv( tcoords+i*2 );
    glVertex3fv( verts+i*3 );
    }
  glEnd();

  glDisable( GL_BLEND );
  glDisable( GL_ALPHA_TEST );
  glDisable( GL_TEXTURE_2D );
  glDepthMask( 1 );

  // Turn lighting back on
  glEnable( GL_LIGHTING );  
}

