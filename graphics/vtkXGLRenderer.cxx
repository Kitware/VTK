/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XglrRen.cc
  Language:  C++
  Date:      04/20/95
  Version:   1.2


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkRayCaster.h"



vtkXGLRenderer::vtkXGLRenderer()
{
  this->NumberOfLightsBound = 0;
}

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
 
// Concrete XGL render method.
void vtkXGLRenderer::DeviceRender(void)
{
  int  actor_count;
  int  volume_count;
  float  scale_factor;
  float  saved_viewport[4];
  float  new_viewport[4];
  int    saved_erase;

  vtkXGLRenderWindow *temp;

  volume_count = this->VisibleVolumeCount();

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

  // If there is a volume renderer, get it's desired viewport size
  // since it may want to render actors into a smaller area for multires
  // rendering during motion
  if ( volume_count > 0 )
    {
    // Get the scale factor
    scale_factor = this->RayCaster->GetViewportScaleFactor( (vtkRenderer *)this);

    // If the volume renderer wants a different resolution than this
    // renderer was going to produce we need to set up the viewport
    if ( scale_factor != 1.0 )
      {
      // Get the current viewport
      this->GetViewport( saved_viewport );

      // Create a new viewport size based on the scale factor
      new_viewport[0] = saved_viewport[0];
      new_viewport[1] = saved_viewport[1];
      new_viewport[2] = saved_viewport[0] +
        scale_factor * ( saved_viewport[2] - saved_viewport[0] );
      new_viewport[3] = saved_viewport[1] +
        scale_factor * ( saved_viewport[3] - saved_viewport[1] );

      // Set this as the new viewport.  This will cause the OpenGL
      // viewport to be set correctly in the camera render method
      this->SetViewport( new_viewport );
      }
    }

  // standard render method 
  this->UpdateCameras();
  this->UpdateLights();

  actor_count = this->UpdateActors();

  // If we are rendering with a reduced size image for the volume
  // rendering, then we need to reset the viewport so that the
  // volume renderer can access the whole window to draw the image.
  // We'll pop off what we've done so far, then we'll save the state
  // of the erase variable in the render window. We will then set the
  // erase variable in the render window to 0, and render the camera
  // again.  This will set our viewport back to the right size.
  // Finally, we restore the erase variable in the render window
  if ( volume_count > 0  && scale_factor != 1.0 )
    {
    saved_erase = this->RenderWindow->GetErase();
    this->RenderWindow->SetErase( 0 );
    this->SetViewport( saved_viewport );
    this->ActiveCamera->Render( (vtkRenderer *)this );
    this->RenderWindow->SetErase( saved_erase );
    }

  volume_count = this->UpdateVolumes();

  if ( !(actor_count + volume_count) )
    {
    vtkWarningMacro(<< "No actors or volumes are on.");
    }
}

void vtkXGLRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkRenderer::PrintSelf(os,indent);

  os << indent << "Number Of Lights Bound: " << 
    this->NumberOfLightsBound << "\n";
}

