/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeTextureMapper2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLVolumeTextureMapper2D.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
# include "vtkOpenGL.h"
#endif


#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLVolumeTextureMapper2D);
#endif



vtkOpenGLVolumeTextureMapper2D::vtkOpenGLVolumeTextureMapper2D()
{
}

vtkOpenGLVolumeTextureMapper2D::~vtkOpenGLVolumeTextureMapper2D()
{
}

void vtkOpenGLVolumeTextureMapper2D::Render(vtkRenderer *ren, vtkVolume *vol)
{
  vtkMatrix4x4       *matrix;
  double             matrixForGL[16];
  int                i, numClipPlanes = 0;
  double             planeEquation[4];

  this->Timer->StartTimer();

  // Let the superclass take care of some initialization
  this->vtkVolumeTextureMapper2D::InitializeRender( ren, vol );

  // compute transformation
  matrix = vol->GetMatrix();
  vtkMatrix4x4::Transpose(*matrix->Element, matrixForGL);

  // insert model transformation 
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glMultMatrixd(matrixForGL);

  // use the OpenGL clip planes
  numClipPlanes = this->GetNumberOfClippingPlanes();
  if (numClipPlanes > 6)
    {
    vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
    numClipPlanes = 6;
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    glEnable(static_cast<GLenum>(GL_CLIP_PLANE0+i));
    this->GetClippingPlaneInDataCoords(matrix, i, planeEquation);
    glClipPlane(static_cast<GLenum>(GL_CLIP_PLANE0+i), planeEquation);
    }

  // Make sure that culling is turned off
  glDisable( GL_CULL_FACE );

  // Turn lighting off - the polygon textures already have illumination
  glDisable( GL_LIGHTING );

  // Turn texturing on so that we can draw the textured polygons
  glEnable( GL_TEXTURE_2D );

#ifdef GL_VERSION_1_1
  GLuint tempIndex;
  glGenTextures(1, &tempIndex);
  glBindTexture(GL_TEXTURE_2D, tempIndex);
#endif

  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  glColor3f( 1.0, 1.0, 1.0 );

  this->GenerateTexturesAndRenderQuads( ren, vol ); 
    
  // pop transformation matrix
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glDisable( GL_TEXTURE_2D );
  
#ifdef GL_VERSION_1_1
  glFlush();
  glDeleteTextures(1, &tempIndex);
#endif
  
  // Turn lighting back on
  glEnable( GL_LIGHTING );

  for (i = 0; i < numClipPlanes; i++)
    {
    glDisable(static_cast<GLenum>(GL_CLIP_PLANE0+i));
    }

  this->Timer->StopTimer();      

  this->TimeToDraw = static_cast<float>(this->Timer->GetElapsedTime());

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if ( this->TimeToDraw == 0.0 )
    {
    this->TimeToDraw = 0.0001;
    }   
}

void vtkOpenGLVolumeTextureMapper2D::RenderQuads( int numQuads,
                                                  float *v, 
                                                  float *t,
                                                  unsigned char *texture,
                                                  int size[2], int reverseFlag )
{
#ifdef GL_VERSION_1_1
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, size[0], size[1], 
                0, GL_RGBA, GL_UNSIGNED_BYTE, texture );
#else
  glTexImage2D( GL_TEXTURE_2D, 0, 4, size[0], size[1], 
                0, GL_RGBA, GL_UNSIGNED_BYTE, texture );
#endif

  glBegin( GL_QUADS );

  float *tptr, *vptr;
  int i, j;
  
  if ( reverseFlag )
    {
    for ( i = 0; i < numQuads; i++ )
      {
      tptr = t+2*4*(numQuads-i-1);
      vptr = v+3*4*(numQuads-i-1);    
      for ( j = 0; j < 4; j++ )
        {
        glTexCoord2fv( tptr );
        glVertex3fv(   vptr );
        tptr += 2;
        vptr += 3;
        }
      }
    }
  else
    {
    tptr = t; 
    vptr = v;    
    for ( i = 0; i < numQuads*4; i++ )
      {
      glTexCoord2fv( tptr );
      glVertex3fv(   vptr );
      tptr += 2;
      vptr += 3;
      }
    }
  
  glEnd();
}

// Print the vtkOpenGLVolumeTextureMapper2D
void vtkOpenGLVolumeTextureMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

