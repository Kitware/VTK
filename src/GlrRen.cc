/*=========================================================================

  Program:   Visualization Library
  Module:    GlrRen.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include <iostream.h>
#include "GlrRen.hh"
#include "GlrRenW.hh"
#include "GlrProp.hh"
#include "GlrCam.hh"
#include "GlrLgt.hh"
#include "GlrPoly.hh"
#include "GlrTri.hh"
#include "GlrLine.hh"
#include "GlrPnt.hh"

#define MAX_LIGHTS 8

static float amb_light_info[] = {
  AMBIENT, 0.2, 0.2, 0.2,
  LMNULL
  };

vlGlrRenderer::vlGlrRenderer()
{
}

// Description:
// Ask actors to build and draw themselves.
int vlGlrRenderer::UpdateActors()
{
  vlActor *anActor;
  float visibility;
  vlMatrix4x4 matrix;
  int count = 0;
 
  // set matrix mode for actors 
  mmode(MVIEWING);

  // loop through actors 
  for ( this->Actors.InitTraversal(); anActor = this->Actors.GetNextItem(); )
    {
    // if it's invisible, we can skip the rest 
    visibility = anActor->GetVisibility();

    if (visibility == 1.0)
      {
      count++;
      // build transformation 
      anActor->GetMatrix(matrix);
      matrix.Transpose();
 
      // insert model transformation 
      pushmatrix();
      multmatrix((float (*)[4])(matrix[0]));
 
      anActor->Render((vlRenderer *)this);
 
      popmatrix();
      }
    }
  return count;
}

// Description:
// Ask active camera to load its view matrix.
int vlGlrRenderer::UpdateCameras ()
{
  // update the viewing transformation 
  if (!this->ActiveCamera) return 0;
  
  this->ActiveCamera->Render((vlRenderer *)this);
  return 1;
}

// Description:
// Internal method temporarily removes lights before reloading them
// into graphics pipeline.
void vlGlrRenderer::ClearLights (void)
{
  short cur_light;

  // define a lighting model and set up the ambient light.
  // use index 11 for the heck of it. Doesn't matter except for 0.
   
  // update the ambient light 
  amb_light_info[1] = this->Ambient[0];
  amb_light_info[2] = this->Ambient[1];
  amb_light_info[3] = this->Ambient[2];

  lmdef(DEFLMODEL, 11, 0, amb_light_info);
  lmbind(LMODEL, 11);

  // now delete all the old lights 
  for (cur_light = LIGHT0; 
       cur_light < LIGHT0 + MAX_LIGHTS; cur_light++)
    {
    lmbind(cur_light,0);
    }

  this->NumberOfLightsBound = 0;
}

// Description:
// Ask lights to load themselves into graphics pipeline.
int vlGlrRenderer::UpdateLights ()
{
  vlLight *light;
  short cur_light;
  float status;
  int count = 0;

  cur_light= this->NumberOfLightsBound + LIGHT0;

  // set the matrix mode for lighting. ident matrix on viewing stack  
  mmode(MVIEWING);
  pushmatrix();

  for ( this->Lights.InitTraversal(); light = this->Lights.GetNextItem(); )
    {

    status = light->GetSwitch();

    // if the light is on then define it and bind it. 
    // also make sure we still have room.             
    if ((status > 0.0)&& (cur_light < (LIGHT0+MAX_LIGHTS)))
      {
      light->Render((vlRenderer *)this,cur_light);
      lmbind(cur_light, cur_light);
      // increment the current light by one 
      cur_light++;
      count++;
      // and do the same for the mirror source if backlit is on
      // and we aren't out of lights
      if ((this->BackLight > 0.0) && 
	  (cur_light < (LIGHT0+MAX_LIGHTS)))
	{
	lmbind(cur_light,cur_light);
	// if backlighting is on then increment the current light again 
	cur_light++;
	}
      }
    }
  
  this->NumberOfLightsBound = cur_light - LIGHT0;
  
  popmatrix();
  return count;
}
 
// Description:
// Concrete gl render method.
void vlGlrRenderer::Render(void)
{
  // standard render method 
  this->ClearLights();
  this->DoCameras();
  this->DoLights();
  this->DoActors();
  // clean up the model view matrix set up by the camera 
  mmode(MVIEWING);
  popmatrix();
}

// Description:
// Create particular type of gl geometry primitive.
vlGeometryPrimitive *vlGlrRenderer::GetPrimitive(char *type)
{
  vlGeometryPrimitive *prim;

  if (!strcmp(type,"polygons"))
      {
      prim = new vlGlrPolygons;
      return (vlGeometryPrimitive *)prim;
      }
  if (!strcmp(type,"triangle_strips"))
      {
      prim = new vlGlrTriangleMesh;
      return (vlGeometryPrimitive *)prim;
      }
  if (!strcmp(type,"lines"))
      {
      prim = new vlGlrLines;
      return (vlGeometryPrimitive *)prim;
      }
  if (!strcmp(type,"points"))
      {
      prim = new vlGlrPoints;
      return (vlGeometryPrimitive *)prim;
      }

  return((vlGeometryPrimitive *)NULL);
}

void vlGlrRenderer::PrintSelf(ostream& os, vlIndent indent)
{
  this->vlRenderer::PrintSelf(os,indent);

  os << indent << "Number Of Lights Bound: " << 
    this->NumberOfLightsBound << "\n";
}


// Description:
// Return center of renderer in display coordinates.
float *vlGlrRenderer::GetCenter()
{
  int *size;
  
  // get physical window dimensions 
  size = this->RenderWindow->GetSize();

  if (this->RenderWindow->GetStereoRender())
    {
    // take into account stereo effects
    switch (this->RenderWindow->GetStereoType()) 
      {
      case VL_STEREO_CRYSTAL_EYES:
	{
	this->Center[0] = ((this->Viewport[2]+this->Viewport[0])
				/2.0*(float)size[0]);
	this->Center[1] = ((this->Viewport[3]+this->Viewport[1])
				/2.0*(float)size[1]);
	this->Center[1] = this->Center[1]*(491.0/1024.0);
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
void vlGlrRenderer::DisplayToView()
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
      case VL_STEREO_CRYSTAL_EYES:
	{
	vx = 2.0 * (this->DisplayPoint[0] - sizex*this->Viewport[0])/ 
	  (sizex*(this->Viewport[2]-this->Viewport[0])) - 1.0;

	vy = 2.0 * (this->DisplayPoint[1]*(1024.0/491.0) - 
		    sizey*this->Viewport[1])/ 
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
void vlGlrRenderer::ViewToDisplay()
{
  int dx,dy;
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
      case VL_STEREO_CRYSTAL_EYES:
	{
	dx = (int)((this->ViewPoint[0]/this->Aspect[0] + 1.0) * 
		   (sizex*(this->Viewport[2]-this->Viewport[0])) / 2.0 +
		   sizex*this->Viewport[0]);
	dy = (int)((this->ViewPoint[1]/this->Aspect[1] + 1.0) * 
		   (sizey*(this->Viewport[3]-this->Viewport[1])) / 2.0 +
		   sizey*this->Viewport[1]);
	dy = (int)(dy*(491.0/1024.0));
	}
	break;
      default:
	{
	dx = (int)((this->ViewPoint[0]/this->Aspect[0] + 1.0) * 
		   (sizex*(this->Viewport[2]-this->Viewport[0])) / 2.0 +
		   sizex*this->Viewport[0]);
	dy = (int)((this->ViewPoint[1]/this->Aspect[1] + 1.0) * 
		   (sizey*(this->Viewport[3]-this->Viewport[1])) / 2.0 +
		   sizey*this->Viewport[1]);
	}
      }
    }
  else
    {
    dx = (int)((this->ViewPoint[0]/this->Aspect[0] + 1.0) * 
	       (sizex*(this->Viewport[2]-this->Viewport[0])) / 2.0 +
	       sizex*this->Viewport[0]);
    dy = (int)((this->ViewPoint[1]/this->Aspect[1] + 1.0) * 
	       (sizey*(this->Viewport[3]-this->Viewport[1])) / 2.0 +
	       sizey*this->Viewport[1]);
    }

  this->SetDisplayPoint(dx,dy,this->ViewPoint[2]);
}


// Description:
// Is a given display point in this renderer's viewport.
int vlGlrRenderer::IsInViewport(int x,int y)
{
  int *size;
  
  // get physical window dimensions 
  size = this->RenderWindow->GetSize();


  if (this->RenderWindow->GetStereoRender())
    {
    // take into account stereo effects
    switch (this->RenderWindow->GetStereoType()) 
      {
      case VL_STEREO_CRYSTAL_EYES:
	{
	int ty = (int)(y*(1023.0/491.0));

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
