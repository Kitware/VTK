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

#define MAX_LIGHTS 8

vtkStandardNewMacro(vtkExternalOpenGLRenderer);

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderer::vtkExternalOpenGLRenderer()
{
  this->PreserveColorBuffer = 1;
  this->PreserveDepthBuffer = 1;
  this->SetAutomaticLightCreation(0);
}

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderer::~vtkExternalOpenGLRenderer()
{
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

  matrix->Delete();

  // Remove all VTK lights
  this->RemoveAllLights();

  // Query OpenGL lights
  GLenum curLight;
  for (curLight = GL_LIGHT0;
       curLight < GL_LIGHT0 + MAX_LIGHTS;
       curLight++)
    {
    GLboolean status;
    GLfloat info[4];
    glGetBooleanv(curLight, &status);
    if (status)
      {
      // For each enabled OpenGL light, add a new VTK headlight.
      // Headlight because VTK will apply transform matrices.
      vtkLight* light = vtkLight::New();
      light->SetLightTypeToHeadlight();

      // Set color parameters
      glGetLightfv(curLight, GL_AMBIENT, info);
      light->SetAmbientColor(info[0], info[1], info[2]);
      glGetLightfv(curLight, GL_DIFFUSE, info);
      light->SetDiffuseColor(info[0], info[1], info[2]);
      glGetLightfv(curLight, GL_SPECULAR, info);
      light->SetSpecularColor(info[0], info[1], info[2]);

      // Position, focal point and positional
      glGetLightfv(curLight, GL_POSITION, info);
      light->SetPositional(info[3] > 0.0 ? 1 : 0);
      if (!light->GetPositional())
        {
        light->SetFocalPoint(0, 0, 0);
        light->SetPosition(-info[0], -info[1], -info[2]);
        }
      else
        {
        light->SetFocalPoint(0, 0, 0);
        light->SetPosition(info[0], info[1], info[2]);

        // Attenuation
        glGetLightfv(curLight, GL_CONSTANT_ATTENUATION, &info[0]);
        glGetLightfv(curLight, GL_LINEAR_ATTENUATION, &info[1]);
        glGetLightfv(curLight, GL_QUADRATIC_ATTENUATION, &info[2]);
        light->SetAttenuationValues(info[0], info[1], info[2]);

        // Cutoff
        glGetLightfv(curLight, GL_SPOT_CUTOFF, &info[0]);
        light->SetConeAngle(info[0]);

        if (light->GetConeAngle() < 180.0)
          {
          // Exponent
          glGetLightfv(curLight, GL_SPOT_EXPONENT, &info[0]);
          light->SetExponent(info[0]);

          // Direction
          glGetLightfv(curLight, GL_SPOT_DIRECTION, info);
          for (unsigned int i = 0; i < 3; ++i)
            {
            info[i] += light->GetPosition()[i];
            }
          light->SetFocalPoint(info[0], info[1], info[2]);
          }
        }
      this->AddLight(light);
      light->Delete();
      }
    }

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
