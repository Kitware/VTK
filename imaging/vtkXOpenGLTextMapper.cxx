/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXOpenGLTextMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkXOpenGLTextMapper.h"
#include <GL/gl.h>
#include <GL/glx.h>
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkXOpenGLTextMapper* vtkXOpenGLTextMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXOpenGLTextMapper");
  if(ret)
    {
    return (vtkXOpenGLTextMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXOpenGLTextMapper;
}




struct vtkFontStruct
{
  vtkWindow *Window;
  int   Italic;
  int	Bold;
  int   FontSize;
  int   FontFamily;
  int   ListBase;
  GLXContext ContextId;
};
  
static vtkFontStruct *cache[10] = {NULL,NULL,NULL,NULL,NULL,
				   NULL,NULL,NULL,NULL,NULL};
static int numCached = 0;

int vtkXOpenGLTextMapper::GetListBaseForFont(vtkTextMapper *tm, 
					     vtkViewport *vp, 
					     Font CurrentFont)
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
  
  // save the current context
  GLXContext ctx = glXGetCurrentContext();
  
  // OK the font is not cached
  // so we need to make room for a new font
  if (numCached == 10)
    {
    glXMakeCurrent((Display *)cache[9]->Window->GetGenericDisplayId(),
		   (Window)cache[9]->Window->GetGenericWindowId(),
		   cache[9]->ContextId);
    glDeleteLists(cache[9]->ListBase,255);
    glXMakeCurrent((Display *)win->GetGenericDisplayId(),
		   (Window)win->GetGenericWindowId(), ctx);
    numCached = 9;
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
      glDeleteLists(cache[i]->ListBase,255);
      delete cache[i];
      // resort them
      numCached--;
      for (j = i; j < numCached; j++)
	{
	cache[j] = cache[j+1];
	}
      cache[numCached] = NULL;
      }
    }
}

vtkXOpenGLTextMapper::vtkXOpenGLTextMapper()
{
}

void vtkXOpenGLTextMapper::RenderOpaqueGeometry(vtkViewport* viewport, 
						vtkActor2D* actor)
{
  vtkDebugMacro (<< "RenderOpaqueGeometry");

  // Check for input
  if ( this->NumberOfLines > 1 )
    {
    this->RenderOpaqueGeometryMultipleLines(viewport, actor);
    return;
    }

  // Check for input
  if (this->Input == NULL) 
    {
    vtkDebugMacro (<<"Render - No input");
    return;
    }

  int size[2];
  this->GetSize(viewport, size);

  // Get the position of the text actor
  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);

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

  int pos[2];
  pos[0] = actorPos[0];
  pos[1] = (int)(actorPos[1] - this->LineOffset);

  switch (this->Justification)
    {
    case VTK_TEXT_LEFT: break;
    case VTK_TEXT_CENTERED:
      pos[0] = pos[0] - size[0]/2;
      break;
    case VTK_TEXT_RIGHT: 
      pos[0] = pos[0] - size[0];
      break;
    }
  switch (this->VerticalJustification)
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
  glMatrixMode( GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0,vsize[0] -1, 0, vsize[1] -1, 0, 1);
  glDisable( GL_LIGHTING);

  glListBase(vtkXOpenGLTextMapper::GetListBaseForFont(this,viewport,
						      this->CurrentFont));
	      
  // Set the colors for the shadow
  if (this->Shadow)
    {
    pos[0]++; pos[1]--;
    // set the colors for the foreground
    glColor3ub(shadowRed, shadowGreen, shadowBlue);
    glRasterPos2i(pos[0],pos[1]);

    // Draw the shadow text
    glCallLists (strlen(this->Input), GL_UNSIGNED_BYTE, this->Input);  
    pos[0]--;  pos[1]++; 
    }
  
  // set the colors for the foreground
  glColor3ub(red, green, blue);
  glRasterPos2i(pos[0],pos[1]);

  // display a string: // indicate start of glyph display lists 
  glCallLists (strlen(this->Input), GL_UNSIGNED_BYTE, this->Input);  

  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_LIGHTING);
}

