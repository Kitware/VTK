/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OglrCam.cc
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
#include "OglrRenW.hh"
#include "OglrRen.hh"
#include "OglrCam.hh"

void oglrlookat(float vx, float vy, float vz, float px, float py, float pz, 
	    float twist )
{
  GLfloat	theta, phi;
  float		temp1, temp2;

  temp1 = (px-vx)*(px-vx) + (pz-vz)*(pz-vz);

  if ( temp1 == 0 ) 
    {
    cerr << "in OglrCamera: zero denominator in lookat!\n";
    }

  theta = (GLfloat)atan2(
    ((double)(( (px-vx) / (float)(sqrt((double)(temp1)))))),
    ((double)(( (vz-pz) / (float)(sqrt((double)(temp1)))))) );

  theta *= 180.0/M_PI;
  
  temp2 = (px-vx)*(px-vx) + (py-vy)*(py-vy) + (pz-vz)*(pz-vz);

  if ( temp2 == 0 ) 
    {
    cerr << "zero denominator in lookat!\n";
    }

  phi = (GLfloat)atan2(
	(double)(( (vy-py) / (float)(sqrt((double)(temp2))))),
	(double)(( sqrt((double)((px-vx)*(px-vx)+(pz-vz)*(pz-vz))) / 
		   (sqrt((double)(temp2))))) );

  phi *= 180.0/M_PI;


  glRotatef( (GLfloat)(-twist), 0.0, 0.0, 1.0 );
  glRotatef(             phi,   1.0, 0.0, 0.0 );
  glRotatef(             theta, 0.0, 1.0, 0.0 );
  glTranslatef( (GLfloat)( -vx ), (GLfloat)( -vy ), (GLfloat)( -vz ) );
}

// bonus stereo perspective function - from manual 
void oglrstereopersp(float fovy, float aspect, float near, float far, 
		     float conv, float eye)
{
  float left, right, top, bottom;
  float gltan;
  
  eye = tan(eye*3.1415926/180.0)*conv;
  gltan = tan(fovy/2.0*M_PI/180.0);
  top = gltan*near;
  bottom = -top;
  
  gltan = tan(fovy*aspect/2.0*M_PI/180.0);
  left = -gltan*near - eye/conv*near;
  right = gltan*near - eye/conv*near;

  glLoadIdentity();
  glFrustum(left,right,bottom,top,near,far);
  glTranslatef(-eye,0.0,0.0);
}

// Description:
// Implement base class method.
void vtkOglrCamera::Render(vtkCamera *cam, vtkRenderer *ren)
{
  this->Render(cam, (vtkOglrRenderer *)ren);
}

// Description:
// Actual camera render method.
void vtkOglrCamera::Render(vtkCamera *cam, vtkOglrRenderer *ren)
{
  float aspect[3];
  float *vport;
  float *bg_color;
  int left,right,bottom,top;
  float twist;
  int stereo;
  float *Position, *FocalPoint, *ClippingRange;
  int  *size;

  // get the bounds of the window 
  size = ((vtkOglrRenderWindow*)(ren->GetRenderWindow()))->GetSize();
  
  // find out if we should stereo render
  stereo = ((vtkOglrRenderWindow*)(ren->GetRenderWindow()))->GetStereoRender();

  vport = ren->GetViewport();

  left = (int)(vport[0]*(size[0] -1));
  right = (int)(vport[2]*(size[0] - 1));

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
	bottom = (int)(vport[1]*(size[1] -1));
	top = (int)(vport[3]*(size[1] - 1));
      }
    }
  else
    {
    bottom = (int)(vport[1]*(size[1] -1));
    top = (int)(vport[3]*(size[1] - 1));
    }
  
  glViewport(left,bottom,(right-left+1),(top-bottom+1));
  glEnable( GL_SCISSOR_TEST );
  glScissor( left, bottom,(right-left+1),(top-bottom+1));   
    
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

  glMatrixMode( GL_PROJECTION);

  ClippingRange = cam->GetClippingRange();
  Position = cam->GetPosition();
  FocalPoint = cam->GetFocalPoint();

  // if were on a stereo renderer use correct perspective for eye 
  if (stereo)
    {
    if (cam->GetLeftEye())
      {
      oglrstereopersp(cam->GetViewAngle(), aspect[0] / aspect[1],
		      ClippingRange[0],ClippingRange[1],
		      cam->GetDistance(),-1.0*cam->GetEyeAngle());
      }
    else
      {
      oglrstereopersp(cam->GetViewAngle(), aspect[0] / aspect[1],
		      ClippingRange[0],ClippingRange[1],
		      cam->GetDistance(),1.0*cam->GetEyeAngle());
      }
    }
  else
    {
    oglrstereopersp(cam->GetViewAngle(), aspect[0] / aspect[1], 
		    ClippingRange[0], ClippingRange[1],
		    cam->GetDistance(),0.0);
    }

  // get twist from camera object twist 
  twist = cam->GetTwist();
  twist = twist*180.0/3.1415926;

  // since lookat modifies the model view matrix do a push 
  // first and set the mmode.  This will be undone in the  
  // render action after the actors! message sis sent      
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  oglrlookat(Position[0], Position[1], Position[2],
	     FocalPoint[0], FocalPoint[1], FocalPoint[2], 
	     twist);
  
  // get the background color
  bg_color = ren->GetBackground();

  if (((vtkOglrRenderWindow*)(ren->GetRenderWindow()))->GetErase()) 
    {
    glClearColor( ((GLclampf)(bg_color[0])),
		  ((GLclampf)(bg_color[1])),
		  ((GLclampf)(bg_color[2])),
		  ((GLclampf)(1.0)) );
    
    glClearDepth( (GLclampd)( 1.0 ) );
    vtkDebugMacro(<< "glClear\n");
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }

  // if we have a stereo renderer, draw other eye next time 
  if (stereo)
    {
    if (cam->GetLeftEye()) cam->SetLeftEye(0);
    else cam->SetLeftEye(1);
    }
}
