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

int vlGlrRenderer::UpdateActors()
{
  vlActor *anActor;
  float visibility;
  vlMatrix4x4 matrix;
  int   num;
  int count = 0;
 
  // set matrix mode for actors 
  mmode(MVIEWING);

  // loop through actors 
  for (num = 1; num <= this->Actors.GetNumberOfItems(); num++)
    {
    anActor = this->Actors.GetItem(num);
 
    // if it's invisible, we can skip the rest 
    visibility = anActor->GetVisibility();

    if (visibility == 1.0)
      {
      count++;
      // build transformation 
      matrix = anActor->GetMatrix();
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

int vlGlrRenderer::UpdateCameras ()
{
  // update the viewing transformation 
  if (!this->ActiveCamera) return 0;
  
  this->ActiveCamera->Render((vlRenderer *)this);
  return 1;
}

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

int vlGlrRenderer::UpdateLights ()
{
  vlLight *light;
  short cur_light;
  float status;
  int i;
  int count = 0;

  cur_light= this->NumberOfLightsBound + LIGHT0;

  // set the matrix mode for lighting. ident matrix on viewing stack  
  mmode(MVIEWING);
  pushmatrix();

  for (i = 1; i <= this->Lights.GetNumberOfItems(); i++)
    {
    light = this->Lights.GetItem(i);

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
  if (this->ShouldIPrint(vlGlrRenderer::GetClassName()))
    {
    this->vlRenderer::PrintSelf(os,indent);

    os << indent << "Number Of Lights Bound: " << 
      this->NumberOfLightsBound << "\n";
    }
}

