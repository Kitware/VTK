/*=========================================================================

  Program:   Visualization Library
  Module:    XglrCam.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "XglrCam.hh"
#include "XglrRenW.hh"
#include "XglrRen.hh"
#include <xgl/xgl.h>

extern Xgl_sys_state xglr_sys_state;

/****
 *
 * view_calc
 *
 * Calculate the View Transform consisting of the orientation given by
 * the eye position and the perspective given by the field of view.
 * Then redisplay the wireframe object.
 *
 ***/
static void
view_calc (Xgl_pt_f3d *eye, Xgl_pt_f3d *focus, 
	   float near, float far, float fov, float twist, 
	   Xgl_trans *trans, float conv, float eye_ang, float aspect)
{
#ifndef PI
#define PI 3.1415926
#endif
  Xgl_pt               pt;
  Xgl_pt_f3d           pt_f3d;
  Xgl_trans            view_trans;
  Xgl_trans            perspective_trans;
  Xgl_matrix_f3d       matrix;
  float                temp;
  float                distance;
  float                x_view_ratio;
  float                distance_old;
  int i;
  float gltan;
  float top,bottom,left,right;

  /* do some stereo calculations */
  eye_ang = tan(eye_ang*PI/180.0)*conv;
  gltan = tan(fov*PI/360.0);
  top = gltan*near;
  bottom = -top;
  gltan = tan(fov*aspect*M_PI/360.0);
  left = bottom*aspect - eye_ang/conv*near;
  right = top*aspect - eye_ang/conv*near;

  /* Translate the origin of VDC to the eye position in WC */
  pt.pt_type = XGL_PT_F3D;
  pt.pt.f3d = &pt_f3d;
  pt_f3d.x = -eye->x;
  pt_f3d.y = -eye->y;
  pt_f3d.z = -eye->z;
  xgl_transform_translate (*trans, &pt, XGL_TRANS_REPLACE);
  
  /* first rotate y */
  distance = sqrt((eye->x-focus->x)*(eye->x-focus->x) +
		  (eye->z-focus->z)*(eye->z-focus->z));
  if (distance > 0.0)
    {
    matrix[0][0] = (eye->z-focus->z)/distance;
    matrix[0][2] = -1.0*(focus->x - eye->x)/distance;
    }
  else
    {
    if (eye->y < focus->y)
      {
      matrix[0][0] = -1.0;
      }
    else
      {
      matrix[0][0] = 1.0;
      }
    matrix[0][2] = 0.0;
    }
  matrix[0][1] = matrix[0][3] = 0.0;
  matrix[1][1] = 1.0;
  matrix[1][0] = matrix[1][2] = matrix[1][3] = 0.0;
  matrix[2][0] = -1.0*matrix[0][2];
  matrix[2][2] = matrix[0][0];
  matrix[2][3] = 0.0;
  matrix[2][1] = 0.0;
  matrix[3][3] = 1.0;
  matrix[3][0] = matrix[3][1] = matrix[3][2] = 0.0;
  perspective_trans = xgl_object_create(xglr_sys_state,XGL_TRANS, NULL, NULL);

  xgl_transform_write (perspective_trans, matrix);
  xgl_transform_multiply (*trans, *trans, perspective_trans);

  /* now rotate x */
  distance_old = distance;
  distance = sqrt((eye->x-focus->x)*(eye->x-focus->x) +
		  (eye->y-focus->y)*(eye->y-focus->y) +
		  (eye->z-focus->z)*(eye->z-focus->z));
  matrix[0][0] = 1.0;
  matrix[0][1] = matrix[0][2] = matrix[0][3] = 0.0;
  matrix[1][1] = distance_old/distance;
  matrix[1][2] = (eye->y - focus->y)/distance;
  matrix[1][0] = matrix[1][3] = 0.0;
  matrix[2][1] = -1.0*matrix[1][2];
  matrix[2][2] = matrix[1][1];
  matrix[2][3] = 0.0;
  matrix[2][0] = 0.0;
  matrix[3][3] = 1.0;
  matrix[3][0] = matrix[3][1] = matrix[3][2] = 0.0;
  xgl_transform_write (perspective_trans, matrix);
  xgl_transform_multiply (*trans, *trans, perspective_trans);

  /* now rotate z (twist) */
  matrix[0][0] = cos(-twist);
  matrix[0][1] = sin(-twist);
  matrix[0][2] = matrix[0][3] = 0.0;
  matrix[1][0] = -1.0*matrix[0][1];
  matrix[1][1] = matrix[0][0];
  matrix[1][2] = matrix[1][3] = 0.0;
  matrix[2][1] = 0.0;
  matrix[2][2] = 1.0;
  matrix[2][3] = 0.0;
  matrix[2][0] = 0.0;
  matrix[3][3] = 1.0;
  matrix[3][0] = matrix[3][1] = matrix[3][2] = 0.0;
  xgl_transform_write (perspective_trans, matrix);
  xgl_transform_multiply (*trans, *trans, perspective_trans);
  xgl_transform_read(*trans,matrix);

  /* do the stereo translate */
  pt_f3d.x = -eye_ang;
  pt_f3d.y = 0;
  pt_f3d.z = 0;
  xgl_transform_translate (*trans, &pt, XGL_TRANS_POSTCONCAT);

  /* Set view perspective and concatenate with view orientation */
  matrix[0][0] = 2.0*near/(right-left);
  matrix[0][1] = matrix[0][2] = matrix[0][3] = 0.0;
  matrix[1][1] = 2.0*near/(top-bottom);
  matrix[1][0] = matrix[1][2] = matrix[1][3] = 0.0;
  matrix[2][2] = 0.5*(far+near)/(far-near) + 0.5;
  matrix[2][3] = -1.0;
  matrix[2][0] = (right+left)/(right-left);
  matrix[2][1] = (top+bottom)/(top-bottom);
  matrix[3][2] = 1.0*far*near/ (far-near);
  matrix[3][0] = matrix[3][1] = 0.0;
  matrix[3][3] = 0;
  xgl_transform_write (perspective_trans, matrix);
  xgl_transform_multiply (*trans, *trans, perspective_trans);
  xgl_object_destroy (perspective_trans);
}

// Description:
// Implement base class method.
void vlXglrCamera::Render(vlCamera *cam, vlRenderer *ren)
{
  this->Render(cam, (vlXglrRenderer *)ren);
}

// Description:
// Actual camera render method.
void vlXglrCamera::Render(vlCamera *cam, vlXglrRenderer *ren)
{
  Xgl_ctx *context;
  Xgl_win_ras *win_ras = NULL; // XGLR Window Raster object 
  int stereo;
  int *size;
  Xgl_color_rgb bg_color;
  float *background;
  float aspect[3];
  Xgl_bounds_d3d vdc_bounds;
  Xgl_trans view_trans;
  Xgl_trans trans;
  Xgl_pt_f3d  eye,focus;
  float twist;
  float matrix[4][4];
  vlXglrRenderWindow *rw;
  float *Position, *FocalPoint, *ClippingRange;

  context = ren->GetContext();
  win_ras = ren->GetRaster();

  // get size info
  rw = (vlXglrRenderWindow*)(ren->GetRenderWindow());
  size = rw->GetSize();

  // find out if we should stereo render
  stereo = rw->GetStereoRender();
  if (stereo)
    {
    switch ((ren->GetRenderWindow())->GetStereoType())
      {
      case VL_STEREO_CRYSTAL_EYES:
	if (cam->GetLeftEye())
	  {
	  xgl_object_set(*win_ras,
			 XGL_WIN_RAS_STEREO_MODE,XGL_STEREO_LEFT,NULL);
	  }
	else
	  {
	  xgl_object_set(*win_ras,
			 XGL_WIN_RAS_STEREO_MODE,XGL_STEREO_RIGHT,NULL);
	  }
	break;
      default:
	xgl_object_set(*win_ras,
		       XGL_WIN_RAS_STEREO_MODE, XGL_STEREO_NONE,NULL);
      }
    }
  else
    {
    xgl_object_set(*win_ras,
		   XGL_WIN_RAS_STEREO_MODE, XGL_STEREO_NONE,NULL);
    }


  ClippingRange = cam->GetClippingRange();
  Position = cam->GetPosition();
  FocalPoint = cam->GetFocalPoint();

  // get the background color
  background = ren->GetBackground();
  bg_color.r = background[0];
  bg_color.g = background[1];
  bg_color.b = background[2];
  if (cam->GetLeftEye() || (!stereo) || 
      ((ren->GetRenderWindow())->GetStereoType() != VL_STEREO_CRYSTAL_EYES))
    {
    xgl_object_set(*context,XGL_CTX_BACKGROUND_COLOR,
		   &bg_color,0);
    xgl_context_new_frame(*context);
    }

  aspect[0] = size[0]/size[1];
  aspect[1] = 1.0;
  
  vdc_bounds.xmin = -1;
  vdc_bounds.xmax = 1;
  vdc_bounds.ymin = -1.0*aspect[1];
  vdc_bounds.ymax = 1.0*aspect[1];

  vdc_bounds.zmin = -1.0;
  vdc_bounds.zmax = 0;

  ren->SetAspect(aspect);

  trans = xgl_object_create(xglr_sys_state,XGL_TRANS,NULL,NULL);
  eye.x = Position[0];
  eye.y = Position[1];
  eye.z = Position[2];
  focus.x = FocalPoint[0];
  focus.y = FocalPoint[1];
  focus.z = FocalPoint[2];

  /* xgl_object_set(*context,XGL_CTX_VDC_WINDOW, &vdc_bounds, NULL); */
  xgl_object_set(*context,XGL_CTX_VDC_WINDOW, &vdc_bounds, NULL);
  xgl_object_set(*context,XGL_CTX_VIEW_CLIP_BOUNDS, &vdc_bounds, NULL);

  twist = cam->GetTwist();

  if (stereo)
    {
    if (cam->GetLeftEye())
      {
      view_calc(&eye,&focus,ClippingRange[0],
		ClippingRange[1],
		cam->GetViewAngle(),twist,&trans,cam->GetDistance(),
		-1.0*cam->GetEyeAngle(),aspect[0]/aspect[1]);
      }
    else
      {
      view_calc(&eye,&focus,ClippingRange[0],
		ClippingRange[1],
		cam->GetViewAngle(),twist,&trans,cam->GetDistance(),
		cam->GetEyeAngle(),aspect[0]/aspect[1]);
      }
    }
  else
    {
      view_calc(&eye,&focus,ClippingRange[0],
		ClippingRange[1],
		cam->GetViewAngle(),twist,&trans,cam->GetDistance(),
		0.0,aspect[0]/aspect[1]);
    }

  xgl_object_get(*context,XGL_CTX_VIEW_TRANS, &view_trans);
  xgl_transform_copy(view_trans,trans);
  xgl_transform_read(trans,matrix);
  xgl_object_destroy(trans);

  // if we have a stereo renderer, draw other eye next time 
  if (stereo)
    {
    if (cam->GetLeftEye()) cam->SetLeftEye(0);
    else cam->SetLeftEye(1);
    }
}
