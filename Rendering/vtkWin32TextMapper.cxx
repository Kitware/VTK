/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32TextMapper.cxx
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
#include "vtkWin32TextMapper.h"

#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

vtkCxxRevisionMacro(vtkWin32TextMapper, "1.29");

//--------------------------------------------------------------------------
vtkWin32TextMapper* vtkWin32TextMapper::New()
{
  vtkGenericWarningMacro(<<"Obsolete native imaging class: " 
                         <<"use OpenGL version instead");

  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWin32TextMapper");
  if(ret)
    {
    return (vtkWin32TextMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWin32TextMapper;
}

vtkWin32TextMapper::vtkWin32TextMapper()
{
  this->LastSize[0] = 0;
  this->LastSize[1] = 0;
  this->Font = 0;
}

vtkWin32TextMapper::~vtkWin32TextMapper()
{
  if ( this->Font )
    {
    DeleteObject( this->Font );
    }
}

void vtkWin32TextMapper::GetSize(vtkViewport* viewport, int *size)
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

  vtkTextProperty *tprop = this->GetTextProperty();
 
  // Check to see whether we have to rebuild anything
  if (this->GetMTime() < this->BuildTime &&
      tprop->GetMTime() < this->BuildTime)
    {
    size[0] = this->LastSize[0];
    size[1] = this->LastSize[1];
    return;
    }

  // Check for input
  if (this->Input == NULL) 
    {
    vtkErrorMacro (<<"vtkWin32TextMapper::Render - No input");
    return;
    }

  // Get the window information for display
  vtkWindow*  window = viewport->GetVTKWindow();
  // Get the device context from the window
  HDC hdc = (HDC) window->GetGenericContext();

  // Create the font
  LOGFONT fontStruct;
  char fontname[32];
  DWORD family;
  switch (tprop->GetFontFamily())
    {
    case VTK_ARIAL:
      strcpy(fontname, "Arial");
          family = FF_SWISS;
          break;
        case VTK_TIMES:
      strcpy(fontname, "Times Roman");
          family = FF_ROMAN;
          break;
        case VTK_COURIER:
      strcpy(fontname, "Courier");
          family = FF_MODERN;
          break;
        default:
      strcpy(fontname, "Arial");
          family = FF_SWISS;
          break;
    }
#ifdef _WIN32_WCE
  fontStruct.lfHeight = tprop->GetFontSize() * window->GetDPI() / 72;  
#else
  fontStruct.lfHeight = MulDiv(tprop->GetFontSize(), 
                               window->GetDPI(), 72);  
#endif
  // height in logical units
  fontStruct.lfWidth = 0;  // default width
  fontStruct.lfEscapement = 0;
  fontStruct.lfOrientation = 0;
  if (tprop->GetBold() == 1)
    {
    fontStruct.lfWeight = FW_BOLD;
    }
  else 
    {
    fontStruct.lfWeight = FW_NORMAL;
    }
  fontStruct.lfItalic = tprop->GetItalic();
  fontStruct.lfUnderline = 0;
  fontStruct.lfStrikeOut = 0;
  fontStruct.lfCharSet = ANSI_CHARSET;
  fontStruct.lfOutPrecision = OUT_DEFAULT_PRECIS;
  fontStruct.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  fontStruct.lfQuality = DEFAULT_QUALITY;
  fontStruct.lfPitchAndFamily = DEFAULT_PITCH | family;
#ifdef _WIN32_WCE
  mbstowcs(fontStruct.lfFaceName, fontname, strlen(fontname));
#else
  strcpy(fontStruct.lfFaceName, fontname);
#endif
   
  if (this->Font)
    {
    DeleteObject(this->Font);
    }
  this->Font = CreateFontIndirect(&fontStruct);
  HFONT hOldFont = (HFONT) SelectObject(hdc, this->Font);

  // Define bounding rectangle
  RECT rect;
  rect.left = 0;
  rect.top = 0;
  rect.bottom = 0;
  rect.right = 0;

  // Calculate the size of the bounding rectangle
#ifdef UNICODE
        wchar_t *wtxt = new wchar_t [mbstowcs(NULL, this->Input, 32000)];
        mbstowcs(wtxt, this->Input, 32000);
  size[1] = DrawText(hdc, wtxt, -1, &rect, 
                     DT_CALCRECT|DT_LEFT|DT_NOPREFIX);
        delete [] wtxt;
#else
  size[1] = static_cast<int>(DrawText(hdc, this->Input, 
                                      static_cast<int>(strlen(this->Input)), 
                                      &rect, 
                                      DT_CALCRECT|DT_LEFT|DT_NOPREFIX));
#endif
  size[0] = rect.right - rect.left + 1;
  this->LastSize[0] = size[0];
  this->LastSize[1] = size[1];
  this->BuildTime.Modified();
  SelectObject(hdc, hOldFont);
}



