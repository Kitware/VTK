/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SbrCam.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "SbrCam.hh"
#include "SbrRenW.hh"
#include "SbrRen.hh"

void rotate(int fd, float Cosine,float Sine, char axis)
{
  static float tform[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};
  int i,j;

  for (i = 0; i < 3; i++)
    {
    for (j = 0; j < 3; j++)
      {
       if (j == i) tform[i][j] = 1.0;
       else tform[i][j] = 0.0;
      }
    }

  switch (axis)
    {
  case 'x' :
  case 'X' : 
    tform[1][1] = Cosine;
    tform[2][2] = tform[1][1];
    tform[1][2] = Sine;
    tform[2][1] = -tform[1][2];
    break;
  case 'y' :
  case 'Y' : 
    tform[0][0] = Cosine;
    tform[2][2] = tform[0][0];
    tform[2][0] = Sine;
    tform[0][2] = -tform[2][0];
    break;
  case 'z' :
  case 'Z' : 
    tform[0][0] = Cosine;
    tform[1][1] = tform[0][0];
    tform[0][1] = Sine;
    tform[1][0] = -tform[0][1];
    break;
    }
  
  view_matrix3d(fd, tform, PRE_CONCAT_VW);
}

void translate(int fd, float x, float y, float z)
{
  static float tform[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};

  tform[3][0] = x;
  tform[3][1] = y;
  tform[3][2] = z;

  view_matrix3d(fd, tform, PRE_CONCAT_VW);
}

void kens_view_volume(int fd, float left, float right, float bottom, 
		      float top, float nearz, float farz)
{
  static float tform[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};

  tform[0][0] = 2.0*nearz/(right-left);
  tform[1][1] = 2.0*nearz/(top-bottom);
  tform[2][0] = (right+left)/(right-left);
  tform[2][1] = (top+bottom)/(top-bottom);
  tform[2][2] = -0.5*(farz+nearz)/(farz-nearz) - 0.5;
  tform[2][3] = -1.0;
  tform[3][2] = -1.0*farz*nearz/(farz-nearz);
  tform[3][3] = 0.0;

  view_matrix3d(fd, tform, REPLACE_VW);
}

void lookat(int fd, float vx,float vy,float vz,float px,float py,float pz,
	    float twist)
{
  float mag;
  
  rotate(fd,cos(-1.0*twist),sin(-1.0*twist),'z');
  
  mag = sqrt((pz-vz)*(pz-vz) + (py-vy)*(py-vy) + (px-vx)*(px-vx));
  if (mag != 0)
    rotate(fd,sqrt((px-vx)*(px-vx) + (pz-vz)*(pz-vz))/mag,(vy-py)/mag,'x');
  
  mag = sqrt((pz-vz)*(pz-vz) + (px-vx)*(px-vx));
  if (mag != 0)
    {
    rotate(fd, (vz-pz)/mag, (px-vx)/mag, 'y');
    }
  else
    {
    if (py < vy)
      {
      rotate(fd, 1, 0, 'y');
      }
    else
      {
      rotate(fd, -1, 0, 'y');
      }
    }

  translate(fd,-vx,-vy,-vz);

  viewpoint(fd,POSITIONAL,vx,vy,vz);
}

/* bonus stereo perspective function - from manual */
void stereopersp(int fd, float fovy, float aspect, float near, 
		 float far, float conv, float eye)
{
  float left, right, top, bottom;
  float gltan;

  eye = tan(eye*3.1415926/180.0)*conv;
  gltan = tan(fovy*M_PI/360.0);
  top = gltan*near;
  bottom = -top;

  gltan = tan(fovy*aspect*M_PI/360.0);
  left = bottom*aspect - eye/conv*near;
  right = top*aspect - eye/conv*near;

  kens_view_volume(fd,left,right,bottom,top,near,far);

  /* now translate */
  translate(fd, -eye, 0, 0);
}




// Description:
// Implement base class method.
void vtkSbrCamera::Render(vtkCamera *cam, vtkRenderer *ren)
{
  this->Render(cam, (vtkSbrRenderer *)ren);
}

// Description:
// Actual camera render method.
void vtkSbrCamera::Render(vtkCamera *cam, vtkSbrRenderer *ren)
{
  float aspect[3];
  float viewport[4];
  float *background;
  float twist;
  int stereo;
  int fd;
  int *size;
  int *screen_size;
  vtkSbrRenderWindow *rw;
  float view_size[2];
  float vdc_vals[6];
  float *Position, *FocalPoint, *ClippingRange;
  fd = ren->GetFd();

  // get the background color
  background = ren->GetBackground();
  // get size info
  rw = (vtkSbrRenderWindow*)(ren->GetRenderWindow());
  size = rw->GetSize();
  screen_size = rw->GetScreenSize();
  // find out if we should stereo render
  stereo = rw->GetStereoRender();


  // set this renderer's viewport, must turn off z-buffering when changing
  // viewport
  hidden_surface(fd, FALSE, FALSE);
  vtkDebugMacro(<< " SB_hidden_surface: False False\n");

  memcpy(viewport,ren->GetViewport(),sizeof(float)*4);

  // if were on a stereo renderer draw to special parts of screen 
  if (stereo)
    {
    switch ((ren->GetRenderWindow())->GetStereoType())
      {
      case VTK_STEREO_CRYSTAL_EYES:
	if (cam->GetLeftEye()) 
	  {
	  viewport[1] = 0.5 + viewport[1]*0.5;
	  viewport[3] = 0.5 + viewport[3]*0.5;
	  }
	else
	  {
	  viewport[1] = viewport[1]*0.5;
	  viewport[3] = viewport[3]*0.5;
	  }
	break;
      }
    }

  view_size[0] = (viewport[2] - viewport[0])*size[0];
  view_size[1] = (viewport[3] - viewport[1])*size[1];
  vdc_vals[0] = -1.0 - viewport[0]*size[0]*2.0/view_size[0];
  vdc_vals[3] = vdc_vals[0] + 2.0*screen_size[0]/view_size[0];
  vdc_vals[4] = 1.0 + (1.0-viewport[3])*size[1]*2.0/view_size[1];
  vdc_vals[1] = vdc_vals[4] - 2.0*screen_size[1]/view_size[1];
  vdc_vals[2] = 0;
  vdc_vals[5] = 1.0;


  // make sure the aspect is up to date
  if (stereo)
    {
    switch ((ren->GetRenderWindow())->GetStereoType())
      {
      case VTK_STEREO_CRYSTAL_EYES:
	{
	aspect[0] = view_size[0]/(2.0*view_size[1]);
	aspect[1] = 1.0;
	}
	break;
      default:
	{
	aspect[0] = view_size[0]/view_size[1];
	aspect[1] = 1.0;
	}
      }
    }
  else
    {
    aspect[0] = view_size[0]/view_size[1];
    aspect[1] = 1.0;
    }
  ren->SetAspect(aspect);

  vdc_extent(fd,vdc_vals[0],vdc_vals[1],vdc_vals[2],
	     vdc_vals[3],vdc_vals[4],vdc_vals[5]);

  vtkDebugMacro(<< " screen_size " << screen_size[0] << " " 
  << screen_size[1] << endl);
  vtkDebugMacro(<< " size " << size[0] << " " << size[1] << endl);
  vtkDebugMacro(<< " viewport " << viewport[0] << " " << viewport[1] 
  << " " << viewport[2] << " " << viewport[3] << endl);

  // set viewport to clear entire window 
  view_port(fd,-1.0,-1.0,1.0,1.0); 
  hidden_surface(fd, TRUE, FALSE);
  vtkDebugMacro(<< " SB_hidden_surface: True False\n");

  // Set the background color and clear the display.
  // Since clear control was set to clear z buffer, this is done here
  // also.
  background_color(fd, background[0], background[1], background[2]);
  
  // clear the view surface so the new background color takes effect
  if (rw->GetErase()) 
    {
    clear_view_surface(fd);
    vtkDebugMacro(<< " SB_clear_view_surface\n");
    }

  hidden_surface(fd, FALSE, FALSE);
  vtkDebugMacro(<< " SB_hidden_surface: False False\n");

  // I think the z clipping is done before the divide by w 
  vdc_extent(fd,vdc_vals[0],vdc_vals[1],vdc_vals[2],
	     vdc_vals[3],vdc_vals[4],vdc_vals[5]);
  
  view_port(fd,-1.0,-1.0,1.0,1.0); 

  hidden_surface(fd, TRUE, FALSE);
  vtkDebugMacro(<< " SB_hidden_surface: True False\n");

  twist = cam->GetTwist();

  ClippingRange = cam->GetClippingRange();
  Position = cam->GetPosition();
  FocalPoint = cam->GetFocalPoint();
  if (stereo)
    {
    if (cam->GetLeftEye())
      {
      stereopersp(fd,cam->GetViewAngle(), aspect[0] / aspect[1],
		  ClippingRange[0],ClippingRange[1],
		  cam->GetDistance(),-1.0*cam->GetEyeAngle());
      }
    else
      {
      stereopersp(fd,cam->GetViewAngle(), aspect[0] / aspect[1],
		  ClippingRange[0],ClippingRange[1],
		  cam->GetDistance(),1.0*cam->GetEyeAngle());
      }

    lookat(fd,Position[0], Position[1], Position[2], 
	   FocalPoint[0], FocalPoint[1], FocalPoint[2],twist);
    }
  else
    {
    stereopersp(fd,cam->GetViewAngle(), aspect[0] / aspect[1],
		ClippingRange[0],ClippingRange[1],
		cam->GetDistance(),0.0);

    lookat(fd,Position[0], Position[1], Position[2], 
	   FocalPoint[0], FocalPoint[1], FocalPoint[2],twist);
    }

  clip_depth(fd,0.0,ClippingRange[1]);

  // if we have a stereo renderer, draw other eye next time 
  if (stereo)
    {
    if (cam->GetLeftEye()) cam->SetLeftEye(0);
    else cam->SetLeftEye(1);
    }
}
