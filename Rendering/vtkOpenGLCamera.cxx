/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLCamera.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOutputWindow.h"
#include "vtkRenderWindow.h"
#include "vtkgluPickMatrix.h"

#if defined(__APPLE__) && (defined(VTK_USE_CARBON) || defined(VTK_USE_COCOA))
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <math.h>

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLCamera, "1.59.4.3");
vtkStandardNewMacro(vtkOpenGLCamera);
#endif

#define vtkOpenGLCameraBound(vpu, vpv) \
{ \
  if (vpu > 1.0) \
    { \
    vpu = 1.0; \
    } \
  if (vpu < 0.0) \
    { \
    vpu = 0.0; \
    } \
  if (vpv > 1.0) \
    { \
    vpv = 1.0; \
    } \
  if (vpv < 0.0) \
    { \
    vpv = 0.0; \
    }  \
}

// Implement base class method.
void vtkOpenGLCamera::Render(vtkRenderer *ren)
{
  double aspect[2];
  double *vport;
  int  lowerLeft[2];
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();

  // find out if we should stereo render
  this->Stereo = (ren->GetRenderWindow())->GetStereoRender();
  vport = ren->GetViewport();

  double *tileViewPort = ren->GetVTKWindow()->GetTileViewport();
  //int scale = 
  ren->GetVTKWindow()->GetTileScale();
  
  double vpu, vpv;
  // find the lower left corner of the viewport, taking into account the
  // lower left boundary of this tile
  vpu = (vport[0] - tileViewPort[0]);
  vpv = (vport[1] - tileViewPort[1]);
  vtkOpenGLCameraBound(vpu,vpv);
  // store the result as a pixel value
  ren->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  double vpu2, vpv2;
  // find the upper right corner of the viewport, taking into account the
  // lower left boundary of this tile
  vpu2 = (vport[2] - tileViewPort[0]);
  vpv2 = (vport[3] - tileViewPort[1]);
  vtkOpenGLCameraBound(vpu2,vpv2);
  // also watch for the upper right boundary of the tile
  if (vpu2 > (tileViewPort[2] - tileViewPort[0]))
    {
    vpu2 = tileViewPort[2] - tileViewPort[0];
    }
  if (vpv2 > (tileViewPort[3] - tileViewPort[1]))
    {
    vpv2 = tileViewPort[3] - tileViewPort[1];
    }  
  ren->NormalizedDisplayToDisplay(vpu2,vpv2);
  // now compute the size of the intersection of the viewport with the
  // current tile
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
    
  // some renderer subclasses may have more complicated computations for the
  // aspect ratio. SO take that into account by computing the difference
  // between our simple aspect ratio and what the actual renderer is
  // reporting.
  ren->ComputeAspect();
  ren->GetAspect(aspect);
  double aspect2[2];
  ren->vtkViewport::ComputeAspect();
  ren->vtkViewport::GetAspect(aspect2);
  double aspectModification = aspect[0]*aspect2[1]/(aspect[1]*aspect2[0]);
  
  glMatrixMode( GL_PROJECTION);
  if(usize && vsize)
    {
    matrix->DeepCopy(this->GetPerspectiveTransformMatrix(
                       aspectModification*usize/vsize, -1,1));
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

  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
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
  double *vport;
  int  lowerLeft[2];

  vport = ren->GetViewport();

  double *tileViewPort = ren->GetVTKWindow()->GetTileViewport();
  //int scale = 
  ren->GetVTKWindow()->GetTileScale();
  
  double vpu, vpv;
  vpu = (vport[0] - tileViewPort[0]);
  vpv = (vport[1] - tileViewPort[1]);
  vtkOpenGLCameraBound(vpu,vpv);
  ren->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  double vpu2, vpv2;
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

//----------------------------------------------------------------------------
void vtkOpenGLCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
