/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCamera.cxx
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

#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLCamera.h"
#include <GL/gl.h>


// Description:
// Implement base class method.
void vtkOpenGLCamera::Render(vtkRenderer *ren)
{
  float aspect[2];
  float *vport;
  float *bg_color;
  int left,right,bottom,top;
  int  *size;
  vtkMatrix4x4 matrix;

  // get the bounds of the window 
  size = (ren->GetRenderWindow())->GetSize();
  
  // find out if we should stereo render
  this->Stereo = (ren->GetRenderWindow())->GetStereoRender();
  vport = ren->GetViewport();

  left = (int)(vport[0]*(size[0] -1));
  right = (int)(vport[2]*(size[0] - 1));

  // if were on a stereo renderer draw to special parts of screen
  if (this->Stereo)
    {
    switch ((ren->GetRenderWindow())->GetStereoType())
      {
      case VTK_STEREO_CRYSTAL_EYES:
	if (this->GetLeftEye()) 
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
  if (this->Stereo)
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

  glMatrixMode( GL_PROJECTION);
  matrix = this->GetPerspectiveTransform(aspect[0]/aspect[1],0,1);
  matrix.Transpose();
  // insert camera view transformation 
  glLoadMatrixf(matrix[0]);

  // since lookat modifies the model view matrix do a push 
  // first and set the mmode.  This will be undone in the  
  // render action after the actors! message sis sent      
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  matrix = this->GetViewTransform();
  matrix.Transpose();
  
  // insert camera view transformation 
  glMultMatrixf(matrix[0]);

  // get the background color
  bg_color = ren->GetBackground();

  if ((ren->GetRenderWindow())->GetErase()) 
    {
    glClearColor( ((GLclampf)(bg_color[0])),
		  ((GLclampf)(bg_color[1])),
		  ((GLclampf)(bg_color[2])),
		  ((GLclampf)(1.0)) );
    
    glClearDepth( (GLclampd)( 1.0 ) );
    vtkDebugMacro(<< "glClear\n");
    glClear((GLbitfield)(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    }

  // if we have a stereo renderer, draw other eye next time 
  if (this->Stereo)
    {
    if (this->LeftEye) this->LeftEye = 0;
    else this->LeftEye = 1;
    }
}
