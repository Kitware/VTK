/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32TextMapper.cxx
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
#include "vtkWin32TextMapper.h"
#include "vtkObjectFactory.h"

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
  switch (this->FontFamily)
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
  fontStruct.lfHeight = MulDiv(this->FontSize, 
			       window->GetDPI(), 72);  
  // height in logical units
  fontStruct.lfWidth = 0;  // default width
  fontStruct.lfEscapement = 0;
  fontStruct.lfOrientation = 0;
  if (this->Bold == 1)
    {
    fontStruct.lfWeight = FW_BOLD;
    }
  else 
    {
    fontStruct.lfWeight = FW_NORMAL;
    }
  fontStruct.lfItalic = this->Italic;
  fontStruct.lfUnderline = 0;
  fontStruct.lfStrikeOut = 0;
  fontStruct.lfCharSet = ANSI_CHARSET;
  fontStruct.lfOutPrecision = OUT_DEFAULT_PRECIS;
  fontStruct.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  fontStruct.lfQuality = DEFAULT_QUALITY;
  fontStruct.lfPitchAndFamily = DEFAULT_PITCH | family;
  strcpy(fontStruct.lfFaceName, fontname);
   
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
  size[1] = DrawText(hdc, this->Input, strlen(this->Input), &rect, 
		     DT_CALCRECT|DT_LEFT|DT_NOPREFIX);
  size[0] = rect.right - rect.left + 1;
  this->LastSize[0] = size[0];
  this->LastSize[1] = size[1];
  this->BuildTime.Modified();
  SelectObject(hdc, hOldFont);
}



