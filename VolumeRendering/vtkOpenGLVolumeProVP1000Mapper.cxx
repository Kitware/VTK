/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeProVP1000Mapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLVolumeProVP1000Mapper.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkVolume.h"

#include "vtkOpenGL.h"

vtkStandardNewMacro(vtkOpenGLVolumeProVP1000Mapper);

void vtkOpenGLVolumeProVP1000Mapper::RenderImageBuffer(vtkRenderer  *ren,
                                                       vtkVolume    *vol,
                                                       int          size[2],
                                                       unsigned int *outData)
{
  float depthVal, nearestPt[3], testZ, minZ;
  double planeCoords[4][4];
  float tCoords[4][2];
  int i, j, k;
  int textureSize[2];
  unsigned int *textureData;
  double bounds[6];
  
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
  
  double aspect[2];
  ren->GetAspect(aspect);
  
  ren->SetViewPoint(-aspect[0], -aspect[1], depthVal);
  ren->ViewToWorld();
  ren->GetWorldPoint(planeCoords[0]);
  
  ren->SetViewPoint(aspect[0], -aspect[1], depthVal);
  ren->ViewToWorld();
  ren->GetWorldPoint(planeCoords[1]);
  
  ren->SetViewPoint(aspect[0], aspect[1], depthVal);
  ren->ViewToWorld();
  ren->GetWorldPoint(planeCoords[2]);
  
  ren->SetViewPoint(-aspect[0], aspect[1], depthVal);
  ren->ViewToWorld();
  ren->GetWorldPoint(planeCoords[3]);
  
  // OpenGL stuff
  glDisable( GL_LIGHTING );
  
  glEnable( GL_TEXTURE_2D );
  glDepthMask( 0 );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
  // Specify the texture
  glColor3f(1.0,1.0,1.0);
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
    glVertex3dv(planeCoords[i]);
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
  
  // The render window allocated this memory, so it should release it.
#if ((VTK_MAJOR_VERSION == 3)&&(VTK_MINOR_VERSION == 2))
  delete [] zData;
#else
  vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow())->ReleaseRGBAPixelData(zData);
#endif
}


void vtkOpenGLVolumeProVP1000Mapper::RenderBoundingBox(vtkRenderer *ren,
                                                       vtkVolume *vol)
{
  double bounds[6], background[3], color[3];
  ren->GetBackground(background);
  if (background[0] > 0.5 && background[1] > 0.5 && background[2] > 0.5)
    {
    // black
    color[0] = color[1] = color[2] = 0.0;
    }
  else
    {
    // white
    color[0] = color[1] = color[2] = 1.0;
    }
  
  vol->GetBounds(bounds);

  glColor3dv(color);
  glDisable( GL_LIGHTING );
  
  glBegin( GL_LINE_LOOP );
  glVertex3d(bounds[0], bounds[2], bounds[4]);
  glVertex3d(bounds[1], bounds[2], bounds[4]);
  glVertex3d(bounds[1], bounds[2], bounds[5]);
  glVertex3d(bounds[0], bounds[2], bounds[5]);
  glEnd();
  glBegin( GL_LINE_LOOP );
  glVertex3d(bounds[0], bounds[3], bounds[4]);
  glVertex3d(bounds[1], bounds[3], bounds[4]);
  glVertex3d(bounds[1], bounds[3], bounds[5]);
  glVertex3d(bounds[0], bounds[3], bounds[5]);
  glEnd();
  glBegin( GL_LINES );
  glVertex3d(bounds[0], bounds[2], bounds[4]);
  glVertex3d(bounds[0], bounds[3], bounds[4]);
  glVertex3d(bounds[1], bounds[2], bounds[4]);
  glVertex3d(bounds[1], bounds[3], bounds[4]);
  glVertex3d(bounds[1], bounds[2], bounds[5]);
  glVertex3d(bounds[1], bounds[3], bounds[5]);
  glVertex3d(bounds[0], bounds[2], bounds[5]);
  glVertex3d(bounds[0], bounds[3], bounds[5]);
  glEnd();
  
  glEnable( GL_LIGHTING );
  glFlush();
}

//----------------------------------------------------------------------------
void vtkOpenGLVolumeProVP1000Mapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
