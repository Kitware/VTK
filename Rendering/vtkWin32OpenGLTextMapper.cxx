/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OpenGLTextMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkWin32OpenGLTextMapper.h"
#include <GL/gl.h>
#include "vtkObjectFactory.h"
#include "vtkgluPickMatrix.h"


//------------------------------------------------------------------------------
vtkWin32OpenGLTextMapper* vtkWin32OpenGLTextMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWin32OpenGLTextMapper");
  if(ret)
    {
    return (vtkWin32OpenGLTextMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWin32OpenGLTextMapper;
}




struct vtkFontStruct
{
  vtkWindow *Window;
  int   Italic;
  int   Bold;
  int   FontSize;
  int   FontFamily;
  int   ListBase;
};
  
static vtkFontStruct *cache[30] = {
  NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL};
static int numCached = 0;

int vtkWin32OpenGLTextMapper::GetListBaseForFont(vtkTextMapper *tm, 
                                                 vtkViewport *vp)
{
  int i, j;
  vtkWindow *win = vp->GetVTKWindow();

  // has the font been cached ?
  for (i = 0; i < numCached; i++)
    {
    if (cache[i]->Window == win &&
        cache[i]->Italic == tm->GetItalic() &&
        cache[i]->Bold == tm->GetBold() &&
        cache[i]->FontSize == tm->GetFontSize() &&
        cache[i]->FontFamily == tm->GetFontFamily())
      {
      // make this the most recently used
      if (i != 0)
        {
        vtkFontStruct *tmp = cache[i];
        for (j = i-1; j >= 0; j--)
          {
          cache[j+1] = cache[j];
          }
        cache[0] = tmp;
        }
      return cache[0]->ListBase;
      }
    }
  
  HDC hdc = (HDC) win->GetGenericContext();

  // OK the font is not cached
  // so we need to make room for a new font
  if (numCached == 30)
    {
    wglMakeCurrent((HDC)cache[29]->Window->GetGenericContext(), 
                   (HGLRC)cache[29]->Window->GetGenericDisplayId());
    glDeleteLists(cache[29]->ListBase,255);
    wglMakeCurrent(hdc, (HGLRC)win->GetGenericDisplayId());
    numCached = 29;
    }

  // add the new font
  if (!cache[numCached])
    {
    cache[numCached] = new vtkFontStruct;
    int done = 0;
    cache[numCached]->ListBase = 1000;
    do 
      {
      done = 1;
      cache[numCached]->ListBase += 260;
      for (i = 0; i < numCached; i++)
        {
        if (cache[i]->ListBase == cache[numCached]->ListBase)
          {
          done = 0;
          }
        }
      }
    while (!done);
    }
  
  // set the other info and build the font
  cache[numCached]->Window = win;
  cache[numCached]->Italic = tm->GetItalic();
  cache[numCached]->Bold = tm->GetBold();
  cache[numCached]->FontSize = tm->GetFontSize();
  cache[numCached]->FontFamily = tm->GetFontFamily();
  wglUseFontBitmaps(hdc, 0, 255, cache[numCached]->ListBase); 
  
  // now resort the list
  vtkFontStruct *tmp = cache[numCached];
  for (i = numCached-1; i >= 0; i--)
    {
    cache[i+1] = cache[i];
    }
  cache[0] = tmp;
  numCached++;
  return cache[0]->ListBase;
}

void vtkWin32OpenGLTextMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  int i,j;
  
  // free up any cached font associated with this window
  // has the font been cached ?
  for (i = 0; i < numCached; i++)
    {
    if (cache[i]->Window == win)
      {
      win->MakeCurrent();
      glDeleteLists(cache[i]->ListBase,255);
      delete cache[i];
      // resort them
      numCached--;
      for (j = i; j < numCached; j++)
        {
        cache[j] = cache[j+1];
        }
      cache[numCached] = NULL;
      i--;
      }
    }

  if ( this->Font )
    {
    DeleteObject( this->Font );
    this->Font = 0;
    }
  
  this->LastWindow = NULL;
  
  // very important
  // the release of graphics resources indicates that significant changes have
  // occurred. Old fonts, cached sizes etc are all no longer valid, so we send
  // ourselves a general modified message.
  this->Modified();
}

vtkWin32OpenGLTextMapper::vtkWin32OpenGLTextMapper()
{
}

vtkWin32OpenGLTextMapper::~vtkWin32OpenGLTextMapper()
{
  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    }  
}

void vtkWin32OpenGLTextMapper::RenderOpaqueGeometry(vtkViewport* viewport, 
                                                    vtkActor2D* actor)
{
  float*  actorColor = actor->GetProperty()->GetColor();
  if ( actorColor[3] == 1.0 )
    {
    this->RenderGeometry( viewport, actor );
    }
}
void vtkWin32OpenGLTextMapper::RenderTranslucentGeometry(vtkViewport* viewport, 
                                                         vtkActor2D* actor)
{
  float*  actorColor = actor->GetProperty()->GetColor();
  if ( actorColor[3] != 1.0 )
    {
    this->RenderGeometry( viewport, actor );
    }
}

void vtkWin32OpenGLTextMapper::RenderGeometry(vtkViewport* viewport, 
                                              vtkActor2D* actor)
{
vtkDebugMacro (<< "RenderOpaqueGeometry");

  // turn off texturing in case it is on
  glDisable( GL_TEXTURE_2D );
  
  // Get the window information for display
  vtkWindow*  window = viewport->GetVTKWindow();
  if (this->LastWindow && this->LastWindow != window)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    }
  this->LastWindow = window;
  
  // Check for input
  if ( this->NumberOfLines > 1 )
    {
    this->RenderOpaqueGeometryMultipleLines(viewport, actor);
    return;
    }

  if ( this->Input == NULL ) 
    {
    vtkErrorMacro (<<"Render - No input");
    return;
    }

  int size[2];
  this->GetSize(viewport, size);

  // Get the device context from the window
  HDC hdc = (HDC) window->GetGenericContext();
 
  // Select the font
  HFONT hOldFont = (HFONT) SelectObject(hdc, this->Font);
  

  // Get the position of the text actor
  POINT ptDestOff;
  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);
  ptDestOff.x = actorPos[0];
  ptDestOff.y = static_cast<long>(actorPos[1] - this->LineOffset);

  // Set up the font color from the text actor
  unsigned char red = 0;
  unsigned char green = 0;
  unsigned char blue = 0;
  unsigned char alpha = 0;
  
  float*  actorColor = actor->GetProperty()->GetColor();
  red = (unsigned char) (actorColor[0] * 255.0);
  green = (unsigned char) (actorColor[1] * 255.0);
  blue = (unsigned char) (actorColor[2] * 255.0);
  alpha = (unsigned char) (actorColor[3] * 255.0);
  
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

  rect.right = rect.left + size[0];
  rect.top = rect.bottom + size[1];
  
  int winJust;
  switch (this->Justification)
    {
    int tmp;
    case VTK_TEXT_LEFT: winJust = DT_LEFT; break;
    case VTK_TEXT_CENTERED:
      winJust = DT_CENTER;
      tmp = rect.right - rect.left + 1;
      rect.left = rect.left - tmp/2;
      rect.right = rect.left + tmp;
      break;
    case VTK_TEXT_RIGHT: 
      winJust = DT_RIGHT;
      tmp = rect.right - rect.left + 1;
      rect.right = rect.left;
      rect.left = rect.left - tmp;
    }
  switch (this->VerticalJustification)
    {
    case VTK_TEXT_TOP: 
      rect.top = rect.bottom;
      rect.bottom = rect.bottom - size[1];
      break;
    case VTK_TEXT_CENTERED:
      rect.bottom = rect.bottom - size[1]/2;
      rect.top = rect.bottom + size[1];
      break;
    case VTK_TEXT_BOTTOM: 
      break;
    }
  
  // push a 2D matrix on the stack
  int *vsize = viewport->GetSize();
  glMatrixMode( GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  if(viewport->GetIsPicking())
    {
    vtkgluPickMatrix(viewport->GetPickX(), viewport->GetPickY(),
		     1, 1, viewport->GetOrigin(), viewport->GetSize());
    }
  
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glDisable( GL_LIGHTING);

  int front = 
    (actor->GetProperty()->GetDisplayLocation() == VTK_FOREGROUND_LOCATION);

  // When picking draw the bounds of the text as a rectangle,
  // as text only picks when the pick point is exactly on the
  // origin of the text 
  if(viewport->GetIsPicking())
    {
    float width = 2.0 * ((float)rect.right - rect.left) / vsize[0];
    float height = 2.0 * ((float)rect.top - rect.bottom) / vsize[1];
    float x1 = (2.0 * (GLfloat)(rect.left) / vsize[0] - 1);
    float y1 = (2.0 * (GLfloat)(rect.bottom) / vsize[1] - 1);
    glRectf(x1, y1, x1+width, y1+height);

    // Clean up and return after drawing the rectangle
    glMatrixMode( GL_PROJECTION);
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW);
    glPopMatrix();
    glEnable( GL_LIGHTING);
    
    // Restore the state
    SelectObject(hdc, hOldFont);
    return;
    }
  
  glListBase(vtkWin32OpenGLTextMapper::GetListBaseForFont(this,viewport));

  // Set the colors for the shadow
  if (this->Shadow)
    {
    rect.left++; rect.bottom--;
    // set the colors for the foreground
    glColor4ub(shadowRed, shadowGreen, shadowBlue, alpha);
    glRasterPos3f((2.0 * (GLfloat)(rect.left) / vsize[0] - 1), 
		  (2.0 * (GLfloat)(rect.bottom) / vsize[1] - 1), 
		  (front)?(-1):(.9999));

    // Draw the shadow text
    glCallLists (strlen(this->Input), GL_UNSIGNED_BYTE, this->Input);  
    rect.left--;  rect.bottom++; 
    }
  
  // set the colors for the foreground
  glColor4ub(red, green, blue, alpha);
  glRasterPos3f((2.0 * (GLfloat)(rect.left) / vsize[0] - 1), 
		(2.0 * (GLfloat)(rect.bottom) / vsize[1] - 1), 
		(front)?(-1):(.9999));

  // display a string: // indicate start of glyph display lists 
  glCallLists (strlen(this->Input), GL_UNSIGNED_BYTE, this->Input);  

  glFlush();
  GdiFlush();
  
  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_LIGHTING);

  // Restore the state
  SelectObject(hdc, hOldFont);
}

