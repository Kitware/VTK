/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeProVP1000Mapper.cxx
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

#include "vtkOpenGLVolumeProVP1000Mapper.h"
#include "vtkTransform.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include <GL/gl.h>
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"

vtkOpenGLVolumeProVP1000Mapper* vtkOpenGLVolumeProVP1000Mapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLVolumeProVP1000Mapper");
  if(ret)
    {
    return (vtkOpenGLVolumeProVP1000Mapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLVolumeProVP1000Mapper;
}

void vtkOpenGLVolumeProVP1000Mapper::RenderImageBuffer(vtkRenderer  *ren,
                                                       vtkVolume    *vol,
                                                       int          size[2],
                                                       unsigned int *outData)
{
  float depthVal, nearestPt[3], testZ, minZ;
  float planeCoords[4][4];
  float tCoords[4][2];
  int i, j, k;
  int textureSize[2];
  unsigned int *textureData;
  float bounds[6];
  
  textureSize[0] = textureSize[1] = 32;
  while (textureSize[0] < size[0])
    {
    textureSize[0] *= 2;
    }
  while (textureSize[1] < size[1])
    {
    textureSize[1] *= 2;
    }

  textureData = new unsigned int[textureSize[0]*textureSize[1]];
  for (j = 0; j < textureSize[1]; j++)
    {
    for (i = 0; i < textureSize[0]; i++)
      {
      if (i < size[0] && j < size[1])
        {
        textureData[j*textureSize[0] + i] = outData[j*size[0] + i];
        }
      else
        {
        textureData[j*textureSize[0] + i] = 0;
        }
      }
    }
  
  if ( ! this->IntermixIntersectingGeometry )
    {
    ren->SetWorldPoint(vol->GetCenter()[0],
                       vol->GetCenter()[1],
                       vol->GetCenter()[2],
                       1.0);
    }
  else
    {
    minZ = 1;
    vol->GetBounds(bounds);
    
    for (k = 0; k < 2; k++)
      {
      for (j = 0; j < 2; j++)
        {
        for (i = 0; i < 2; i++)
          {
          ren->SetWorldPoint(bounds[i+0], bounds[j+2], bounds[k+4], 1.0);
          ren->WorldToDisplay();
          testZ = ren->GetDisplayPoint()[2];
          if (testZ < minZ)
            {
            minZ = testZ;
            nearestPt[0] = bounds[i+0];
            nearestPt[1] = bounds[j+2];
            nearestPt[2] = bounds[k+4];
            }
          }
        }
      }
    ren->SetWorldPoint(nearestPt[0], nearestPt[1], nearestPt[2], 1.0);
    }
  
  ren->WorldToView();
  depthVal = ren->GetViewPoint()[2];
  
  ren->SetViewPoint(-1, -1, depthVal);
  ren->ViewToWorld();
  ren->GetWorldPoint(planeCoords[0]);
  
  ren->SetViewPoint(1, -1, depthVal);
  ren->ViewToWorld();
  ren->GetWorldPoint(planeCoords[1]);
  
  ren->SetViewPoint(1, 1, depthVal);
  ren->ViewToWorld();
  ren->GetWorldPoint(planeCoords[2]);
  
  ren->SetViewPoint(-1, 1, depthVal);
  ren->ViewToWorld();
  ren->GetWorldPoint(planeCoords[3]);
  
  // OpenGL stuff
  glDisable( GL_LIGHTING );

  glEnable( GL_TEXTURE_2D );
  glDepthMask( 0 );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
#ifdef GL_VERSION_1_1
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 
                textureSize[0], textureSize[1],
                0, GL_RGBA, GL_UNSIGNED_BYTE, textureData );
#else
  glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                textureSize[0], textureSize[1],
                0, GL_RGBA, GL_UNSIGNED_BYTE, textureData );
#endif 
  
  tCoords[0][0] = 0.0;
  tCoords[0][1] = 0.0;
  tCoords[1][0] = (float)size[0]/(float)textureSize[0];
  tCoords[1][1] = 0.0;
  tCoords[2][0] = (float)size[0]/(float)textureSize[0];
  tCoords[2][1] = (float)size[1]/(float)textureSize[1];
  tCoords[3][0] = 0.0;
  tCoords[3][1] = (float)size[1]/(float)textureSize[1];
  
  glBegin( GL_POLYGON );
  for (i = 0; i < 4; i++)
    {
    glTexCoord2fv(tCoords[i]);
    glVertex3fv(planeCoords[i]);
    }
  glEnd();
  glDisable( GL_TEXTURE_2D);
  glDepthMask( 1 );
  glEnable( GL_LIGHTING );
  
  glFlush();
  
  delete [] textureData;
}

void vtkOpenGLVolumeProVP1000Mapper::GetDepthBufferValues(vtkRenderer *ren,
                                                          int size[2],
                                                          unsigned int *outData)
{
  float *zData;
  int i, length, rescale;
  
  zData = ren->GetRenderWindow()->GetZbufferData(0, 0, size[0]-1, size[1]-1);
  if ( ! zData )
    {
    vtkErrorMacro("could not get Z buffer data");
    return;
    }

  length = size[0]*size[1];
  
  rescale = 16777215; // 2^24 - 1
  
  for (i = 0; i < length; i++)
    {
    outData[i] = (unsigned int)(zData[i] * rescale);
   }
}
