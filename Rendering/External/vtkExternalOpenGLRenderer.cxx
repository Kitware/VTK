/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExternalOpenGLRenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExternalOpenGLRenderer.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkExternalOpenGLCamera.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGL.h"
#include "vtkRenderWindow.h"
#include "vtkTexture.h"

vtkStandardNewMacro(vtkExternalOpenGLRenderer);

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderer::vtkExternalOpenGLRenderer()
{
  this->PreserveColorBuffer = 1;
  this->PreserveDepthBuffer = 1;
}

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderer::~vtkExternalOpenGLRenderer()
{
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderer::Clear()
{
  vtkOpenGLClearErrorMacro();

  GLbitfield  clear_mask = 0;

  if (!this->Transparent() && !this->GetPreserveColorBuffer())
    {
    glClearColor( static_cast<GLclampf>(this->Background[0]),
                  static_cast<GLclampf>(this->Background[1]),
                  static_cast<GLclampf>(this->Background[2]),
                  static_cast<GLclampf>(0.0));
    clear_mask |= GL_COLOR_BUFFER_BIT;
    }

  if (!this->GetPreserveDepthBuffer())
    {
    glClearDepth(static_cast<GLclampf>(1.0));
    clear_mask |= GL_DEPTH_BUFFER_BIT;
    }

  vtkDebugMacro(<< "glClear\n");
  glClear(clear_mask);

  // If gradient background is turned on, draw it now.
  if (!this->Transparent() &&
      (this->GradientBackground || this->TexturedBackground))
    {
    double tile_viewport[4];
    this->GetRenderWindow()->GetTileViewport(tile_viewport);
    glPushAttrib(GL_ENABLE_BIT | GL_TRANSFORM_BIT);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glShadeModel(GL_SMOOTH); // color interpolation

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
      {
      glLoadIdentity();
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
        {
        glLoadIdentity();
        glOrtho(
          tile_viewport[0],
          tile_viewport[2],
          tile_viewport[1],
          tile_viewport[3],
          -1.0, 1.0);

        //top vertices
        if (this->TexturedBackground && this->BackgroundTexture)
          {
          glEnable(GL_TEXTURE_2D);

          this->BackgroundTexture->Render(this);

          // NOTE: By default the mode is GL_MODULATE. Since the user
          // cannot set the mode, the default is set to replace.
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

          // NOTE: vtkTexture Render enables the alpha test
          // so that no buffer is affected if alpha of incoming fragment is
          // below the threshold. Here we have to enable it so that it won't
          // rejects the fragments of the quad as the alpha is set to 0 on it.
          glDisable(GL_ALPHA_TEST);
          }

        glBegin(GL_QUADS);
        glColor4d(this->Background[0],this->Background[1],this->Background[2],
                  0.0);
        glTexCoord2f(0.0, 0.0);
        glVertex2f(0.0, 0.0);

        glTexCoord2f(1.0, 0.0);
        glVertex2f(1.0, 0);

        //bottom vertices
        glColor4d(this->Background2[0],this->Background2[1],
                  this->Background2[2],0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex2f(1.0, 1.0);

        glTexCoord2f(0.0, 1.0);
        glVertex2f(0.0, 1.0);

        glEnd();
        }
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      }
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
    }
  vtkOpenGLCheckErrorMacro("failed after Clear");
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderer::Render(void)
{
  GLdouble mv[16],p[16];
  glGetDoublev(GL_MODELVIEW_MATRIX,mv);
  glGetDoublev(GL_PROJECTION_MATRIX,p);

  vtkExternalOpenGLCamera* camera = vtkExternalOpenGLCamera::SafeDownCast(
    this->GetActiveCameraAndResetIfCreated());

  camera->SetProjectionTransformMatrix(p);
  camera->SetViewTransformMatrix(mv);

  vtkMatrix4x4* matrix = vtkMatrix4x4::New();
  matrix->DeepCopy(mv);
  matrix->Transpose();
  matrix->Invert();

  // Synchronize camera viewUp
  double viewUp[4] = {0.0, 1.0, 0.0, 0.0}, newViewUp[4];
  matrix->MultiplyPoint(viewUp, newViewUp);
  vtkMath::Normalize(newViewUp);
  camera->SetViewUp(newViewUp);

  // Synchronize camera position
  double position[4] = {0.0, 0.0, 1.0, 1.0}, newPosition[4];
  matrix->MultiplyPoint(position, newPosition);

  if (newPosition[3] != 0.0)
    {
    newPosition[0] /= newPosition[3];
    newPosition[1] /= newPosition[3];
    newPosition[2] /= newPosition[3];
    newPosition[3] = 1.0;
    }
  camera->SetPosition(newPosition);

  // Synchronize focal point
  double focalPoint[4] = {0.0, 0.0, 0.0, 1.0}, newFocalPoint[4];
  matrix->MultiplyPoint(focalPoint, newFocalPoint);
  camera->SetFocalPoint(newFocalPoint);

  matrix->DeepCopy(mv);
  matrix->Invert();

  vtkCollectionSimpleIterator sit;
  vtkLight* light;
  for (this->Lights->InitTraversal(sit);
    (light = this->Lights->GetNextLight(sit)); )
    {
    // If we set the transform matrix then even for the headlight
    // vtkOpenGLLight will use it and result in wrong lighting computation.
    // What we want is to use camera position and focal point only
    // when we have a head light.
    if (light->LightTypeIsCameraLight())
      {
      light->SetTransformMatrix(matrix);
      }
    }

  matrix->Delete();

  // Forward the call to the Superclass
  this->Superclass::Render();
}

//----------------------------------------------------------------------------
vtkCamera* vtkExternalOpenGLRenderer::MakeCamera()
{
  vtkCamera* cam = vtkExternalOpenGLCamera::New();
  this->InvokeEvent(vtkCommand::CreateCameraEvent, cam);
  return cam;
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderer::PrintSelf(ostream &os, vtkIndent indent)
{
  os << indent << "PreserveColorBuffer: " << this->PreserveColorBuffer << "\n";
  this->Superclass::PrintSelf(os, indent);
}
