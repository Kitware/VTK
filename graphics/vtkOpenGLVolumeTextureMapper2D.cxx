/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeTextureMapper2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkMatrix4x4.h"
#include "vtkVolume.h"
#include "vtkTimerLog.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#include <GL/gl.h>
#endif
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkOpenGLVolumeTextureMapper2D* vtkOpenGLVolumeTextureMapper2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLVolumeTextureMapper2D");
  if(ret)
    {
    return (vtkOpenGLVolumeTextureMapper2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLVolumeTextureMapper2D;
}




vtkOpenGLVolumeTextureMapper2D::vtkOpenGLVolumeTextureMapper2D()
{
}

vtkOpenGLVolumeTextureMapper2D::~vtkOpenGLVolumeTextureMapper2D()
{
}

void vtkOpenGLVolumeTextureMapper2D::Render(vtkRenderer *ren, vtkVolume *vol)
{
  vtkMatrix4x4       *matrix = vtkMatrix4x4::New();
  vtkTimerLog        *timer;
  vtkPlaneCollection *clipPlanes;
  vtkPlane           *plane;
  int                i, numClipPlanes;
  double             planeEquation[4];

  timer = vtkTimerLog::New();
  timer->StartTimer();


  // Let the superclass take care of some initialization
  this->vtkVolumeTextureMapper2D::InitializeRender( ren, vol );

  // build transformation 
  vol->GetMatrix(matrix);
  matrix->Transpose();

  // insert model transformation 
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glMultMatrixd(matrix->Element[0]);

  // Turn lighting off - the polygon textures already have illumination
  glDisable( GL_LIGHTING );

  // Turn texturing on so that we can draw the textured polygons
  glEnable( GL_TEXTURE_2D );

  // Turn blending on so that the translucent geometry of the polygons can
  // be blended with other geoemtry (non-intersecting only)
  glEnable( GL_BLEND );

#ifdef GL_VERSION_1_1
  GLuint tempIndex;
  glGenTextures(1, &tempIndex);
  glBindTexture(GL_TEXTURE_2D, tempIndex);
#endif

  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  glColor3f( 1.0, 1.0, 1.0 );

  // Use the OpenGL clip planes
  clipPlanes = this->ClippingPlanes;
  if ( clipPlanes )
    {
    numClipPlanes = clipPlanes->GetNumberOfItems();
    if (numClipPlanes > 6)
      {
      vtkErrorMacro(<< "OpenGL guarantees only 6 additional clipping planes");
      }

    for (i = 0; i < numClipPlanes; i++)
      {
      glEnable((GLenum)(GL_CLIP_PLANE0+i));

      plane = (vtkPlane *)clipPlanes->GetItemAsObject(i);

      planeEquation[0] = plane->GetNormal()[0]; 
      planeEquation[1] = plane->GetNormal()[1]; 
      planeEquation[2] = plane->GetNormal()[2];
      planeEquation[3] = -(planeEquation[0]*plane->GetOrigin()[0]+
			   planeEquation[1]*plane->GetOrigin()[1]+
			   planeEquation[2]*plane->GetOrigin()[2]);
      glClipPlane((GLenum)(GL_CLIP_PLANE0+i),planeEquation);
      }
    }

  this->GenerateTexturesAndRenderRectangles(); 
    
  // pop transformation matrix
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  matrix->Delete();

  glDisable( GL_BLEND );
  glDisable( GL_TEXTURE_2D );
  
#ifdef GL_VERSION_1_1
  glFlush();
  glDeleteTextures(1, &tempIndex);
#endif
  
  // Turn lighting back on
  glEnable( GL_LIGHTING );

  if ( clipPlanes )
    {
    for (i = 0; i < numClipPlanes; i++)
      {
      glDisable((GLenum)(GL_CLIP_PLANE0+i));
      }
    }

  timer->StopTimer();      

  this->TimeToDraw = (float)timer->GetElapsedTime();

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if ( this->TimeToDraw == 0.0 )
    {
    this->TimeToDraw = 0.0001;
    }	
  timer->Delete();
}

void vtkOpenGLVolumeTextureMapper2D::RenderRectangle( float v[12], 
						      float t[8],
						      unsigned char *texture,
						      int size[2])
{
#ifdef GL_VERSION_1_1
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, size[0], size[1], 
		0, GL_RGBA, GL_UNSIGNED_BYTE, texture );
#else
  glTexImage2D( GL_TEXTURE_2D, 0, 4, size[0], size[1], 
		0, GL_RGBA, GL_UNSIGNED_BYTE, texture );
#endif

  glBegin( GL_POLYGON );

  glTexCoord2fv( t );
  glVertex3fv( v ); 
  
  glTexCoord2fv( t+2 );
  glVertex3fv( v+3 ); 
  
  glTexCoord2fv( t+4 );
  glVertex3fv( v+6 ); 
  
  glTexCoord2fv( t+6 );
  glVertex3fv( v+9 ); 

  glEnd();
}

// Print the vtkOpenGLVolumeTextureMapper2D
void vtkOpenGLVolumeTextureMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkVolumeTextureMapper::PrintSelf(os,indent);
}

