/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCarbonTextMapper.cxx
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
#include "vtkCarbonTextMapper.h"
#include "vtkCarbonRenderWindow.h"
#include <OpenGL/gl.h>
#include <AGL/agl.h>
#include "vtkObjectFactory.h"
#include "vtkgluPickMatrix.h"
#include "vtkString.h"

vtkCxxRevisionMacro(vtkCarbonTextMapper, "1.3");
vtkStandardNewMacro(vtkCarbonTextMapper);

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

// Copy C string to Pascal string
// Some of the Carbon routines require Pascal strings
static void CStrToPStr (StringPtr outString, const char *inString)
{
  unsigned char x = 0;
  do
    *(((char*)outString) + x + 1) = *(inString + x++);
  while ((*(inString + x) != 0)  && (x < 256));
  *((char*)outString) = (char) x;
}


void vtkCarbonTextMapper::GetSize(vtkViewport* viewport, int *size)
{
  if ( this->NumberOfLines > 1 )
  {
    this->GetMultiLineSize(viewport, size);
    return;
  }

  if (this->Input == NULL)
  {
    size[0] = 0;
    size[1] = 0;
    return;
  }

  // Check to see whether we have to rebuild anything
  if ( this->GetMTime() < this->BuildTime)
  {
    size[0] = this->LastSize[0];
    size[1] = this->LastSize[1];
    return;
  }

  // Check for input
  if (this->Input == NULL)
  {
    vtkErrorMacro (<<"vtkCarbonTextMapper::GetSize - No input");
    return;
  }

  // Get the window information for display
  vtkWindow*  window = viewport->GetVTKWindow();
  // Get the device context from the window
  AGLDrawable hdc = (AGLDrawable) window->GetGenericContext();

  // Get the font number
  switch (this->FontFamily)
  {
    case VTK_ARIAL:
      GetFNum("\pArial", &(this->currentFontNum));
      break;
    case VTK_TIMES:
      GetFNum("\pTimes", &(this->currentFontNum));
      break;
    case VTK_COURIER:
      GetFNum("\pCourier", &(this->currentFontNum));
      break;
    default:
      GetFNum("\pArial", &(this->currentFontNum));
      break;
  }

  TextFont(this->currentFontNum);
  TextFace(normal + (italic*this->Italic) + (bold*this->Bold) +
           (shadow*this->Shadow));
  TextSize(this->FontSize);

  GetFontInfo(&(this->myFontInfo));
  
  // Calculate the size of the bounding rectangle
  Str255 testString = "\p"; //StringWidth needs a pascal string
  CStrToPStr(testString, this->Input);
  size[0] = StringWidth(testString);
  size[1] = (this->myFontInfo.ascent +
            this->myFontInfo.descent +
            this->myFontInfo.leading);
  
  this->LastSize[0] = size[0];
  this->LastSize[1] = size[1];
  this->BuildTime.Modified();
}



int vtkCarbonTextMapper::GetListBaseForFont(vtkViewport *vp)
{
  int i, j;
  vtkCarbonRenderWindow *win = (vtkCarbonRenderWindow *)(vp->GetVTKWindow());

  // has the font been cached ?
  for (i = 0; i < numCached; i++)
    {
    if (cache[i]->Window == win &&
        cache[i]->Italic == this->GetItalic() &&
        cache[i]->Bold == this->GetBold() &&
        cache[i]->FontSize == this->GetFontSize() &&
        cache[i]->FontFamily == this->GetFontFamily())
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
  
  AGLDrawable hdc = (AGLDrawable) win->GetGenericContext();

  // OK the font is not cached
  // so we need to make room for a new font
  if (numCached == 30)
    {
      aglSetCurrentContext((AGLContext)cache[29]->Window->GetGenericDisplayId());
    glDeleteLists(cache[29]->ListBase,255);
    aglSetCurrentContext((AGLContext)win->GetGenericDisplayId());
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

  short fNum = 0;
  // set the other info and build the font
  cache[numCached]->Window = win;
  cache[numCached]->Italic = this->GetItalic();
  cache[numCached]->Bold = this->GetBold();
  cache[numCached]->FontSize = this->GetFontSize();
  cache[numCached]->FontFamily = this->GetFontFamily();
  if (cache[numCached]->FontSize < 9)
    cache[numCached]->FontSize = 9; // minimum font size (or it goes blank!)
  aglUseFont((AGLContext)win->GetGenericDisplayId(), cache[numCached]->FontFamily,
             normal+(italic*this->Italic) + (bold*this->Bold) +
             (shadow*this->Shadow), cache[numCached]->FontSize, 0, 255, cache[numCached]->ListBase);
  GLenum err = aglGetError();
  if (AGL_NO_ERROR != err)
    cout << "vtkCarbonMapper AGLError: "<<(char *)aglErrorString(err)<<"\n";
  
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

void vtkCarbonTextMapper::ReleaseGraphicsResources(vtkWindow *win)
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
  
  // very important
  // the release of graphics resources indicates that significant changes have
  // occurred. Old fonts, cached sizes etc are all no longer valid, so we send
  // ourselves a general modified message.
  this->Modified();
}

vtkCarbonTextMapper::vtkCarbonTextMapper()
{
}

vtkCarbonTextMapper::~vtkCarbonTextMapper()
{
  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    }  
}

void vtkCarbonTextMapper::RenderOverlay(vtkViewport* viewport, 
                                             vtkActor2D* actor)
{
  vtkDebugMacro (<< "RenderOverlay");

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

  if ( this->Input == NULL ) 
    {
    vtkErrorMacro (<<"Render - No input");
    return;
    }

  int size[2];
  this->GetSize(viewport, size);

  // Get the device context from the window
  AGLDrawable hdc = (AGLDrawable) window->GetGenericContext();
 
  // Get the position of the text actor
  Point ptDestOff;
  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);
  ptDestOff.h = actorPos[0];
  ptDestOff.v = static_cast<long>(actorPos[1] - this->LineOffset);

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
  Rect rect;
  rect.left = ptDestOff.h;
  rect.top = ptDestOff.v;
  rect.bottom = ptDestOff.v;
  rect.right = ptDestOff.h;

  rect.right = rect.left + size[0];
  rect.top = rect.bottom + size[1];
  
  switch (this->Justification)
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
    
    return;
    }
  
  glListBase(this->GetListBaseForFont(viewport));

  // Set the colors for the shadow
  if (this->Shadow)
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
  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_LIGHTING);
}

