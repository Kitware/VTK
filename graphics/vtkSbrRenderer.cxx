/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSbrRenderer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkSbrProperty.hh"
#include "vtkSbrCamera.hh"
#include "vtkSbrLight.hh"
#include "vtkSbrRenderWindow.hh"
#include "vtkSbrRenderer.hh"
#include "vtkVolumeRenderer.hh"
#include "vtkNewVolumeRenderer.hh"

#define MAX_LIGHTS 16

vtkSbrRenderer::vtkSbrRenderer()
{
}

// Description:
// Ask actors to build and draw themselves.
int vtkSbrRenderer::UpdateActors()
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
int vtkSbrRenderer::UpdateVolumes()
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
int vtkSbrRenderer::UpdateCameras ()
{
  // update the viewing transformation 
  if (!this->ActiveCamera)
    {
    vtkDebugMacro(<< "No cameras are on, creating one.");
    // the get method will automagically create a camera
    // and reset it since one hasn't been specified yet
    this->GetActiveCamera();
    }
  
  this->ActiveCamera->Render((vtkRenderer *)this);

  return 1;
}

// Description:
// Internal method temporarily removes lights before reloading them
// into graphics pipeline.
void vtkSbrRenderer::ClearLights (void)
{
  light_ambient(this->Fd, this->Ambient[0],
		this->Ambient[1], this->Ambient[2]);
  this->LightSwitch = 0x0001;
  
  vtkDebugMacro(<< "SB_light_ambient: " << this->Ambient[0] << " " <<
  this->Ambient[1] << " " << this->Ambient[2] << "\n");
  
  light_switch(this->Fd, this->LightSwitch);
 
  vtkDebugMacro( << " SB_light_switch: " << this->LightSwitch << "\n");

  this->NumberOfLightsBound = 1;
}

// Description:
// Ask lights to load themselves into graphics pipeline.
int vtkSbrRenderer::UpdateLights ()
{
  vtkLight *light;
  short curLight;
  float status;
  int count;

  // Check if a light is on. If not then make a new light.
  count = 0;
  curLight= this->NumberOfLightsBound;

  for(this->Lights.InitTraversal(); 
      (light = this->Lights.GetNextItem()); )
    {
    status = light->GetSwitch();
    if ((status > 0.0)&& (curLight < MAX_LIGHTS))
      {
      curLight++;
      count++;
      }
    }

  if( !count )
    {
    vtkDebugMacro(<<"No lights are on, creating one.");
    this->CreateLight();
    }

  count = 0;
  curLight= this->NumberOfLightsBound;

  for (this->Lights.InitTraversal(); (light = this->Lights.GetNextItem()); )
    {

    status = light->GetSwitch();

    // if the light is on then define it and bind it. 
    // also make sure we still have room.             
    if ((status > 0.0)&& (curLight < MAX_LIGHTS))
      {
      light->Render((vtkRenderer *)this,curLight);
      // increment the current light by one 
      curLight++;
      count++;
      }
    }
  
  this->NumberOfLightsBound = curLight;
  
  return count;
}
 
// Description:
// Concrete starbase render method.
void vtkSbrRenderer::Render(void)
{
  int  actor_count;
  int  volume_count;

  vtkSbrRenderWindow *temp;

  if (this->StartRenderMethod) 
    {
    (*this->StartRenderMethod)(this->StartRenderMethodArg);
    }

  // update our Fd first
  temp = (vtkSbrRenderWindow *)this->GetRenderWindow();
  this->Fd = temp->GetFd();

  if ( this->TwoSidedLighting )
    {
    bf_control(this->Fd, FALSE, TRUE);
    }
  else
    {
    bf_control(this->Fd, FALSE, FALSE);
    }

  // standard render method 
  this->ClearLights();

  this->UpdateCameras();
  this->UpdateLights();

  actor_count = this->UpdateActors();
  volume_count = this->UpdateVolumes();

  if ( !(actor_count + volume_count) )
    {
    vtkWarningMacro(<< "No actors or volumes are on.");
    }

  if (this->VolumeRenderer)
    {
    this->VolumeRenderer->Render((vtkRenderer *)this);
    }

  if (this->EndRenderMethod) 
    {
    (*this->EndRenderMethod)(this->EndRenderMethodArg);
    }
}

// Description:
// Return center of renderer in display coordinates.
float *vtkSbrRenderer::GetCenter()
{
  int *size;
  
  // get physical window dimensions 
  size = this->RenderWindow->GetSize();

  if (this->RenderWindow->GetStereoRender())
    {
    // take into account stereo effects
    switch (this->RenderWindow->GetStereoType()) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
	this->Center[0] = ((this->Viewport[2]+this->Viewport[0])
				/2.0*(float)size[0]);
	this->Center[1] = ((this->Viewport[3]+this->Viewport[1])
				/2.0*(float)size[1]);
	this->Center[1] = this->Center[1]/2.0;
	}
	break;
      default:
	{
	this->Center[0] = ((this->Viewport[2]+this->Viewport[0])
			   /2.0*(float)size[0]);
	this->Center[1] = ((this->Viewport[3]+this->Viewport[1])
			   /2.0*(float)size[1]);
	}
      }
    }
  else
    {
    this->Center[0] = ((this->Viewport[2]+this->Viewport[0])
			    /2.0*(float)size[0]);
    this->Center[1] = ((this->Viewport[3]+this->Viewport[1])
			    /2.0*(float)size[1]);
    }

  return this->Center;
}


// Description:
// Convert display coordinates to view coordinates.
void vtkSbrRenderer::DisplayToView()
{
  float vx,vy,vz;
  int sizex,sizey;
  int *size;
  
  /* get physical window dimensions */
  size = this->RenderWindow->GetSize();
  sizex = size[0];
  sizey = size[1];

  if (this->RenderWindow->GetStereoRender())
    {
    // take into account stereo effects
    switch (this->RenderWindow->GetStereoType()) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
	vx = 2.0 * (this->DisplayPoint[0] - sizex*this->Viewport[0])/ 
	  (sizex*(this->Viewport[2]-this->Viewport[0])) - 1.0;

	vy = 2.0 * (this->DisplayPoint[1]*2.0 - sizey*this->Viewport[1])/ 
	  (sizey*(this->Viewport[3]-this->Viewport[1])) - 1.0;
	}
	break;
      default:
	{
	vx = 2.0 * (this->DisplayPoint[0] - sizex*this->Viewport[0])/ 
	  (sizex*(this->Viewport[2]-this->Viewport[0])) - 1.0;
	vy = 2.0 * (this->DisplayPoint[1] - sizey*this->Viewport[1])/ 
	  (sizey*(this->Viewport[3]-this->Viewport[1])) - 1.0;
	}
      }
    }
  else
    {
    vx = 2.0 * (this->DisplayPoint[0] - sizex*this->Viewport[0])/ 
      (sizex*(this->Viewport[2]-this->Viewport[0])) - 1.0;
    vy = 2.0 * (this->DisplayPoint[1] - sizey*this->Viewport[1])/ 
      (sizey*(this->Viewport[3]-this->Viewport[1])) - 1.0;
    }

  vz = this->DisplayPoint[2];

  this->SetViewPoint(vx*this->Aspect[0],vy*this->Aspect[1],vz);
}

// Description:
// Convert view coordinates to display coordinates.
void vtkSbrRenderer::ViewToDisplay()
{
  float dx,dy;
  int sizex,sizey;
  int *size;
  
  /* get physical window dimensions */
  size = this->RenderWindow->GetSize();
  sizex = size[0];
  sizey = size[1];

  if (this->RenderWindow->GetStereoRender())
    {
    // take into account stereo effects
    switch (this->RenderWindow->GetStereoType()) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
	dx = (this->ViewPoint[0]/this->Aspect[0] + 1.0) * 
	  (sizex*(this->Viewport[2]-this->Viewport[0])) / 2.0 +
	  sizex*this->Viewport[0];
	dy = (this->ViewPoint[1]/this->Aspect[1] + 1.0) * 
	  (sizey*(this->Viewport[3]-this->Viewport[1])) / 2.0 +
	  sizey*this->Viewport[1];
	dy = dy/2.0;
	}
	break;
      default:
	{
	dx = (this->ViewPoint[0]/this->Aspect[0] + 1.0) * 
	  (sizex*(this->Viewport[2]-this->Viewport[0])) / 2.0 +
	  sizex*this->Viewport[0];
	dy = (this->ViewPoint[1]/this->Aspect[1] + 1.0) * 
	  (sizey*(this->Viewport[3]-this->Viewport[1])) / 2.0 +
	  sizey*this->Viewport[1];
	}
      }
    }
  else
    {
    dx = (this->ViewPoint[0]/this->Aspect[0] + 1.0) * 
      (sizex*(this->Viewport[2]-this->Viewport[0])) / 2.0 +
      sizex*this->Viewport[0];
    dy = (this->ViewPoint[1]/this->Aspect[1] + 1.0) * 
      (sizey*(this->Viewport[3]-this->Viewport[1])) / 2.0 +
      sizey*this->Viewport[1];
    }

  this->SetDisplayPoint(dx,dy,this->ViewPoint[2]);
}

// Description:
// Is a given display point in this renderer's viewport.
int vtkSbrRenderer::IsInViewport(int x,int y)
{
  int *size;
  
  // get physical window dimensions 
  size = this->RenderWindow->GetSize();


  if (this->RenderWindow->GetStereoRender())
    {
    // take into account stereo effects
    switch (this->RenderWindow->GetStereoType()) 
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
	int ty = y*2;

	if ((this->Viewport[0]*size[0] <= x)&&
	    (this->Viewport[2]*size[0] >= x)&&
	    (this->Viewport[1]*size[1] <= ty)&&
	    (this->Viewport[3]*size[1] >= ty))
	  {
	  return 1;
	  }
	}
	break;
      default:
	{
	if ((this->Viewport[0]*size[0] <= x)&&
	    (this->Viewport[2]*size[0] >= x)&&
	    (this->Viewport[1]*size[1] <= y)&&
	    (this->Viewport[3]*size[1] >= y))
	  {
	  return 1;
	  }
	}
      }
    }
  else
    {
    if ((this->Viewport[0]*size[0] <= x)&&
	(this->Viewport[2]*size[0] >= x)&&
	(this->Viewport[1]*size[1] <= y)&&
	(this->Viewport[3]*size[1] >= y))
      {
      return 1;
      }
    }
  
  return 0;
}

void vtkSbrRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkRenderer::PrintSelf(os,indent);

  os << indent << "Number Of Lights Bound: " << 
    this->NumberOfLightsBound << "\n";
}

