/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCamera.cxx
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

#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLCamera.h"
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkgluPickMatrix.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLCamera, "1.50");
vtkStandardNewMacro(vtkOpenGLCamera);
#endif

void vtkOpenGLCameraBound(float &vpu, float &vpv)
{
  if (vpu > 1.0) 
    {
    vpu = 1.0;
    }
  if (vpu < 0.0)
    {
    vpu = 0.0;
    }
  if (vpv > 1.0) 
    {
    vpv = 1.0;
    }
  if (vpv < 0.0)
    {
    vpv = 0.0;
    }  
}

// Implement base class method.
void vtkOpenGLCamera::Render(vtkRenderer *ren)
{
  float aspect[2];
  float *vport;
  int  lowerLeft[2];
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();

  // find out if we should stereo render
  this->Stereo = (ren->GetRenderWindow())->GetStereoRender();
  vport = ren->GetViewport();

  float *tileViewPort = ren->GetVTKWindow()->GetTileViewport();
  int scale = ren->GetVTKWindow()->GetTileScale();
  
  float vpu, vpv;
  vpu = (vport[0] - tileViewPort[0]);
  vpv = (vport[1] - tileViewPort[1]);
  vtkOpenGLCameraBound(vpu,vpv);
  ren->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  float vpu2, vpv2;
  vpu2 = (vport[2] - tileViewPort[0]);
  vpv2 = (vport[3] - tileViewPort[1]);
  vtkOpenGLCameraBound(vpu2,vpv2);
  if (vpu2 > tileViewPort[2])
    {
    vpu2 = tileViewPort[2];
    }
  if (vpv2 > tileViewPort[3])
    {
    vpv2 = tileViewPort[3];
    }  
  ren->NormalizedDisplayToDisplay(vpu2,vpv2);
  int usize = (int)(vpu2 + 0.5) - lowerLeft[0];
  int vsize = (int)(vpv2 + 0.5) - lowerLeft[1];  
  if (usize < 0)
    {
    usize = 0;
    }
  if (vsize < 0)
    {
    vsize = 0;
    }
  
  // if were on a stereo renderer draw to special parts of screen
  if (this->Stereo)
    {
    switch ((ren->GetRenderWindow())->GetStereoType())
      {
      case VTK_STEREO_CRYSTAL_EYES:
        if (this->LeftEye)
          {
          glDrawBuffer(GL_BACK_LEFT);
          }
        else
          {
          glDrawBuffer(GL_BACK_RIGHT);
          }
        break;
      case VTK_STEREO_LEFT:
        this->LeftEye = 1;
        break;
      case VTK_STEREO_RIGHT:
        this->LeftEye = 0;
        break;
      default:
        break;
      }
    }
  else
    {
    if (ren->GetRenderWindow()->GetDoubleBuffer())
      {
      glDrawBuffer(GL_BACK);
      }
    else
      {
      glDrawBuffer(GL_FRONT);
      }
    }
  
  glViewport(lowerLeft[0],lowerLeft[1], usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1], usize, vsize);
    
  ren->ComputeAspect();
  ren->GetAspect(aspect);

  glMatrixMode( GL_PROJECTION);
  if(usize && vsize)
    {
    matrix->DeepCopy(this->GetPerspectiveTransformMatrix(1.0*usize/vsize,
                                                         -1,1));
    matrix->Transpose();
    }
  if(ren->GetIsPicking())
    {
    int size[2]; size[0] = usize; size[1] = vsize;
    glLoadIdentity();
    vtkgluPickMatrix(ren->GetPickX(), ren->GetPickY(), 1, 1, lowerLeft, size);
    glMultMatrixd(matrix->Element[0]);
    }
  else
    {
    // insert camera view transformation 
    glLoadMatrixd(matrix->Element[0]);
    }
  
  // push the model view matrix onto the stack, make sure we 
  // adjust the mode first
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  matrix->DeepCopy(this->GetViewTransformMatrix());
  matrix->Transpose();
  
  // insert camera view transformation 
  glMultMatrixd(matrix->Element[0]);

  if ((ren->GetRenderWindow())->GetErase())
    {
    ren->Clear();
    }

  // if we have a stereo renderer, draw other eye next time 
  if (this->Stereo)
    {
    if (this->LeftEye)
      {
      this->LeftEye = 0;
      }
    else
      {
      this->LeftEye = 1;
      }
    }

  matrix->Delete();
}

void vtkOpenGLCamera::UpdateViewport(vtkRenderer *ren)
{
  float *vport;
  int  lowerLeft[2];

  vport = ren->GetViewport();

  float *tileViewPort = ren->GetVTKWindow()->GetTileViewport();
  int scale = ren->GetVTKWindow()->GetTileScale();
  
  float vpu, vpv;
  vpu = (vport[0] - tileViewPort[0]);
  vpv = (vport[1] - tileViewPort[1]);
  vtkOpenGLCameraBound(vpu,vpv);
  ren->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  float vpu2, vpv2;
  vpu2 = (vport[2] - tileViewPort[0]);
  vpv2 = (vport[3] - tileViewPort[1]);
  vtkOpenGLCameraBound(vpu2,vpv2);
  if (vpu2 > tileViewPort[2])
    {
    vpu2 = tileViewPort[2];
    }
  if (vpv2 > tileViewPort[3])
    {
    vpv2 = tileViewPort[3];
    }  
  ren->NormalizedDisplayToDisplay(vpu2,vpv2);
  int usize = (int)(vpu2 + 0.5) - lowerLeft[0];
  int vsize = (int)(vpv2 + 0.5) - lowerLeft[1];  
  if (usize < 0)
    {
    usize = 0;
    }
  if (vsize < 0)
    {
    vsize = 0;
    }

  glViewport(lowerLeft[0],lowerLeft[1], usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1], usize, vsize);
}

