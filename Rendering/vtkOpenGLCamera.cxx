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
vtkCxxRevisionMacro(vtkOpenGLCamera, "1.48");
vtkStandardNewMacro(vtkOpenGLCamera);
#endif

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

  float vpu, vpv;
  vpu = tileViewPort[0] + (tileViewPort[2] - tileViewPort[0])*vport[0];
  vpv = tileViewPort[1] + (tileViewPort[3] - tileViewPort[1])*vport[1];  
  ren->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  float vpu2, vpv2;
  vpu2 = tileViewPort[0] + (tileViewPort[2] - tileViewPort[0])*vport[2];
  vpv2 = tileViewPort[1] + (tileViewPort[3] - tileViewPort[1])*vport[3];  
  ren->NormalizedDisplayToDisplay(vpu2,vpv2);
  int usize = (int)(vpu2 + 0.5) - lowerLeft[0];
  int vsize = (int)(vpv2 + 0.5) - lowerLeft[1];  
  
  vpu2 = tileViewPort[0];
  vpv2 = tileViewPort[1];  
  ren->NormalizedDisplayToDisplay(vpu2,vpv2);
  lowerLeft[0] = lowerLeft[0] - (int)(vpu2+0.5);
  lowerLeft[1] = lowerLeft[1] - (int)(vpv2+0.5);
  
  
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
  matrix->DeepCopy(this->GetPerspectiveTransformMatrix(aspect[0]/aspect[1],
                                                       -1,1));
  matrix->Transpose();
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

  float vpu, vpv;
  vpu = tileViewPort[0] + (tileViewPort[2] - tileViewPort[0])*vport[0];
  vpv = tileViewPort[1] + (tileViewPort[3] - tileViewPort[1])*vport[1];  
  ren->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  float vpu2, vpv2;
  vpu2 = tileViewPort[0] + (tileViewPort[2] - tileViewPort[0])*vport[2];
  vpv2 = tileViewPort[1] + (tileViewPort[3] - tileViewPort[1])*vport[3];  
  ren->NormalizedDisplayToDisplay(vpu2,vpv2);
  int usize = (int)(vpu2 + 0.5) - lowerLeft[0];
  int vsize = (int)(vpv2 + 0.5) - lowerLeft[1];  

  vpu2 = tileViewPort[0];
  vpv2 = tileViewPort[1];  
  ren->NormalizedDisplayToDisplay(vpu2,vpv2);
  lowerLeft[0] = lowerLeft[0] - (int)(vpu2+0.5);
  lowerLeft[1] = lowerLeft[1] - (int)(vpv2+0.5);

  glViewport(lowerLeft[0],lowerLeft[1], usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1], usize, vsize);
}

