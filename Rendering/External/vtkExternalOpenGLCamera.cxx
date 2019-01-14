/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExternalOpenGLCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExternalOpenGLCamera.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGL.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkOutputWindow.h"
#include "vtkPerspectiveTransform.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

#include <cmath>

vtkStandardNewMacro(vtkExternalOpenGLCamera);

//----------------------------------------------------------------------------
vtkExternalOpenGLCamera::vtkExternalOpenGLCamera()
{
  this->UserProvidedViewTransform = false;
}

//----------------------------------------------------------------------------
// Implement base class method.
void vtkExternalOpenGLCamera::Render(vtkRenderer *ren)
{
  vtkOpenGLClearErrorMacro();

  int  lowerLeft[2];
  int usize, vsize;

  vtkOpenGLRenderWindow *win = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState *ostate = win->GetState();

  // find out if we should stereo render
  this->Stereo = (ren->GetRenderWindow())->GetStereoRender();
  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft+1);

  // Take the window position into account
  for (int i = 0; i < 2; ++i)
  {
    lowerLeft[i] = lowerLeft[i] + ren->GetRenderWindow()->GetPosition()[i];
  }

  // if were on a stereo renderer draw to special parts of screen
  if (this->Stereo)
  {
    switch ((ren->GetRenderWindow())->GetStereoType())
    {
      case VTK_STEREO_CRYSTAL_EYES:
        if (this->LeftEye)
        {
          if (ren->GetRenderWindow()->GetDoubleBuffer())
          {
            glDrawBuffer(static_cast<GLenum>(win->GetBackLeftBuffer()));
            glReadBuffer(static_cast<GLenum>(win->GetBackLeftBuffer()));
          }
          else
          {
            glDrawBuffer(static_cast<GLenum>(win->GetFrontLeftBuffer()));
            glReadBuffer(static_cast<GLenum>(win->GetFrontLeftBuffer()));
          }
        }
        else
        {
           if (ren->GetRenderWindow()->GetDoubleBuffer())
           {
            glDrawBuffer(static_cast<GLenum>(win->GetBackRightBuffer()));
            glReadBuffer(static_cast<GLenum>(win->GetBackRightBuffer()));
           }
          else
          {
            glDrawBuffer(static_cast<GLenum>(win->GetFrontRightBuffer()));
            glReadBuffer(static_cast<GLenum>(win->GetFrontRightBuffer()));
          }
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
      glDrawBuffer(static_cast<GLenum>(win->GetBackBuffer()));

      // Reading back buffer means back left. see OpenGL spec.
      // because one can write to two buffers at a time but can only read from
      // one buffer at a time.
      glReadBuffer(static_cast<GLenum>(win->GetBackBuffer()));
    }
    else
    {
      glDrawBuffer(static_cast<GLenum>(win->GetFrontBuffer()));

      // Reading front buffer means front left. see OpenGL spec.
      // because one can write to two buffers at a time but can only read from
      // one buffer at a time.
      glReadBuffer(static_cast<GLenum>(win->GetFrontBuffer()));
    }
  }

  ostate->vtkglViewport(lowerLeft[0], lowerLeft[1], usize, vsize);
  ostate->vtkglEnable(GL_SCISSOR_TEST);
  ostate->vtkglScissor(lowerLeft[0], lowerLeft[1], usize, vsize);

  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLCamera::SetViewTransformMatrix(
  const double elements[16])
{
  if (!elements)
  {
    return;
  }
  // Transpose the matrix to undo the transpose that VTK does internally
  vtkMatrix4x4* matrix = vtkMatrix4x4::New();
  matrix->DeepCopy(elements);
  matrix->Transpose();
  this->ViewTransform->SetMatrix(matrix);
  this->ModelViewTransform->SetMatrix(matrix);
  this->UserProvidedViewTransform = true;
  matrix->Delete();
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLCamera::SetProjectionTransformMatrix(
  const double elements[16])
{
  if (!elements)
  {
    return;
  }
  // Transpose the matrix to undo the transpose that VTK does internally
  vtkMatrix4x4* matrix = vtkMatrix4x4::New();
  matrix->DeepCopy(elements);
  matrix->Transpose();

  this->SetExplicitProjectionTransformMatrix(matrix);
  this->SetUseExplicitProjectionTransformMatrix(true);
  matrix->Delete();
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLCamera::ComputeViewTransform()
{
  if (this->UserProvidedViewTransform)
  {
    // Do not do anything
    return;
  }
  else
  {
    this->Superclass::ComputeViewTransform();
  }
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
