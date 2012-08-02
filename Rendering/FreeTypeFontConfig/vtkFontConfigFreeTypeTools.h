/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFontConfigFreeTypeTools.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkFontConfigFreeTypeTools - Subclass of vtkFreeTypeTools that uses
// system installed fonts.
//
// .SECTION Description
// vtkFontConfigFreeTypeTools defers to vtkFreeTypeTools for rendering and
// rasterization, but sources fonts from a FontConfig system lookup. If the
// lookup fails, the compiled fonts of vtkFreeType are used instead.
//
// .SECTION Caveats
// Do not instantiate this class directly. Rather, call
// vtkFreeTypeTools::GetInstance() to ensure that the singleton design is
// correctly applied.
// Be aware that FontConfig lookup is disabled by default. To enable, call
// vtkFreeTypeTools::GetInstance()->ForceCompiledFontsOff();

#ifndef __vtkFontConfigFreeTypeTools_h
#define __vtkFontConfigFreeTypeTools_h

#include "vtkRenderingFreeTypeFontConfigModule.h" // For export macro
#include "vtkFreeTypeTools.h"

class VTKRENDERINGFREETYPEFONTCONFIG_EXPORT vtkFontConfigFreeTypeTools:
    public vtkFreeTypeTools
{
public:
  vtkTypeMacro(vtkFontConfigFreeTypeTools, vtkFreeTypeTools);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a new object of this type, but it is not preferred to use this
  // method directly. Instead, call vtkFreeTypeTools::GetInstance() and let
  // the object factory create a new instance. In this way the singleton
  // pattern of vtkFreeTypeTools is preserved.
  static vtkFontConfigFreeTypeTools *New();

  // Description:
  // Modified version of vtkFreeTypeTools::LookupFace that locates FontConfig
  // faces. Falls back to the Superclass method for compiled fonts if the
  // FontConfig lookup fails.
  static bool LookupFaceFontConfig(vtkTextProperty *tprop, FT_Library lib,
                                   FT_Face *face);

protected:
  vtkFontConfigFreeTypeTools();
  ~vtkFontConfigFreeTypeTools();

  // Description:
  // Reimplemented from Superclass to use the FontConfig face lookup callback.
  FT_Error CreateFTCManager();

private:
  vtkFontConfigFreeTypeTools(const vtkFontConfigFreeTypeTools &); // Not implemented.
  void operator=(const vtkFontConfigFreeTypeTools &);   // Not implemented.
};

#endif //__vtkFontConfigFreeTypeTools_h
