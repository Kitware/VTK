/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXglrCamera.cxx
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
#include "vtkXglrCamera.hh"
#include "vtkXglrRenderWindow.hh"
#include "vtkXglrRenderer.hh"
#include <xgl/xgl.h>

extern Xgl_sys_state xglr_sys_state;

// Description:
// Implement base class method.
void vtkXglrCamera::Render(vtkCamera *cam, vtkRenderer *ren)
{
  this->Render(cam, (vtkXglrRenderer *)ren);
}

// Description:
// Actual camera render method.
void vtkXglrCamera::Render(vtkCamera *cam, vtkXglrRenderer *ren)
{
  Xgl_ctx *context;
  Xgl_win_ras *win_ras = NULL; // XGLR Window Raster object 
  int stereo;
  int *size;
  Xgl_color_rgb bg_color;
  float *background;
  float aspect[3];
  Xgl_bounds_d3d vdc_bounds;
  Xgl_bounds_d3d dc_bounds;
  Xgl_pt_d3d  max_device_values;
  Xgl_trans view_trans;  
  vtkXglrRenderWindow *rw;
  float *vport;
  vtkMatrix4x4 matrix;

  context = ren->GetContext();
  win_ras = ren->GetRaster();

  // get size info
  rw = (vtkXglrRenderWindow*)(ren->GetRenderWindow());
  size = rw->GetSize();
  vport = ren->GetViewport();
  xgl_object_get(*win_ras, XGL_DEV_MAXIMUM_COORDINATES, 
		 &max_device_values);
		 
  // find out if we should stereo render
  stereo = cam->GetStereo();
  if (stereo)
    {
    switch ((ren->GetRenderWindow())->GetStereoType())
      {
      case VTK_STEREO_CRYSTAL_EYES:
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

  // get the background color
  background = ren->GetBackground();
  bg_color.r = background[0];
  bg_color.g = background[1];
  bg_color.b = background[2];

  dc_bounds.xmin = vport[0]*(size[0] - 1);
  dc_bounds.xmax = vport[2]*(size[0] - 1);
  dc_bounds.ymin = (1.0 - vport[3])*(size[1] - 1);
  dc_bounds.ymax = (1.0 - vport[1])*(size[1] - 1);
  dc_bounds.zmin = 0;
  dc_bounds.zmax = max_device_values.z;
  xgl_object_set(*context, XGL_CTX_DC_VIEWPORT, &dc_bounds, NULL);

  if (cam->GetLeftEye() || (!stereo) || 
      ((ren->GetRenderWindow())->GetStereoType() != VTK_STEREO_CRYSTAL_EYES))
    {
    xgl_object_set(*context,XGL_CTX_BACKGROUND_COLOR,
		   &bg_color,0);
    if ((ren->GetRenderWindow())->GetErase())
      {
      xgl_context_new_frame(*context);
      }
    }

  aspect[0] = ((vport[2] - vport[0])*size[0])/((vport[3] - vport[1])*size[1]);
  aspect[1] = 1.0;
  
  vdc_bounds.xmin = -1;
  vdc_bounds.xmax = 1;
  vdc_bounds.ymin = -1.0*aspect[1];
  vdc_bounds.ymax = 1.0*aspect[1];

  vdc_bounds.zmin = -1.0;
  vdc_bounds.zmax = 0;

  ren->SetAspect(aspect);

  xgl_object_set(*context,XGL_CTX_VDC_WINDOW, &vdc_bounds, NULL);
  xgl_object_set(*context,XGL_CTX_VIEW_CLIP_BOUNDS, &vdc_bounds, NULL);

  matrix = cam->GetCompositePerspectiveTransform(aspect[0]/aspect[1],0,-1);
  matrix.Transpose();
 
  // insert model transformation 
  xgl_object_get(*context,XGL_CTX_VIEW_TRANS, &view_trans);
  xgl_transform_write(view_trans,matrix[0]);
}
