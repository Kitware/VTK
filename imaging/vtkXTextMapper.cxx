/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXTextMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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
#include "vtkXTextMapper.h"
#include "vtkObjectFactory.h"

#ifndef VTK_REMOVE_LEGACY_CODE
//mark this class for future legacy-related changes
#endif
//-------------------------------------------------------------------------
vtkXTextMapper* vtkXTextMapper::New()
{
  vtkGenericWarningMacro(<<"Obsolete native imaging class: " 
                         <<"use OpenGL version instead");

  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXTextMapper");
  if(ret)
    {
    return (vtkXTextMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXTextMapper;
}

vtkXTextMapper::vtkXTextMapper()
{
  this->Size[0] = this->Size[1] = 0;
  this->ViewportSize[0] = this->ViewportSize[1] = 0;
}

void vtkXTextMapper::SetFontSize(int size)
{
  int newSize;

  // Make sure that the font size matches an available X font size.
  // This routine assumes that some standard X fonts are installed.
  switch (size)
    {  
    // available X Font sizes
    case 8:
    case 10:
    case 12:
    case 14:
    case 18:
    case 24:
      newSize = size;
      break;

    // In between sizes use next larger size
    case 9:
      newSize = 10;
      break;
    case 11:
      newSize = 12;
      break;
    case 13:
      newSize = 14;
      break;
    case 15:
    case 16:
    case 17:
      newSize = 18;
      break;
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
      newSize = 24;
      break;

    // catch the values outside the font range 
    default:
      if (size < 8) newSize = 8;
      else if (size > 24) newSize  = 24;
      else newSize = 12;   // just in case we missed something above
      break;
    }
  
  if (this->FontSize != newSize)
    {
      this->FontSize = newSize;
      this->FontMTime.Modified();
    }
  return;
}

void vtkXTextMapper::GetSize(vtkViewport* viewport, int *s)
{
  int *vSize = viewport->GetSize();
  
  if (this->SizeMTime < this->MTime || this->SizeMTime < this->FontMTime ||
      vSize[0] != this->ViewportSize[0] || vSize[1] != this->ViewportSize[1])
    {
    this->ViewportSize[0] = vSize[0];
    this->ViewportSize[1] = vSize[1];    
    this->DetermineSize(viewport, s);
    this->SizeMTime.Modified();
    this->Size[0] = s[0];
    this->Size[1] = s[1];
    }
  else
    {
    s[0] = this->Size[0];
    s[1] = this->Size[1];
    }
}

void vtkXTextMapper::DetermineSize(vtkViewport *viewport, int *size)
{
  if ( this->NumberOfLines > 1 )
    {
    this->GetMultiLineSize(viewport, size);
    return;
    }

  if (this->Input == NULL || this->Input[0] == '\0')
    {
    size[0] = 0;
    size[1] = 0;
    return;
    }

  // Get the window info
  vtkWindow*  window = viewport->GetVTKWindow();
  Display* displayId = (Display*) window->GetGenericDisplayId();

  // Set up the font name string. Note: currently there is no checking to see
  // if we've exceeded the fontname length.
  // Foundry
  char fontname[256]= "*";
 
  // Family
  switch (this->FontFamily)
    {
    case VTK_ARIAL:
      strcat(fontname, "helvetica-");
      break;
    case VTK_COURIER:
      strcat(fontname, "courier-");
      break;
    case VTK_TIMES:
      strcat(fontname, "times-");
      break;
    default:
      strcat(fontname, "helvetica-");
    }

  // Weight
  if (this->Bold == 1)
    {
    strcat(fontname, "bold-");
    }
  else
    {
    strcat (fontname, "medium-");
    }

  // Slant
  if (this->Italic == 1)
    {
    if (this->FontFamily == VTK_TIMES) strcat(fontname, "i-");
    else strcat(fontname, "o-");
    }
  else
    {
    strcat(fontname, "r-");
    }

  char tempString[100];
 
  // Set width, pixels, point size
  sprintf(tempString, "*-%d-*", 10*this->FontSize);

  strcat(fontname, tempString);

  vtkDebugMacro(<<"Render - Font specifier: " << fontname);

  // Set the font
  int cnt;
  char **fn = XListFonts(displayId, fontname, 1, &cnt);
  if (fn)
    {
    XFreeFontNames(fn);
    }
  if (!cnt)
    {
    sprintf(fontname,"9x15");
    }
  XFontStruct *fontStruct = XLoadQueryFont(displayId,  fontname );
  int ascent, descent, direction;
  XCharStruct overall;
  // XTextExtents does not require a trip to the server
  XTextExtents(fontStruct, this->Input, strlen(this->Input),
		    &direction, &ascent, &descent, &overall);
  size[1] = ascent + descent;
  size[0] = overall.width;
  this->CurrentFont = fontStruct->fid;
  XFreeFontInfo(NULL, fontStruct, 1);
}

void vtkXTextMapper::RenderOverlay(vtkViewport* viewport, vtkActor2D* actor)
{
  vtkDebugMacro (<< "RenderOverlayGeometry");

  if ( this->NumberOfLines > 1 )
    {
    this->RenderOverlayMultipleLines(viewport, actor);
    return;
    }

  // Check for input
  if (this->Input == NULL) 
    {
    vtkDebugMacro (<<"Render - No input");
    return;
    }
  
  // Get the window info
  vtkWindow*  window = viewport->GetVTKWindow();
  Display* displayId = (Display*) window->GetGenericDisplayId();
  GC gc = (GC) window->GetGenericContext();
  Window windowId = (Window) window->GetGenericWindowId();

  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedLocalDisplayValue(viewport);

  // Set up the font color
  float* actorColor = actor->GetProperty()->GetColor();
  unsigned char red = (unsigned char) (actorColor[0] * 255.0);
  unsigned char green = (unsigned char) (actorColor[1] * 255.0);
  unsigned char  blue = (unsigned char)  (actorColor[2] * 255.0);

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
  
  // Use the color masks from the visual
  unsigned long rmask = 0;
  unsigned long gmask = 0;
  unsigned long bmask = 0;

  XWindowAttributes winAttribs;
  XGetWindowAttributes(displayId, windowId, &winAttribs);
 
  XVisualInfo temp1;
  temp1.visualid = winAttribs.visual->visualid;

  int nvisuals = 0;
  XVisualInfo* visuals = XGetVisualInfo(displayId, VisualIDMask, &temp1,
                                        &nvisuals);   

  if (nvisuals == 0)  vtkErrorMacro(<<"Could not get color masks");

  rmask = visuals->red_mask;
  gmask = visuals->green_mask;
  bmask = visuals->blue_mask;
  
  XFree(visuals);

  // Compute the shifts to match up the pixel bits with the mask bits
  int rshift = 0;
  while ( ((rmask & 0x80000000) == 0) && (rshift < 32))
    {
    rmask = rmask << 1;
    rshift++;
    }

  int gshift = 0;
  while ( ((gmask & 0x80000000) == 0) && (gshift < 32))
    {
    gmask = gmask << 1;
    gshift++;
    }

  int bshift = 0;
  while ( ((bmask & 0x80000000) == 0) && (bshift < 32))
    {
    bmask = bmask << 1;
    bshift++;
    }

  // Mask the colors into the foreground variable
  unsigned long foreground = 0;
  foreground = foreground | ((rmask & (red << 24)) >> rshift);
  foreground = foreground | ((gmask & (green << 24)) >> gshift);
  foreground = foreground | ((bmask & (blue << 24)) >> bshift);

  unsigned long shadowForeground = 0;
  shadowForeground = shadowForeground | ((rmask & (shadowRed << 24)) >>rshift);
  shadowForeground = shadowForeground | ((gmask & (shadowGreen<<24)) >>gshift);
  shadowForeground = shadowForeground | ((bmask & (shadowBlue<< 24)) >>bshift);

  // compute the size of the string so that we can center it etc.
  // a side effect is that this->CurrentFont will be set so that
  // we can use it here. That saves the expensive process of 
  // computing it again.
  int size[2];
  this->GetSize(viewport,size);
  XSetFont(displayId, gc, this->CurrentFont);
  
  // adjust actorPos to account for justification
  int pos[2];
  pos[0] = actorPos[0];
  pos[1] = (int)(actorPos[1] + this->LineOffset);
  switch (this->Justification)
    {
    // do nothing for case 0 left
    case VTK_TEXT_LEFT: break;
    case VTK_TEXT_CENTERED: pos[0] = pos[0] - size[0] / 2; break;
    case VTK_TEXT_RIGHT: pos[0] = pos[0] - size[0]; break;
    }
  switch (this->VerticalJustification)
    {
    case VTK_TEXT_TOP: 
      pos[1] = pos[1] + size[1];
      break;
    case VTK_TEXT_CENTERED:
      pos[1] = pos[1] + size[1]/2;
      break;
    case VTK_TEXT_BOTTOM: 
      break;
    }
    
  // Get the drawable to draw into
  Drawable drawable = (Drawable) window->GetGenericDrawable();
  if (!drawable) vtkErrorMacro(<<"Window returned NULL drawable!");

  // Draw the shadow
  if (this->Shadow)
    {
    XSetForeground(displayId, gc, shadowForeground);
    XDrawString(displayId, drawable, gc, pos[0]+1, pos[1]+1,
                this->Input, strlen(this->Input));
    }
  
  // Draw the string
  XSetForeground(displayId, gc, foreground);
  XDrawString(displayId, drawable, gc, pos[0], pos[1],
              this->Input, strlen(this->Input));
 
  // Flush the X queue
  XFlush(displayId);
  XSync(displayId, False);
}







