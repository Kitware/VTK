/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeTextureMapper2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkOpenGLVolumeTextureMapper2D.h"
#include "vtkMatrix4x4.h"
#include "vtkVolume.h"
#include "vtkTimerLog.h"

#include <GL/gl.h>
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
      glEnable(GL_CLIP_PLANE0+i);

      plane = (vtkPlane *)clipPlanes->GetItemAsObject(i);

      planeEquation[0] = plane->GetNormal()[0]; 
      planeEquation[1] = plane->GetNormal()[1]; 
      planeEquation[2] = plane->GetNormal()[2];
      planeEquation[3] = -(planeEquation[0]*plane->GetOrigin()[0]+
			   planeEquation[1]*plane->GetOrigin()[1]+
			   planeEquation[2]*plane->GetOrigin()[2]);
      glClipPlane(GL_CLIP_PLANE0+i,planeEquation);
      }
    }

  this->GenerateTexturesAndRenderRectangles(); 
    
  glDisable( GL_BLEND );
  glDisable( GL_TEXTURE_2D );

  // Turn lighting back on
  glEnable( GL_LIGHTING );

  // pop transformation matrix
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  matrix->Delete();

  if ( clipPlanes )
    {
    for (i = 0; i < numClipPlanes; i++)
      {
      glDisable(GL_CLIP_PLANE0+i);
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

