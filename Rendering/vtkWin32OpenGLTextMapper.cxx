/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OpenGLTextMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWin32OpenGLTextMapper.h"

#include <GL/gl.h>

#include "vtkActor2D.h"
#include "vtkgluPickMatrix.h"
#include "vtkObjectFactory.h"
#include "vtkString.h"
#include "vtkProperty2D.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

vtkCxxRevisionMacro(vtkWin32OpenGLTextMapper, "1.46");
vtkStandardNewMacro(vtkWin32OpenGLTextMapper);

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

  vtkTextProperty *tprop = tm->GetTextProperty();
  if (!tprop)
    {
    vtkErrorWithObjectMacro(tm,<< "Need a text property to get list base for font");
    return 0;
    }

  // has the font been cached ?
  for (i = 0; i < numCached; i++)
    {
    if (cache[i]->Window == win &&
        cache[i]->Italic == tprop->GetItalic() &&
        cache[i]->Bold == tprop->GetBold() &&
        cache[i]->FontSize == tprop->GetFontSize() &&
        cache[i]->FontFamily == tprop->GetFontFamily())
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
    if (!wglMakeCurrent((HDC)cache[29]->Window->GetGenericContext(), 
                        (HGLRC)cache[29]->Window->GetGenericDisplayId()))
      {
      vtkErrorWithObjectMacro(tm,<< "wglMakeCurrent failed");
      }
    glDeleteLists(cache[29]->ListBase,255);
    if (!wglMakeCurrent(hdc, (HGLRC)win->GetGenericDisplayId()))
      {
      vtkErrorWithObjectMacro(tm,<< "wglMakeCurrent failed");
      }
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
  cache[numCached]->Italic = tprop->GetItalic();
  cache[numCached]->Bold = tprop->GetBold();
  cache[numCached]->FontSize = tprop->GetFontSize();
  cache[numCached]->FontFamily = tprop->GetFontFamily();
  if (!wglUseFontBitmaps(hdc, 0, 255, cache[numCached]->ListBase))
    {
    vtkErrorWithObjectMacro(tm,<< "wglUseFontBitmaps failed");
    }
  
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

void vtkWin32OpenGLTextMapper::RenderOverlay(vtkViewport* viewport, 
                                             vtkActor2D* actor)
{
  vtkDebugMacro (<< "RenderOverlay");

  // Check for input

  if (this->Input == NULL || this->Input[0] == '\0') 
    {
    return;
    }

  // Check for multi-lines

  if (this->NumberOfLines > 1)
    {
    this->RenderOverlayMultipleLines(viewport, actor);
    return;
    }

  // Get text property

  vtkTextProperty *tprop = this->GetTextProperty();
  if (!tprop)
    {
    vtkErrorMacro(<< "Need a text property to render mapper");
    return;
    }

  // turn off texturing in case it is on
  glDisable( GL_TEXTURE_2D );
  
  // Get the window information for display
  vtkWindow*  window = viewport->GetVTKWindow();
  if (this->LastWindow && this->LastWindow != window)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    }
  this->LastWindow = window;
  
  int size[2];
  this->GetSize(viewport, size);

  // Get the device context from the window
  HDC hdc = (HDC) window->GetGenericContext();
 
  // Select the font
  HFONT hOldFont = (HFONT) SelectObject(hdc, this->Font);
  

  // Get the position of the text actor
  POINT ptDestOff;
  int* actorPos = 
    actor->GetActualPositionCoordinate()->GetComputedViewportValue(viewport);
  ptDestOff.x = actorPos[0];
  ptDestOff.y = static_cast<long>(actorPos[1] - tprop->GetLineOffset());

  // Set up the font color from the text actor
  unsigned char red = 0;
  unsigned char green = 0;
  unsigned char blue = 0;
  unsigned char alpha = 0;
  
  // TOFIX: the default text prop color is set to a special (-1, -1, -1) value
  // to maintain backward compatibility for a while. Text mapper classes will
  // use the Actor2D color instead of the text prop color if this value is 
  // found (i.e. if the text prop color has not been set).

  float* actorColor = tprop->GetColor();
  if (actorColor[0] < 0.0 && actorColor[1] < 0.0 && actorColor[2] < 0.0)
    {
    actorColor = actor->GetProperty()->GetColor();
    }

  // TOFIX: same goes for opacity

  float opacity = tprop->GetOpacity();
  if (opacity < 0.0)
    {
    opacity = actor->GetProperty()->GetOpacity();
    }

  red = (unsigned char) (actorColor[0] * 255.0);
  green = (unsigned char) (actorColor[1] * 255.0);
  blue = (unsigned char) (actorColor[2] * 255.0);
  alpha = (unsigned char) (opacity * 255.0);
  
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
  
  switch (tprop->GetJustification())
    {
    int tmp;
    case VTK_TEXT_LEFT: 
      break;
    case VTK_TEXT_CENTERED:
      tmp = rect.right - rect.left + 1;
      rect.left = rect.left - tmp/2;
      rect.right = rect.left + tmp;
      break;
    case VTK_TEXT_RIGHT: 
      tmp = rect.right - rect.left + 1;
      rect.right = rect.left;
      rect.left = rect.left - tmp;
    }
  switch (tprop->GetVerticalJustification())
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

  float *tileViewport = viewport->GetVTKWindow()->GetTileViewport();
  float *vport = viewport->GetViewport();
  float visVP[4];
  visVP[0] = (vport[0] >= tileViewport[0]) ? vport[0] : tileViewport[0];
  visVP[1] = (vport[1] >= tileViewport[1]) ? vport[1] : tileViewport[1];
  visVP[2] = (vport[2] <= tileViewport[2]) ? vport[2] : tileViewport[2];
  visVP[3] = (vport[3] <= tileViewport[3]) ? vport[3] : tileViewport[3];
  if (visVP[0] == visVP[2])
    {
    return;
    }
  if (visVP[1] == visVP[3])
    {
    return;
    }
 
  int *winSize = viewport->GetVTKWindow()->GetSize();
  int xoff = static_cast<int>
    (rect.left - winSize[0]*((visVP[2] + visVP[0])/2.0 - vport[0]));
  int yoff = static_cast<int>
    (rect.bottom - winSize[1]*((visVP[3] + visVP[1])/2.0 - vport[1]));

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
  if (tprop->GetShadow())
    {
    // set the colors for the foreground
    glColor4ub(shadowRed, shadowGreen, shadowBlue, alpha);
    glRasterPos3f(0,0,(front)?(-1):(.99999));

    // required for clipping to work correctly
    glBitmap(0, 0, 0, 0, xoff + 1, yoff - 1, NULL);
    
    // Draw the shadow text
    glCallLists (vtkString::Length(this->Input), GL_UNSIGNED_BYTE, 
                 this->Input);  
    }
  
  // set the colors for the foreground
  glColor4ub(red, green, blue, alpha);
  glRasterPos3f(0,0,(front)?(-1):(.99999));

  // required for clipping to work correctly
  glBitmap(0, 0, 0, 0, xoff, yoff, NULL);

  // display a string: // indicate start of glyph display lists 
  glCallLists (vtkString::Length(this->Input), GL_UNSIGNED_BYTE, 
               this->Input);  

  glFlush();
#ifndef _WIN32_WCE
  GdiFlush();
#endif  
  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_LIGHTING);

  // Restore the state
  SelectObject(hdc, hOldFont);
}

