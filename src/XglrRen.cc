/*=========================================================================

  Program:   Visualization Library
  Module:    XglrRen.cc
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
#include "XglrProp.hh"
#include "XglrCam.hh"
#include "XglrLgt.hh"
#include "XglrRenW.hh"
#include "XglrRen.hh"
#include "XglrPoly.hh"
#include "XglrTri.hh"
#include "XglrLine.hh"
#include "XglrPnt.hh"
#include "VolRen.hh"


vlXglrRenderer::vlXglrRenderer()
{
}

// Description:
// Ask actors to build and draw themselves.
int vlXglrRenderer::UpdateActors()
{
  vlActor *anActor;
  float visibility;
  vlMatrix4x4 matrix;
  int count = 0;
  Xgl_trans model_trans;

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

      // shold we transpose this matrix ?
      matrix.Transpose();
 
      // insert model transformation 
      xgl_object_get(this->Context,
		     XGL_CTX_GLOBAL_MODEL_TRANS, &model_trans);
      xgl_transform_write(model_trans,(float (*)[4])(matrix[0]));

      anActor->Render((vlRenderer *)this);
      }
    }
  return count;
}

// Description:
// Ask active camera to load its view matrix.
int vlXglrRenderer::UpdateCameras ()
{
  // update the viewing transformation 
  if (!this->ActiveCamera) return 0;
  
  this->ActiveCamera->Render((vlRenderer *)this);
  return 1;
}

// Description:
// Ask lights to load themselves into graphics pipeline.
int vlXglrRenderer::UpdateLights ()
{
  vlLight *light;
  short cur_light, idx;
  float status;
  int count = 0;
  Xgl_boolean xglr_switches[MAX_LIGHTS];
  Xgl_color   light_color;

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
  for (idx = 1; idx < MAX_LIGHTS; idx++)
    {
    xglr_switches[idx] = FALSE;
    }
  
  cur_light = 1;

  for ( this->Lights.InitTraversal(); light = this->Lights.GetNextItem(); )
    {
    status = light->GetSwitch();

    // if the light is on then define it and bind it. 
    // also make sure we still have room.             
    if ((status > 0.0)&& (cur_light < MAX_LIGHTS))
      {
      light->Render((vlRenderer *)this,cur_light);
      xglr_switches[cur_light] = TRUE;
      // increment the current light by one 
      cur_light++;
      count++;
      // and do the same for the mirror source if backlit is on
      // and we aren't out of lights
      if ((this->BackLight > 0.0) && 
	  (cur_light < MAX_LIGHTS))
	{
	xglr_switches[cur_light] = TRUE;
	// if backlighting is on then increment the current light again 
	cur_light++;
	}
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
// Concrete starbase render method.
void vlXglrRenderer::Render(void)
{
  vlXglrRenderWindow *temp;

  if (this->StartRenderMethod) 
    {
    (*this->StartRenderMethod)(this->StartRenderMethodArg);
    }

  // update our Context first
  temp = (vlXglrRenderWindow *)this->GetRenderWindow();
  this->Context = *(temp->GetContext());

  // standard render method 
  this->DoCameras();
  this->DoLights();
  this->DoActors();
  if (this->VolumeRenderer)
    {
    this->VolumeRenderer->Render((vlRenderer *)this);
    }

  if (this->EndRenderMethod) 
    {
    (*this->EndRenderMethod)(this->EndRenderMethodArg);
    }
}

// Description:
// Create particular type of starbase geometry primitive.
vlGeometryPrimitive *vlXglrRenderer::GetPrimitive(char *type)
{
  vlGeometryPrimitive *prim;

  if (!strcmp(type,"polygons"))
      {
      prim = new vlXglrPolygons;
      return (vlGeometryPrimitive *)prim;
      }

  if (!strcmp(type,"triangle_strips"))
      {
      prim = new vlXglrTriangleMesh;
      return (vlGeometryPrimitive *)prim;
      }
  if (!strcmp(type,"lines"))
      {
      prim = new vlXglrLines;
      return (vlGeometryPrimitive *)prim;
      }
  if (!strcmp(type,"points"))
      {
      prim = new vlXglrPoints;
      return (vlGeometryPrimitive *)prim;
      }

  return((vlGeometryPrimitive *)NULL);
}

void vlXglrRenderer::PrintSelf(ostream& os, vlIndent indent)
{
  this->vlRenderer::PrintSelf(os,indent);

  os << indent << "Number Of Lights Bound: " << 
    this->NumberOfLightsBound << "\n";
}

