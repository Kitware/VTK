/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSbrCamera.cxx
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
#include "vtkSbrCamera.h"
#include "vtkSbrRenderWindow.h"
#include "vtkSbrRenderer.h"


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
  int stereo;
  int fd;
  int *size;
  int *screen_size;
  vtkSbrRenderWindow *rw;
  float view_size[2];
  float vdc_vals[6];
  fd = ren->GetFd();
  vtkMatrix4x4 matrix;
  float *pos;
  
  // get the background color
  background = ren->GetBackground();
  // get size info
  rw = (vtkSbrRenderWindow*)(ren->GetRenderWindow());
  size = rw->GetSize();
  screen_size = rw->GetScreenSize();

  // find out if we should stereo render
  stereo = cam->GetStereo();
  
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

  matrix = cam->GetCompositePerspectiveTransform(aspect[0]/aspect[1],0,1);
  matrix.Transpose();
 
  // insert model transformation 
  view_matrix3d(fd, (float (*)[4])(matrix[0]),REPLACE_VW);
  
  pos = cam->GetPosition();
  viewpoint(fd,POSITIONAL,pos[0],pos[1],pos[2]);
  
  clip_depth(fd,0.0,1.0);
}
