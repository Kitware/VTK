/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OpenGLTextMapper.cxx
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
#include "vtkWin32OpenGLTextMapper.h"
#include <GL/gl.h>

vtkWin32OpenGLTextMapper::vtkWin32OpenGLTextMapper()
{
  static listBase = 1000;
  this->ListBase = listBase;
  listBase += 1000;
}

void vtkWin32OpenGLTextMapper::RenderOpaqueGeometry(vtkViewport* viewport, 
						    vtkActor2D* actor)
{
  vtkDebugMacro (<< "vtkWin32OpenGLTextMapper::RenderOpaqueGeometry");

  // Check for input
  if (this->Input == NULL) 
    {
    return;
    }

  // Get the position of the text actor
  POINT ptDestOff;
  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedDisplayValue(viewport);
  ptDestOff.x = actorPos[0];
  ptDestOff.y = actorPos[1];

  // Set up the font color from the text actor
  unsigned char red = 0;
  unsigned char green = 0;
  unsigned char blue = 0;
  float*  actorColor = actor->GetProperty()->GetColor();
  red = (unsigned char) (actorColor[0] * 255.0);
  green = (unsigned char) (actorColor[1] * 255.0);
  blue = (unsigned char) (actorColor[2] * 255.0);

  // Set up the shadow color
  float intensity;
  intensity = (red + green + blue)/3.0;

  unsigned char shadowRed, shadowGreen, shadowBlue;
  if (intensity > 128)
    {
    shadowRed = shadowBlue = shadowGreen = 0;
    }
  else
    {
    shadowRed = shadowBlue = shadowGreen = 255;
    }

  // Define bounding rectangle
  RECT rect;
  rect.left = ptDestOff.x;
  rect.top = ptDestOff.y;
  rect.bottom = ptDestOff.y;
  rect.right = ptDestOff.x;

  int size[2];
  this->GetSize(viewport, size);
  rect.right = rect.left + size[0];
  rect.top = rect.bottom + size[1];
  
  int winJust;
  switch (this->Justification)
    {
    int tmp;
    case 0: winJust = DT_LEFT; break;
    case 1:
      winJust = DT_CENTER;
      tmp = rect.right - rect.left + 1;
      rect.left = rect.left - tmp/2;
      rect.right = rect.left + tmp;
      break;
    case 2: 
      winJust = DT_RIGHT;
      tmp = rect.right - rect.left + 1;
      rect.left = rect.right;
      rect.right = rect.right - tmp;
    }
  
  // push a 2D matrix on the stack
  int *vsize = viewport->GetSize();
  glMatrixMode( GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0,vsize[0] -1, 0, vsize[1] -1, 0, 1);
  glDisable( GL_DEPTH_TEST );
  glDisable( GL_LIGHTING);

  // Check to see whether we have to rebuild anything
  if ( this->FontMTime > this->OpenGLBuildTime)
    {
    // Get the window information for display
    vtkWindow*  window = viewport->GetVTKWindow();
    // Get the device context from the window
    HDC hdc = (HDC) window->GetGenericContext();
    wglUseFontBitmaps(hdc, 0, 255, this->ListBase); 
    this->OpenGLBuildTime.Modified();
    }
  
  glListBase (this->ListBase); 
  // Set the colors for the shadow
  if (this->Shadow)
    {
    rect.left++; rect.bottom--;
    // set the colors for the foreground
    glColor3ub(shadowRed, shadowGreen, shadowBlue);
    glRasterPos2i(rect.left,rect.bottom);

    // Draw the shadow text
    glCallLists (strlen(this->Input), GL_UNSIGNED_BYTE, this->Input);  
    rect.left--;  rect.bottom++; 
    }
  
  // set the colors for the foreground
  glColor3ub(red, green, blue);
  glRasterPos2i(rect.left,rect.bottom);

  // display a string: // indicate start of glyph display lists 
  glCallLists (strlen(this->Input), GL_UNSIGNED_BYTE, this->Input);  

  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_DEPTH_TEST );
  glEnable( GL_LIGHTING);
}

