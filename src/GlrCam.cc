/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GlrCam.cc
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
#include "GlrRenW.hh"
#include "GlrRen.hh"
#include "GlrCam.hh"

// bonus stereo perspective function - from manual 
void stereopersp(int fovy, float aspect, float near, float far, 
		 float conv, float eye)
{
  float left, right, top, bottom;
  float gltan;
  
  eye = tan(eye*3.1415926/180.0)*conv;
  gltan = tan(fovy/2.0/10.0*M_PI/180.0);
  top = gltan*near;
  bottom = -top;
  
  gltan = tan(fovy*aspect/2.0/10.0*M_PI/180.0);
  left = -gltan*near - eye/conv*near;
  right = gltan*near - eye/conv*near;

  window(left,right,bottom,top,near,far);
  translate(-eye,0.0,0.0);
}

// Description:
// Implement base class method.
void vtkGlrCamera::Render(vtkCamera *cam, vtkRenderer *ren)
{
  this->Render(cam, (vtkGlrRenderer *)ren);
}

// Description:
// Actual camera render method.
void vtkGlrCamera::Render(vtkCamera *cam, vtkGlrRenderer *ren)
{
  float aspect[3];
  float *vport;
  float *bg_color;
  long	width,height;
  long clr;
  int left,right,bottom,top;
  float twist;
  int stereo;
  float *Position, *FocalPoint, *ClippingRange;

  // get the bounds of the window 
  getsize(&width,&height);
  
  // find out if we should stereo render
  stereo = ((vtkGlrRenderWindow*)(ren->GetRenderWindow()))->GetStereoRender();

  // must use width -1 and height -1 because width*1.0 = width,  
  // but the maximum pixel value allowed is width -1            
  width--; height--;
  
  vport = ren->GetViewport();

  left = (int)(vport[0]*width);
  right = (int)(vport[2]*width);

  // if were on a stereo renderer draw to special parts of screen
  if (stereo)
    {
    switch ((ren->GetRenderWindow())->GetStereoType())
      {
      case VTK_STEREO_CRYSTAL_EYES:
	if (cam->GetLeftEye()) 
	  {
	  bottom = (int)(532 + (1023-532)*vport[1]);
	  top = (int)(532 + (1023-532)*vport[3]);
	  }
	else
	  {
	  bottom = (int)(491*vport[1]);
	  top = (int)(491*vport[3]);
	  }
	break;
      default:
	bottom = (int)(vport[1]*height);
	top = (int)(vport[3]*height);
      }
    }
  else
    {
    bottom = (int)(vport[1]*height);
    top = (int)(vport[3]*height);
    }
  
  viewport(left,right,bottom,top);
    
  /* for stereo we have to fiddle with aspect */
  if (stereo)
    {
    switch ((ren->GetRenderWindow())->GetStereoType())
      {
      case VTK_STEREO_CRYSTAL_EYES:
	aspect[0] = 1.0;
	aspect[1] = 2.0*(float)(top-bottom+1)/(float)(right-left+1);
	break;
      default:
	aspect[0] = 1.0;
	aspect[1] = (float)(top-bottom+1)/(float)(right-left+1);
      }
    }
  else
    {
    aspect[0] = 1.0;
    aspect[1] = (float)(top-bottom+1)/(float)(right-left+1);
    }
  
  ren->SetAspect(aspect);

  mmode(MPROJECTION);

  ClippingRange = cam->GetClippingRange();
  Position = cam->GetPosition();
  FocalPoint = cam->GetFocalPoint();

  // if were on a stereo renderer use correct perspective for eye 
  if (stereo)
    {
    if (cam->GetLeftEye())
      {
      stereopersp((short)(10.0*cam->GetViewAngle()), aspect[0] / aspect[1],
		  ClippingRange[0],ClippingRange[1],
		  cam->GetDistance(),-1.0*cam->GetEyeAngle());
      }
    else
      {
      stereopersp((short)(10.0*cam->GetViewAngle()), aspect[0] / aspect[1],
		  ClippingRange[0],ClippingRange[1],
		  cam->GetDistance(),1.0*cam->GetEyeAngle());
      }
    }
  else
    {
    perspective((short)(10.0*cam->GetViewAngle()), aspect[0] / aspect[1], 
		ClippingRange[0], ClippingRange[1]);
    }

  // get twist from camera object twist 
  twist = cam->GetTwist();
  twist = twist*1800.0/3.1415926;

  // since lookat modifies the model view matrix do a push 
  // first and set the mmode.  This will be undone in the  
  // render action after the actors! message sis sent      
  mmode(MVIEWING);
  pushmatrix();
  lookat(Position[0], Position[1], Position[2],
	 FocalPoint[0], FocalPoint[1], FocalPoint[2], 
	 (short)(twist));
  
  // get the background color
  bg_color = ren->GetBackground();

  // Set the background and clear the zbuff
  clr = 0xff000000;
  clr |= ((int)(255*bg_color[2])) << 16;
  clr |= ((int)(255*bg_color[1])) << 8;
  clr |= (int)(255*bg_color[0]);

  if (ren->GetErase()) 
    {
    czclear(clr, getgdesc(GD_ZMAX));
    vtkDebugMacro(<< "czclear: " << clr << "\n");
    }

  // if we have a stereo renderer, draw other eye next time 
  if (stereo)
    {
    if (cam->GetLeftEye()) cam->SetLeftEye(0);
    else cam->SetLeftEye(1);
    }
}
