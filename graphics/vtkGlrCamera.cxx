/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlrCamera.cxx
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
#include "vtkGlrRenderWindow.h"
#include "vtkGlrRenderer.h"
#include "vtkGlrCamera.h"

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
  int stereo;
  vtkMatrix4x4 matrix;

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
	aspect[0] = (float)(right-left+1)/(float)(2.0*(top-bottom+1));
	aspect[1] = 1.0;
	break;
      default:
	aspect[0] = (float)(right-left+1)/(float)(top-bottom+1);
	aspect[1] = 1.0;
      }
    }
  else
    {
    aspect[0] = (float)(right-left+1)/(float)(top-bottom+1);
    aspect[1] = 1.0;
    }
  
  ren->SetAspect(aspect);

  mmode(MPROJECTION);
  matrix = cam->GetPerspectiveTransform(aspect[0]/aspect[1],-1,1);
  matrix.Transpose();
  // insert camera view transformation 
  loadmatrix((const float (*)[4])(matrix[0]));

  // since lookat modifies the model view matrix do a push 
  // first and set the mmode.  This will be undone in the  
  // render action after the actors! message sis sent      
  mmode(MVIEWING);
  pushmatrix();
  matrix = cam->GetViewTransform();
  matrix.Transpose();
  
  // insert camera view transformation 
  multmatrix((const float (*)[4])(matrix[0]));

  // get the background color
  bg_color = ren->GetBackground();

  // Set the background and clear the zbuff
  clr = 0xff000000;
  clr |= ((int)(255*bg_color[2])) << 16;
  clr |= ((int)(255*bg_color[1])) << 8;
  clr |= (int)(255*bg_color[0]);

  if ((ren->GetRenderWindow())->GetErase()) 
    {
    czclear(clr, getgdesc(GD_ZMAX));
    vtkDebugMacro(<< "czclear: " << clr << "\n");
    }
}
