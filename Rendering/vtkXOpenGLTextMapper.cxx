/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXOpenGLTextMapper.cxx
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
#include "vtkXOpenGLTextMapper.h"
#include <GL/gl.h>
#include <GL/glx.h>
#include "vtkObjectFactory.h"
#include "vtkgluPickMatrix.h"
#include "vtkProperty2D.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

vtkCxxRevisionMacro(vtkXOpenGLTextMapper, "1.35");
vtkStandardNewMacro(vtkXOpenGLTextMapper);

struct vtkFontStruct
{
  vtkWindow *Window;
  int   Italic;
  int   Bold;
  int   FontSize;
  int   FontFamily;
  int   ListBase;
  GLXContext ContextId;
};
  
static vtkFontStruct *cache[30] = {
  NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL,
  NULL,NULL,NULL,NULL,NULL};
static int numCached = 0;

int vtkXOpenGLTextMapper::GetListBaseForFont(vtkTextMapper *tm, 
                                             vtkViewport *vp, 
                                             Font CurrentFont)
{
  int i, j;
  vtkWindow *win = vp->GetVTKWindow();

  vtkTextProperty *tprop = tm->GetTextProperty();
  int tm_font_size = tm->GetSystemFontSize(tprop->GetFontSize());

  // has the font been cached ?
  for (i = 0; i < numCached; i++)
    {
    if (cache[i]->Window == win &&
        cache[i]->Italic == tprop->GetItalic() &&
        cache[i]->Bold == tprop->GetBold() &&
        cache[i]->FontSize == tm_font_size &&
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
  
  // save the current context
  GLXContext ctx = glXGetCurrentContext();
  
  // OK the font is not cached
  // so we need to make room for a new font
  if (numCached == 30)
    {
    glXMakeCurrent((Display *)cache[29]->Window->GetGenericDisplayId(),
                   (Window)cache[29]->Window->GetGenericWindowId(),
                   cache[29]->ContextId);
    glDeleteLists(cache[29]->ListBase,255);
    glXMakeCurrent((Display *)win->GetGenericDisplayId(),
                   (Window)win->GetGenericWindowId(), ctx);
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
  cache[numCached]->FontSize = tm_font_size;
  cache[numCached]->FontFamily = tprop->GetFontFamily();
  cache[numCached]->ContextId = ctx;
  glXUseXFont(CurrentFont, 0, 255, cache[numCached]->ListBase); 
  
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

void vtkXOpenGLTextMapper::ReleaseGraphicsResources(vtkWindow *win)
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
  this->LastWindow = NULL;
  // forces CurrentFont to be reset if the Mapper is used again.
  this->Modified();
}

vtkXOpenGLTextMapper::vtkXOpenGLTextMapper()
{
}

vtkXOpenGLTextMapper::~vtkXOpenGLTextMapper()
{
  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    }  
}

void vtkXOpenGLTextMapper::RenderOverlay(vtkViewport* viewport, 
                                         vtkActor2D* actor)
{
  vtkDebugMacro (<< "RenderOverlay");

  vtkTextProperty *tprop = this->GetTextProperty();

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
    this->RenderOverlayMultipleLines(viewport, actor);
    return;
    }

  // Check for input
  if (this->Input == NULL || this->Input[0] == '\0') 
    {
    vtkDebugMacro (<<"Render - No input");
    return;
    }

  int size[2];
  this->GetSize(viewport, size);

  // Get the position of the text actor
  int* actorPos = 
    actor->GetActualPositionCoordinate()->GetComputedViewportValue(viewport);

  // Set up the font color from the text actor
  unsigned char red = 0;
  unsigned char green = 0;
  unsigned char blue = 0;
  unsigned char alpha = 0;
  
  // TOFIX: the default text prop color is set to a special (-1, -1, -1) value
  // to maintain backward compatibility for a while. Text mapper classes will
  // use the Actor2D color instead of the text prop color if this value is 
  // found (i.e. if the text prop color has not been set).

  float* actorColor = this->GetTextProperty()->GetColor();
  if (actorColor[0] < 0.0 && actorColor[1] < 0.0 && actorColor[2] < 0.0)
    {
    actorColor = actor->GetProperty()->GetColor();
    }

  // TOFIX: same goes for opacity

  float opacity = this->GetTextProperty()->GetOpacity();
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

  int pos[2];
  pos[0] = actorPos[0];
  pos[1] = (int)(actorPos[1] - tprop->GetLineOffset());

  switch (tprop->GetJustification())
    {
    case VTK_TEXT_LEFT: break;
    case VTK_TEXT_CENTERED:
      pos[0] = pos[0] - size[0]/2;
      break;
    case VTK_TEXT_RIGHT: 
      pos[0] = pos[0] - size[0];
      break;
    }
  switch (tprop->GetVerticalJustification())
    {
    case VTK_TEXT_TOP: 
      pos[1] = pos[1] - size[1];
      break;
    case VTK_TEXT_CENTERED:
      pos[1] = pos[1] - size[1]/2;
      break;
    case VTK_TEXT_BOTTOM: 
      break;
    }
  
  // push a 2D matrix on the stack
  int *vsize = viewport->GetSize();
  float *vport = viewport->GetViewport();

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

  float *tileViewPort = viewport->GetVTKWindow()->GetTileViewport();
  float visVP[4];
  visVP[0] = (vport[0] >= tileViewPort[0]) ? vport[0] : tileViewPort[0];
  visVP[1] = (vport[1] >= tileViewPort[1]) ? vport[1] : tileViewPort[1];
  visVP[2] = (vport[2] <= tileViewPort[2]) ? vport[2] : tileViewPort[2];
  visVP[3] = (vport[3] <= tileViewPort[3]) ? vport[3] : tileViewPort[3];
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
    (pos[0] - winSize[0]*((visVP[2] + visVP[0])/2.0 - vport[0]));
  int yoff = static_cast<int>
    (pos[1] - winSize[1]*((visVP[3] + visVP[1])/2.0 - vport[1]));
  
  // When picking draw the bounds of the text as a rectangle,
  // as text only picks when the pick point is exactly on the
  // origin of the text 
  if(viewport->GetIsPicking())
    {
    float x1 = (2.0*(float)actorPos[0] / vsize[0] - 1);
    float y1 = (2.0*((float)actorPos[1] - tprop->GetLineOffset()) / vsize[1] - 1);
    float width = 2.0*(float)size[0]/vsize[0];
    float height = 2.0*(float)size[1]/vsize[1];
    glRectf(x1, y1, x1+width, y1+height);

    // Clean up and return after drawing the rectangle
    glMatrixMode( GL_PROJECTION);
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW);
    glPopMatrix();
    glEnable( GL_LIGHTING);
    
    return;
    }
  
  glListBase(vtkXOpenGLTextMapper::GetListBaseForFont(this,viewport,
                                                      this->CurrentFont));
              
  // Set the colors for the shadow
  if (tprop->GetShadow())
    {
    // set the colors for the foreground
    glColor4ub(shadowRed, shadowGreen, shadowBlue, alpha);
    glRasterPos3f(0,0,(front)?(-1):(.99999));
    
    // required for clipping to work correctly
    glBitmap(0, 0, 0, 0, xoff + 1, yoff - 1, NULL);
    
    // Draw the shadow text
    glCallLists (strlen(this->Input), GL_UNSIGNED_BYTE, this->Input);  
    }
  
  // set the colors for the foreground
  glColor4ub(red, green, blue, alpha);
  
  // center the raster pos
  glRasterPos3f(0,0,(front)?(-1):(.99999));
  
  // required for clipping to work correctly
  glBitmap(0, 0, 0, 0, xoff, yoff, NULL);
  
  // display a string: // indicate start of glyph display lists 
  glCallLists (strlen(this->Input), GL_UNSIGNED_BYTE, this->Input);  

  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_LIGHTING);
}

