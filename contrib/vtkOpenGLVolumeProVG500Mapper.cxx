/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeProVG500Mapper.cxx
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

#include "vtkOpenGLVolumeProVG500Mapper.h"
#include "vtkTransform.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include <GL/gl.h>

void vtkOpenGLVolumeProVG500Mapper::RenderHexagon(  vtkRenderer  *ren,
						    vtkVolume    *vol,
						    VLIPixel     *basePlane,
						    int          size[2],
						    VLIVector3D  hexagon[6],
						    VLIVector2D  textureCoords[6] )
{
  vtkTransform   *t;
  int            i;
  float          in[4], out[4];
  float          *center, x;
  float          volCenter[3];
  VLIVector3D    hexCenter;

  // Turn lighting off - the hexagon texture already has illumination in it
  glDisable( GL_LIGHTING );

  // Turn texturing on so that we can draw the textured hexagon
  glEnable( GL_TEXTURE_2D );

  // Turn blending on so that the translucent geometry of the hexagon can
  // be blended with other geoemtry
  glEnable( GL_BLEND );

  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Push a new matrix since we are going to modify it
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();

  // Find out where the center of the volume is in camera coordinates
  t = vtkTransform::New();
  t->SetMatrix( ren->GetActiveCamera()->GetViewTransform() );
  center = vol->GetCenter();
  in[0] = center[0];
  in[1] = center[1];
  in[2] = center[2];
  in[3] = 1.0;
  t->MultiplyPoint(in, out);
  volCenter[0] = out[0] / out[3];
  volCenter[1] = out[1] / out[3];
  volCenter[2] = out[2] / out[3];

  // Remove the view transform from the OpenGL modelview matrix stack
  t->Inverse();
  t->Transpose();
  glMultMatrixd( t->GetMatrixPointer()->Element[0] );
  t->Delete();

  // Specify the texture
  glColor3f(1.0,1.0,1.0);
#ifdef GL_VERSION_1_1
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, size[0], size[1], 
		0, GL_RGBA, GL_UNSIGNED_BYTE, basePlane );
#else
  glTexImage2D( GL_TEXTURE_2D, 0, 4, size[0], size[1], 
		0, GL_RGBA, GL_UNSIGNED_BYTE, basePlane );
#endif


  // What is the center of the hexagon?
  if (hexagon[0] + (hexagon[0]-hexagon[3]) == hexagon[3])
    {
    x = 0.5;
    }
  else
    {
    x = -0.5;
    }
  hexCenter = hexagon[0] + (x*(hexagon[0] - hexagon[3]));

  // Render the hexagon - subtract the hexagon center from
  // each vertex, and add the center of the volume to each vertex.
  glBegin( GL_POLYGON );
  for ( i = 0; i < 6; i++ )
    {
    glTexCoord2d( textureCoords[i].X(), textureCoords[i].Y() );
    in[0] = hexagon[i].X() - hexCenter.X() + volCenter[0];
    in[1] = hexagon[i].Y() - hexCenter.Y() + volCenter[1];
    in[2] = hexagon[i].Z() - hexCenter.Z() + volCenter[2];
    glVertex3fv( in );
    }
  glEnd();

  glDisable( GL_BLEND );
  glDisable( GL_ALPHA_TEST );
  glDisable( GL_TEXTURE_2D );

  // Pop the OpenGL modelview matrix
  glPopMatrix();

  // Turn lighting back on
  glEnable( GL_LIGHTING );
}

