/*=========================================================================

  Program:   Visualization Library
  Module:    GlrCam.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "GlrRen.hh"
#include "GlrCam.hh"

// typecast if neccessary 
void vlGlrCamera::Render(vlRenderer *ren)
{
  this->Render((vlGlrRenderer *)ren);
}

// set the appropriate attributes in Glr for this camera
//
void vlGlrCamera::Render(vlGlrRenderer *ren)
{
  float aspect[3];
  float *vport;
  float *bg_color;
  long	width,height;
  long clr;
  int left,right,bottom,top;
  float twist;
  int stereo;

  // get the bounds of the window 
  getsize(&width,&height);
  
  // find out if we should stereo render
  stereo = ren->GetStereoRender();

  // must use width -1 and height -1 because width*1.0 = width,  
  // but the maximum pixel value allowed is width -1            
  width--; height--;
  
  vport = ren->GetViewport();

  left = (int)(vport[0]*width);
  right = (int)(vport[2]*width);
  bottom = (int)(vport[1]*height);
  top = (int)(vport[3]*height);
  
  viewport(left,right,bottom,top);
    
  aspect[0] = 1.0;
  aspect[1] = (float)(top-bottom+1)/(float)(right-left+1);
  
  ren->SetAspect(aspect);

  mmode(MPROJECTION);
  perspective((short)(10.0*this->ViewAngle), aspect[0] / aspect[1], 
	      this->ClippingRange[0], this->ClippingRange[1]);

  // get twist from camera object twist 
  twist = this->GetTwist();
  twist = twist*1800.0/3.1415926;

  // since lookat modifies the model view matrix do a push 
  // first and set the mmode.  This will be undone in the  
  // render action after the actors! message sis sent      
  mmode(MVIEWING);
  pushmatrix();
  lookat(this->Position[0], this->Position[1], this->Position[2],
	 this->FocalPoint[0], this->FocalPoint[1], this->FocalPoint[2], 
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
    vlDebugMacro(<< "czclear: " << clr << "\n");
    }

  // if we have a stereo renderer, draw other eye next time 
  if (stereo)
    {
    if (this->LeftEye) this->LeftEye = 0;
    else this->LeftEye = 1;
    }
}
