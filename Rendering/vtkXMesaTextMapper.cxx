/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMesaTextMapper.cxx
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
#include "GL/gl_mangle.h"
#include "GL/glx_mangle.h"
#include "GL/gl.h"
#include "GL/glx.h"
#include "GL/osmesa.h"

#include "vtkToolkits.h"
#include "vtkXMesaTextMapper.h"
#include "vtkObjectFactory.h"


static void
vtkFillBitmap (Display *dpy, Window win, GC gc,
	       unsigned int width, unsigned int height,
	       int x0, int y0, char c, GLubyte *bitmap)
{
  XImage *image;
  unsigned int x, y;
  Pixmap pixmap;

  pixmap = XCreatePixmap (dpy, win, 8*width, height, 1);
  XSetForeground(dpy, gc, 0);
  XFillRectangle (dpy, pixmap, gc, 0, 0, 8*width, height);
  XSetForeground(dpy, gc, 1);
  XDrawString (dpy, pixmap, gc, x0, y0, &c, 1);

  image = XGetImage (dpy, pixmap, 0, 0, 8*width, height, 1, XYPixmap);

  /* Fill the bitmap (X11 and OpenGL are upside down wrt each other).  */
  for (y = 0; y < height; y++)
    for (x = 0; x < 8*width; x++)
      if (XGetPixel (image, x, y))
	bitmap[width*(height - y - 1) + x/8] |= (1 << (7 - (x % 8)));
  
  XFreePixmap (dpy, pixmap);
  XDestroyImage (image);
}


void vtkOSUseXFont( Display *dpy, Font font, int first, int count, int listbase )
{
  Pixmap pixmap;
  GC gc;
  XGCValues values;
  unsigned long valuemask;

  XFontStruct *fs;

  GLint swapbytes, lsbfirst, rowlength;
  GLint skiprows, skippixels, alignment;

  unsigned int max_width, max_height, max_bm_width, max_bm_height;
  GLubyte *bm;

  int i;

  fs = XQueryFont (dpy, font);  
  if (!fs)
    {
      return;
    }

  /* Allocate a bitmap that can fit all characters.  */
  max_width = fs->max_bounds.rbearing - fs->min_bounds.lbearing;
  max_height = fs->max_bounds.ascent + fs->max_bounds.descent;
  max_bm_width = (max_width + 7) / 8;
  max_bm_height = max_height;

  bm = (GLubyte *) malloc ((max_bm_width * max_bm_height) * sizeof (GLubyte));
  if (!bm)
    {
      return;
    }

  /* Save the current packing mode for bitmaps.  */
  glGetIntegerv	(GL_UNPACK_SWAP_BYTES, &swapbytes);
  glGetIntegerv	(GL_UNPACK_LSB_FIRST, &lsbfirst);
  glGetIntegerv	(GL_UNPACK_ROW_LENGTH, &rowlength);
  glGetIntegerv	(GL_UNPACK_SKIP_ROWS, &skiprows);
  glGetIntegerv	(GL_UNPACK_SKIP_PIXELS, &skippixels);
  glGetIntegerv	(GL_UNPACK_ALIGNMENT, &alignment);

  /* Enforce a standard packing mode which is compatible with
     fill_bitmap() from above.  This is actually the default mode,
     except for the (non)alignment.  */
  glPixelStorei	(GL_UNPACK_SWAP_BYTES, GL_FALSE);
  glPixelStorei	(GL_UNPACK_LSB_FIRST, GL_FALSE);
  glPixelStorei	(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei	(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei	(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei	(GL_UNPACK_ALIGNMENT, 1);

  pixmap = XCreatePixmap (dpy, RootWindow(dpy,DefaultScreen(dpy)), 10, 10, 1);
  values.foreground = BlackPixel (dpy, DefaultScreen (dpy));
  values.background = WhitePixel (dpy, DefaultScreen (dpy));
  values.font = fs->fid;
  valuemask = GCForeground | GCBackground | GCFont;
  gc = XCreateGC (dpy, pixmap, valuemask, &values);
  XFreePixmap (dpy, pixmap);

  for (i = 0; i < count; i++)
    {
      unsigned int width, height, bm_width, bm_height;
      GLfloat x0, y0, dx, dy;
      XCharStruct *ch;
      int x, y;
      unsigned int c = first + i;
      int list = listbase + i;

      if (fs->per_char
	  && (c >= fs->min_char_or_byte2) && (c <= fs->max_char_or_byte2))
	ch = &fs->per_char[c-fs->min_char_or_byte2];
      else
	ch = &fs->max_bounds;
      
      /* glBitmap()' parameters:
	 straight from the glXUseXFont(3) manpage.  */
      width = ch->rbearing - ch->lbearing;
      height = ch->ascent + ch->descent;
      x0 = - ch->lbearing;
      y0 = ch->descent - 1;
      dx = ch->width;
      dy = 0;

      /* X11's starting point.  */
      x = - ch->lbearing;
      y = ch->ascent;
      
      /* Round the width to a multiple of eight.  We will use this also
	 for the pixmap for capturing the X11 font.  This is slightly
	 inefficient, but it makes the OpenGL part real easy.  */
      bm_width = (width + 7) / 8;
      bm_height = height;

      glNewList (list, GL_COMPILE);
      if ((c >= fs->min_char_or_byte2) && (c <= fs->max_char_or_byte2)
	  && (bm_width > 0) && (bm_height > 0))
	{
	memset(bm, '\0', bm_width * bm_height);
	vtkFillBitmap (dpy, RootWindow(dpy,DefaultScreen(dpy)), 
		       gc, bm_width, bm_height, x, y, c, bm);
	glBitmap (width, height, x0, y0, dx, dy, bm);
	}
      else
	glBitmap (0, 0, 0.0, 0.0, dx, dy, NULL);
      glEndList ();
    }

  free (bm);
  XFreeFontInfo( NULL, fs, 0 );
  XFreeGC (dpy, gc);

  /* Restore saved packing modes.  */    
  glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbytes);
  glPixelStorei(GL_UNPACK_LSB_FIRST, lsbfirst);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, rowlength);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, skiprows);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, skippixels);
  glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
}

//------------------------------------------------------------------------------
vtkXMesaTextMapper* vtkXMesaTextMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXMesaTextMapper");
  if(ret)
    {
    return (vtkXMesaTextMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXMesaTextMapper;
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

int vtkXMesaTextMapper::GetListBaseForFont(vtkTextMapper *tm, 
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
  GLXContext ctx = NULL;
  
  // OK the font is not cached
  // so we need to make room for a new font
  if (numCached == 10)
    {
    if (cache[9]->Window->GetOffScreenRendering())
      {
      OSMesaMakeCurrent((OSMesaContext)cache[9]->Window->GetGenericContext(),
			cache[9]->Window->GetGenericWindowId(), 
			GL_UNSIGNED_BYTE, 
			cache[9]->Window->GetSize()[0],
			cache[9]->Window->GetSize()[1]);
      }
    else
      {
      // save the current context
      ctx = glXGetCurrentContext();
      glXMakeCurrent((Display *)cache[9]->Window->GetGenericDisplayId(),
		     (Window)cache[9]->Window->GetGenericWindowId(),
		     cache[9]->ContextId);
      }
    glDeleteLists(cache[9]->ListBase,255);
    if (win->GetOffScreenRendering())
      {
      OSMesaMakeCurrent((OSMesaContext)win->GetGenericContext(),
			win->GetGenericWindowId(), 
			GL_UNSIGNED_BYTE, 
			win->GetSize()[0],
			win->GetSize()[1]);
      }
    else
      {
      glXMakeCurrent((Display *)win->GetGenericDisplayId(),
		     (Window)win->GetGenericWindowId(), ctx);
      }
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
  if (win->GetOffScreenRendering())
    {
    vtkOSUseXFont((Display *)win->GetGenericDisplayId(),
		  CurrentFont, 0, 255, cache[numCached]->ListBase); 
    }  
  else
    {
    glXUseXFont(CurrentFont, 0, 255, cache[numCached]->ListBase); 
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

void vtkXMesaTextMapper::ReleaseGraphicsResources(vtkWindow *win)
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

vtkXMesaTextMapper::vtkXMesaTextMapper()
{
}

void vtkXMesaTextMapper::RenderOpaqueGeometry(vtkViewport* viewport, 
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
  if (actor->GetProperty()->GetDisplayLocation() == VTK_FOREGROUND_LOCATION)
    {
    glOrtho(0,vsize[0] -1, 0, vsize[1] -1, 0, 1);
    }
  else
    {
    glOrtho(0,vsize[0] -1, 0, vsize[1] -1, -1, 0);
    }
  
  glDisable( GL_LIGHTING);

  glListBase(vtkXMesaTextMapper::GetListBaseForFont(this,viewport,
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

