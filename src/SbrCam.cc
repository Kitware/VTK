/*=========================================================================

  Program:   Visualization Library
  Module:    SbrCam.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "SbrCam.hh"
#include "SbrRenW.hh"
#include "SbrRen.hh"

static float xform[4][4] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

// Description:
// Implement base class method.
void vlSbrCamera::Render(vlRenderer *ren)
{
  this->Render((vlSbrRenderer *)ren);
}

// Description:
// Actual camera render method.
void vlSbrCamera::Render(vlSbrRenderer *ren)
{
  float aspect[3];
  float vaspect[2];
  float *viewport;
  float *background;
  int	width,height;
  long clr;
  int left,right,bottom,top;
  long xmax,ymax,zmax;
  float twist;
  int stereo;
  int fd;
  camera_arg cam;
  int *size;
  int *screen_size;
  vlSbrRenderWindow *rw;
  float trans[2];
  float old;

  fd = ren->GetFd();

  // find out if we should stereo render
  stereo = ren->GetStereoRender();

  // set up the view specification
  cam.field_of_view = this->ViewAngle;
  cam.front = this->ClippingRange[0] - this->Distance;
  cam.back = this->ClippingRange[1] - this->Distance;
  cam.camx = this->Position[0];
  cam.camy = this->Position[1];
  cam.camz = - this->Position[2];
  cam.refx = this->FocalPoint[0];
  cam.refy = this->FocalPoint[1];
  cam.refz = - this->FocalPoint[2];
  cam.upx = this->ViewUp[0];
  cam.upy = this->ViewUp[1];
  cam.upz = - this->ViewUp[2];
  cam.projection = CAM_PERSPECTIVE;

  // set this renderer's viewport, must turn off z-buffering when changing
  // viewport
  hidden_surface(fd, FALSE, FALSE);
  vlDebugMacro(<< " SB_hidden_surface: False False\n");

  viewport = ren->GetViewport();
  // get the background color
  background = ren->GetBackground();

  // get size info
  rw = (vlSbrRenderWindow*)(ren->GetRenderWindow());
  size = rw->GetSize();
  screen_size = rw->GetScreenSize();

  // make sure the aspect is up to date
  if (size[0] > size[1])
    {
    aspect[0] = size[0]/(float)size[1];
    aspect[1] = 1.0;
    }
  else
    {
    aspect[0] = 1.0;
    aspect[1] = size[1]/(float)size[0];
    }
  ren->SetAspect(aspect);

  vdc_extent(fd,0.0,-screen_size[1]/(float)size[1]+1.0,0.0,
	     screen_size[0]/(float)size[0],1.0,1.0);
  vlDebugMacro(<< " screen_size " << screen_size[0] << " " 
  << screen_size[1] << endl);

  vlDebugMacro(<< " size " << size[0] << " " << size[1] << endl);
  vlDebugMacro(<< " viewport " << viewport[0] << " " << viewport[1] 
  << " " << viewport[2] << " " << viewport[3] << endl);

  // set viewport to clear entire window 
  view_port(fd, viewport[0], viewport[1],
	    viewport[2], viewport[3]);

  hidden_surface(fd, TRUE, FALSE);
  vlDebugMacro(<< " SB_hidden_surface: True False\n");

  // Set the background color and clear the display.
  // Since clear control was set to clear z buffer, this is done here
  // also.
  background_color(fd, background[0], background[1], background[2]);
  
  // clear the view surface so the new background color takes effect
  if (ren->GetErase()) 
    {
    clear_view_surface(fd);
    vlDebugMacro(<< " SB_clear_view_surface\n");
    }

  hidden_surface(fd, FALSE, FALSE);
  vlDebugMacro(<< " SB_hidden_surface: False False\n");

  vdc_extent(fd,0.0,-screen_size[1]/(float)size[1]+1.0,0.0,
	     screen_size[0]/(float)size[0],1.0,1.0);
  view_port(fd, viewport[0], viewport[1],
	    viewport[2], viewport[3]);
  
  hidden_surface(fd, TRUE, FALSE);
  vlDebugMacro(<< " SB_hidden_surface: True False\n");

  // install the camera
  view_camera(fd, &cam); 

  // calculate the viewport aspect
  // we basically compensate for starbases attempt to compensate for
  // non square windows.
  vaspect[0] = viewport[2] - viewport[0];
  vaspect[1] = viewport[3] - viewport[1];
  if ((vaspect[0] >= vaspect[1])&&(aspect[0] <= aspect[1]))
    {
    old = vaspect[0];
    vaspect[0] = aspect[0]*vaspect[1]/old;
    vaspect[1] = aspect[1]*vaspect[1]/old;
    }
  else
    {
    if ((vaspect[0] <= vaspect[1])&&(aspect[0] >= aspect[1]))
      {
      old = vaspect[1];
      vaspect[1] = aspect[1]*vaspect[0]/old;
      vaspect[0] = aspect[0]*vaspect[0]/old;
      }
    }

  // another try 
  trans[0] = (viewport[2] + viewport[0])*(1.0 - 1.0/vaspect[0])/2.0;
  trans[1] = (viewport[3] + viewport[1])*(1.0 - 1.0/vaspect[1])/2.0;

  // now handle the anisotropy 
  xform[0][0] = 1.0/vaspect[0];
  xform[1][1] = 1.0/vaspect[1];
  xform[3][1] = trans[1];
  xform[3][0] = trans[0];
  view_matrix3d(fd, xform, POST_CONCAT_VW);

  // if we have a stereo renderer, draw other eye next time 
  if (stereo)
    {
    if (this->LeftEye) this->LeftEye = 0;
    else this->LeftEye = 1;
    }
}
