/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImager.cxx
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

#include "vtkOpenGLImager.h"
#include "vtkImageWindow.h"
#include <GL/gl.h>

int vtkOpenGLImager::RenderOpaqueGeometry()
{
  float *vport;
  int left,right,bottom,top;
  int  *size;

  // get the bounds of the window 
  size = (this->GetVTKWindow())->GetSize();
  
  // find out if we should stereo render
  vport = this->GetViewport();

  left = (int)(vport[0]*(size[0] -1));
  right = (int)(vport[2]*(size[0] - 1));

  // we will set this for all modes on the sparc
  bottom = (int)(vport[1]*(size[1] -1));
  top = (int)(vport[3]*(size[1] - 1));
  glViewport(left,bottom,(right-left+1),(top-bottom+1));
  glEnable( GL_SCISSOR_TEST );
  glScissor( left, bottom,(right-left+1),(top-bottom+1));   
  return vtkImager::RenderOpaqueGeometry();
}


