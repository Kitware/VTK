/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XglrRen.cc
  Language:  C++
  Date:      04/20/95
  Version:   1.2


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <math.h>
#include <iostream.h>
#include "vtkXGLProperty.h"
#include "vtkXGLCamera.h"
#include "vtkXGLLight.h"
#include "vtkXGLRenderWindow.h"
#include "vtkXGLRenderer.h"
#include "vtkNewVolumeRenderer.h"


vtkXGLRenderer::vtkXGLRenderer()
{
}

// Description:
// Ask actors to build and draw themselves.
int vtkXGLRenderer::UpdateActors()
{
  vtkActor *anActor;
  int count = 0;

  // loop through actors 
  for (this->Actors.InitTraversal(); (anActor = this->Actors.GetNextItem()); )
    {
    // if it's invisible, we can skip the rest 
    if (anActor->GetVisibility())
      {
      count++;
      anActor->Render((vtkRenderer *)this);
      }
    }
  return count;
}

// Description:
// Ask volumes to render themselves.
int vtkXGLRenderer::UpdateVolumes()
{
  int count = 0;

  if (this->NewVolumeRenderer)
    {
    this->NewVolumeRenderer->Render((vtkRenderer *)this);
    count++;
    }

  return count;
}

// Description:
// Ask active camera to load its view matrix.
int vtkXGLRenderer::UpdateCameras ()
{
  if (!this->ActiveCamera)
    {
    vtkDebugMacro(<< "No cameras are on, creating one.");
    // the get method will automagically create a camera
    // and reset it since one hasn't been specified yet
    this->GetActiveCamera();
    }

  // update the viewing transformation   
  this->ActiveCamera->Render((vtkRenderer *)this);
  return 1;
}

// Description:
// Ask lights to load themselves into graphics pipeline.
int vtkXGLRenderer::UpdateLights ()
{
  vtkLight *light;
  short cur_light, idx;
  float status;
  int count;
  Xgl_boolean xglr_switches[VTK_MAX_LIGHTS];
  Xgl_color   light_color;

  // Check if a light is on. If not then make a new light.
  count = 0;
  cur_light = 1;

  for(this->Lights.InitTraversal(); 
      (light = this->Lights.GetNextItem()); )
    {
    status = light->GetSwitch();
    if ((status > 0.0)&& (cur_light < VTK_MAX_LIGHTS))
      {
      cur_light++;
      count++;
      }
    }

  if( !count )
    {
    vtkDebugMacro(<<"No lights are on, creating one.");
    this->CreateLight();
    }

  // first get the lights and switched from the context 
  xgl_object_get(this->Context,XGL_3D_CTX_LIGHTS, this->XglrLights);
  xgl_object_get(this->Context,XGL_3D_CTX_LIGHT_SWITCHES, xglr_switches);
  
  // update the ambient light light# 0
  light_color.rgb.r = this->Ambient[0];
  light_color.rgb.g = this->Ambient[1];
  light_color.rgb.b = this->Ambient[2];
  xgl_object_set(this->XglrLights[0],
		 XGL_LIGHT_TYPE, XGL_LIGHT_AMBIENT,
		 XGL_LIGHT_COLOR, &light_color,
		 NULL);
  
  // set all lights off except the ambient light 
  xglr_switches[0] = TRUE;
  for (idx = 1; idx < VTK_MAX_LIGHTS; idx++)
    {
    xglr_switches[idx] = FALSE;
    }
  
  count = 0;
  cur_light = 1;

  for (this->Lights.InitTraversal(); 
       (light = this->Lights.GetNextItem()); )
    {
    status = light->GetSwitch();

    // if the light is on then define it and bind it. 
    // also make sure we still have room.             
    if ((status > 0.0)&& (cur_light < VTK_MAX_LIGHTS))
      {
      light->Render((vtkRenderer *)this,cur_light);
      xglr_switches[cur_light] = TRUE;
      // increment the current light by one 
      cur_light++;
      count++;
      }
    }
  
  // now update the context
  xgl_object_set(this->Context,
		 XGL_3D_CTX_LIGHT_SWITCHES, xglr_switches,
		 NULL);

  this->NumberOfLightsBound = cur_light;
  
  return count;
}
 
// Description:
// Concrete XGL render method.
void vtkXGLRenderer::Render(void)
{
  int  actor_count;
  int  volume_count;

  vtkXGLRenderWindow *temp;

  if (this->StartRenderMethod) 
    {
    (*this->StartRenderMethod)(this->StartRenderMethodArg);
    }

  // update our Context first
  temp = (vtkXGLRenderWindow *)this->GetRenderWindow();
  this->Context = *(temp->GetContext());

  if ( this->TwoSidedLighting )
    {
    xgl_object_set(this->Context, XGL_3D_CTX_SURF_FACE_DISTINGUISH, TRUE, 0);
    }
  else
    {
    xgl_object_set(this->Context, XGL_3D_CTX_SURF_FACE_DISTINGUISH, FALSE, 0);
    }

  // standard render method 
  this->UpdateCameras();
  this->UpdateLights();

  actor_count = this->UpdateActors();
  volume_count = this->UpdateVolumes();

  if ( !(actor_count + volume_count) )
    {
    vtkWarningMacro(<< "No actors or volumes are on.");
    }

  if (this->EndRenderMethod) 
    {
    (*this->EndRenderMethod)(this->EndRenderMethodArg);
    }
}

void vtkXGLRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkRenderer::PrintSelf(os,indent);

  os << indent << "Number Of Lights Bound: " << 
    this->NumberOfLightsBound << "\n";
}

