/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeProVG500Mapper.cxx
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

#include "vtkOpenGLVolumeProVG500Mapper.h"
#include "vtkTransform.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include <GL/gl.h>
#include "vtkObjectFactory.h"

vtkOpenGLVolumeProVG500Mapper* vtkOpenGLVolumeProVG500Mapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLVolumeProVG500Mapper");
  if(ret)
    {
    return (vtkOpenGLVolumeProVG500Mapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLVolumeProVG500Mapper;
}

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

  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // Push a new matrix since we are going to modify it
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();

  // Find out where the center of the volume is in camera coordinates
  t = vtkTransform::New();
  t->SetMatrix( ren->GetActiveCamera()->GetViewTransformMatrix() );
  center = vol->GetCenter();

  // if the cut plane is on
  if ( this->CutPlane )
    {
    // How far is the center from the center of the cut?
    float dist = 
      center[0] * this->CutPlaneEquation[0] +
      center[1] * this->CutPlaneEquation[1] +
      center[2] * this->CutPlaneEquation[2] +
      this->CutPlaneEquation[3] + 0.5*this->CutPlaneThickness;
    
    // Move the center along the cut direction onto the cut center plane.
    center[0] -= 
      dist * this->CutPlaneEquation[0];
    
    center[1] -=
      dist * this->CutPlaneEquation[1];

    center[2] -=
      dist * this->CutPlaneEquation[2];

    }

  in[0] = center[0];
  in[1] = center[1];
  in[2] = center[2];
  in[3] = 1.0;
  t->MultiplyPoint(in, out);
  volCenter[0] = out[0] / out[3];
  volCenter[1] = out[1] / out[3];
  volCenter[2] = out[2] / out[3];

  // Invert this matrix to go from view to world
  t->Inverse();
  
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
  // also, transform the vertices into world coordinates (from the
  // view coordinates that the VPRO harware returns them in)
  glBegin( GL_POLYGON );
  for ( i = 0; i < 6; i++ )
    {
    glTexCoord2d( textureCoords[i].X(), textureCoords[i].Y() );
    in[0] = hexagon[i].X() - hexCenter.X() + volCenter[0];
    in[1] = hexagon[i].Y() - hexCenter.Y() + volCenter[1];
    in[2] = hexagon[i].Z() - hexCenter.Z() + volCenter[2];
    in[3] = 1.0;
    t->MultiplyPoint(in, in);
    glVertex3fv( in );
    }
  glEnd();

  t->Delete();

  glDisable( GL_TEXTURE_2D );

  // Pop the OpenGL modelview matrix
  glPopMatrix();

  // Turn lighting back on
  glEnable( GL_LIGHTING );
}

