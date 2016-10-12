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

/**
 * @class   vtkFontConfigFreeTypeTools
 * @brief   Subclass of vtkFreeTypeTools that uses
 * system installed fonts.
 *
 *
 * vtkFontConfigFreeTypeTools defers to vtkFreeTypeTools for rendering and
 * rasterization, but sources fonts from a FontConfig system lookup. If the
 * lookup fails, the compiled fonts of vtkFreeType are used instead.
 *
 * @warning
 * Do not instantiate this class directly. Rather, call
 * vtkFreeTypeTools::GetInstance() to ensure that the singleton design is
 * correctly applied.
 * Be aware that FontConfig lookup is disabled by default. To enable, call
 * vtkFreeTypeTools::GetInstance()->ForceCompiledFontsOff();
*/

#ifndef vtkFontConfigFreeTypeTools_h
#define vtkFontConfigFreeTypeTools_h

#include "vtkRenderingFreeTypeFontConfigModule.h" // For export macro
#include "vtkFreeTypeTools.h"

class VTKRENDERINGFREETYPEFONTCONFIG_EXPORT vtkFontConfigFreeTypeTools:
    public vtkFreeTypeTools
{
public:
  vtkTypeMacro(vtkFontConfigFreeTypeTools, vtkFreeTypeTools);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Creates a new object of this type, but it is not preferred to use this
   * method directly. Instead, call vtkFreeTypeTools::GetInstance() and let
   * the object factory create a new instance. In this way the singleton
   * pattern of vtkFreeTypeTools is preserved.
   */
  static vtkFontConfigFreeTypeTools *New();

  /**
   * Modified version of vtkFreeTypeTools::LookupFace that locates FontConfig
   * faces. Falls back to the Superclass method for compiled fonts if the
   * FontConfig lookup fails.
   */
  static bool LookupFaceFontConfig(vtkTextProperty *tprop, FT_Library lib,
                                   FT_Face *face);

protected:
  vtkFontConfigFreeTypeTools();
  ~vtkFontConfigFreeTypeTools();

  /**
   * Reimplemented from Superclass to use the FontConfig face lookup callback.
   */
  FT_Error CreateFTCManager();

private:
  vtkFontConfigFreeTypeTools(const vtkFontConfigFreeTypeTools &) VTK_DELETE_FUNCTION;
  void operator=(const vtkFontConfigFreeTypeTools &) VTK_DELETE_FUNCTION;
};

#endif //vtkFontConfigFreeTypeTools_h
