/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXTextMapper.cxx
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
#include "vtkXTextMapper.h"

#include "vtkObjectFactory.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

#ifndef VTK_REMOVE_LEGACY_CODE
//mark this class for future legacy-related changes
#endif

vtkCxxRevisionMacro(vtkXTextMapper, "1.33");

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

int vtkXTextMapper::GetMatchingFontSize()
{
  int newSize;
  int size = this->TextProperty->GetSize();

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
  
  return newSize;
}

void vtkXTextMapper::GetSize(vtkViewport* viewport, int *s)
{
  int *vSize = viewport->GetSize();
  
  if (this->SizeMTime < this->MTime || 
      this->SizeMTime < this->TextProperty->GetMTime() ||
      vSize[0] != this->ViewportSize[0] || 
      vSize[1] != this->ViewportSize[1])
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
  switch (this->TextProperty->GetFontFamily())
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
  if (this->TextProperty->GetBold() == 1)
    {
    strcat(fontname, "bold-");
    }
  else
    {
    strcat (fontname, "medium-");
    }

  // Slant
  if (this->TextProperty->GetItalic() == 1)
    {
    if (this->TextProperty->GetFontFamily() == VTK_TIMES) strcat(fontname, "i-");
    else strcat(fontname, "o-");
    }
  else
    {
    strcat(fontname, "r-");
    }

  char tempString[100];
 
  // Set width, pixels, point size
  sprintf(tempString, "*-%d-*", 10 * this->GetMatchingFontSize());

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






