/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaTextMapper.cxx
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
#include "vtkCocoaTextMapper.h"

#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

vtkCxxRevisionMacro(vtkCocoaTextMapper, "1.4");
vtkStandardNewMacro(vtkCocoaTextMapper);

vtkCocoaTextMapper::vtkCocoaTextMapper()
{
  this->LastSize[0] = 0;
  this->LastSize[1] = 0;
  this->Font = 0;
}

vtkCocoaTextMapper::~vtkCocoaTextMapper()
{
  if ( this->Font )
    {
//    DeleteObject( this->Font );
    }
}

void vtkCocoaTextMapper::GetSize(vtkViewport* viewport, int *size)
{
  if ( this->NumberOfLines > 1 )
    {
    this->GetMultiLineSize(viewport, size);
    return;
    }

  if (this->Input == NULL)
    {
    size[0] = 0; size[1] = 0;
    return;
    }

  vtkTextProperty *tprop = this->GetTextProperty();
  if (!tprop)
    {
    vtkErrorMacro(<< "Need a text property to get size");
    size[0] = 0; size[1] = 0;
    return;
    }
 
  // Check to see whether we have to rebuild anything
  if ( this->GetMTime() < this->BuildTime &&
       tprop->GetMTime() < this->BuildTime)
    {
    size[0] = this->LastSize[0];
    size[1] = this->LastSize[1];
    return;
    }

  // Check for input
  if (this->Input == NULL) 
    {
    vtkErrorMacro (<<"vtkCocoaTextMapper::Render - No input");
    return;
    }

  // Get the window information for display
  vtkWindow*  window = viewport->GetVTKWindow();
  // Get the device context from the window
  void *hdc = window->GetGenericContext();
 
  // Create the font
  void *fontStruct;
  char fontname[32];
  void *family;
  switch (tprop->GetFontFamily())
    {
    case VTK_ARIAL:
      strcpy(fontname, "Arial");
        //  family = FF_SWISS;
          break;
        case VTK_TIMES:
      strcpy(fontname, "Times Roman");
         // family = FF_ROMAN;
          break;
        case VTK_COURIER:
      strcpy(fontname, "Courier");
        //  family = FF_MODERN;
          break;
        default:
      strcpy(fontname, "Arial");
        //  family = FF_SWISS;
          break;
    }
//  fontStruct.lfHeight = MulDiv(tprop->GetFontSize(), 
//                             window->GetDPI(), 72);  
  // height in logical units
//  fontStruct.lfWidth = 0;  // default width
//  fontStruct.lfEscapement = 0;
//  fontStruct.lfOrientation = 0;
  if (tprop->GetBold() == 1)
    {
  //  fontStruct.lfWeight = FW_BOLD;
    }
  else 
    {
  //  fontStruct.lfWeight = FW_NORMAL;
    }
//  fontStruct.lfItalic = tprop->GetItalic();
//  fontStruct.lfUnderline = 0;
//  fontStruct.lfStrikeOut = 0;
//  fontStruct.lfCharSet = ANSI_CHARSET;
//  fontStruct.lfOutPrecision = OUT_DEFAULT_PRECIS;
//  fontStruct.lfClipPrecision = CLIP_DEFAULT_PRECIS;
//  fontStruct.lfQuality = DEFAULT_QUALITY;
//  fontStruct.lfPitchAndFamily = DEFAULT_PITCH | family;
//  strcpy(fontStruct.lfFaceName, fontname);
   
  if (this->Font)
    {
//    DeleteObject(this->Font);
    }
//  this->Font = CreateFontIndirect(&fontStruct);
//  HFONT hOldFont = (HFONT) SelectObject(hdc, this->Font);

  // Define bounding rectangle
//  RECT rect;
//  rect.left = 0;
//  rect.top = 0;
//  rect.bottom = 0;
//  rect.right = 0;

  // Calculate the size of the bounding rectangle
//  size[1] = DrawText(hdc, this->Input, strlen(this->Input), &rect, 
//                   DT_CALCRECT|DT_LEFT|DT_NOPREFIX);
//  size[0] = rect.right - rect.left + 1;
  this->LastSize[0] = size[0];
  this->LastSize[1] = size[1];
  this->BuildTime.Modified();
//  SelectObject(hdc, hOldFont);
}

void vtkCocoaTextMapper::RenderOverlay(vtkViewport* viewport, 
                                       vtkActor2D* actor)
{
  vtkDebugMacro (<< "RenderOverlay");

  // Check for input
  if ( this->NumberOfLines > 1 )
    {
    this->RenderOverlayMultipleLines(viewport, actor);
    return;
    }

  // Check for input
  if (this->Input == NULL) 
    {
    vtkErrorMacro (<<"Render - No input");
    return;
    }

  vtkTextProperty *tprop = this->GetTextProperty();
  if (!tprop)
    {
    vtkErrorMacro(<< "Need a text property to render mapper");
    return;
    }

  int size[2];
  this->GetSize(viewport, size);

  // Get the window information for display
  vtkWindow*  window = viewport->GetVTKWindow();
  // Get the device context from the window
  void *hdc = window->GetGenericContext();
 
  // Select the font
//  HFONT hOldFont = (HFONT) SelectObject(hdc, this->Font);
  
  // Get the position of the text actor
//  POINT ptDestOff;
  int* actorPos = 
    actor->GetActualPositionCoordinate()->GetComputedLocalDisplayValue(viewport);
//  ptDestOff.x = actorPos[0];
//  ptDestOff.y = actorPos[1] - tprop->GetLineOffset();

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
//  RECT rect;
//  rect.left = ptDestOff.x;
//  rect.top = ptDestOff.y;
//  rect.bottom = ptDestOff.y;
//  rect.right = ptDestOff.x;

//  SelectObject(hdc, this->Font);
//  rect.right = rect.left + size[0];
//  rect.top = rect.bottom - size[1];
  
  // Set the compositing operator
//  SetROP2(hdc, R2_COPYPEN);

  int winJust;
  switch (tprop->GetJustification())
    {
    int tmp;
//    case VTK_TEXT_LEFT: winJust = DT_LEFT; break;
    case VTK_TEXT_CENTERED:
 //     winJust = DT_CENTER;
  //    tmp = rect.right - rect.left + 1;
  //    rect.left = rect.left - tmp/2;
  //    rect.right = rect.left + tmp;
      break;
    case VTK_TEXT_RIGHT: 
  //    winJust = DT_RIGHT;
  //    tmp = rect.right - rect.left + 1;
  //    rect.left = rect.right;
  //    rect.right = rect.right - tmp;
        break;
    }
  switch (tprop->GetVerticalJustification())
    {
    case VTK_TEXT_TOP: 
   //   rect.top = rect.bottom;
   //   rect.bottom = rect.bottom - size[1];
      break;
    case VTK_TEXT_CENTERED:
   //   rect.bottom = rect.bottom - size[1]/2;
   //   rect.top = rect.bottom + size[1];
      break;
    case VTK_TEXT_BOTTOM: 
      break;
    }

  // Set the colors for the shadow
  long status;
  if (tprop->GetShadow())
    {
 //   status = SetTextColor(hdc, RGB(shadowRed, shadowGreen, shadowBlue));
//    if (status == CLR_INVALID)
 //     vtkErrorMacro(<<"vtkCocoaTextMapper::Render - Set shadow color failed!");

    // Set the background mode to transparent
 //   SetBkMode(hdc, TRANSPARENT);

    // Draw the shadow text
//    rect.left++;  rect.top++; rect.bottom++; rect.right++;
//    DrawText(hdc, this->Input, strlen(this->Input), &rect,winJust|DT_NOPREFIX);
//    rect.left--;  rect.top--; rect.bottom--; rect.right--;
    }
  
  // set the colors for the foreground
//  status = SetTextColor(hdc, RGB(red, green, blue));
 // if (status == CLR_INVALID)
  //  vtkErrorMacro(<<"vtkCocoaTextMapper::Render - SetTextColor failed!");

  // Set the background mode to transparent
//  SetBkMode(hdc, TRANSPARENT);

  // Draw the text
//  DrawText(hdc, this->Input, strlen(this->Input), &rect, winJust|DT_NOPREFIX);

  // Return the state
//  SelectObject(hdc, hOldFont);

}


