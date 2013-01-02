/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMathTextFreeTypeTextRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMathTextFreeTypeTextRenderer - Default implementation of
// vtkTextRenderer.
//
// .SECTION Description
// Default implementation of vtkTextRenderer using vtkFreeTypeTools and
// vtkMathTextUtilities.
//
// .SECTION CAVEATS
// The MathText backend does not currently support UTF16 strings, thus
// UTF16 strings passed to the MathText renderer will be converted to
// UTF8.

#ifndef __vtkMathTextFreeTypeTextRenderer_h
#define __vtkMathTextFreeTypeTextRenderer_h

#include "vtkRenderingFreeTypeModule.h" // For export macro
#include "vtkTextRenderer.h"

class vtkFreeTypeTools;
class vtkMathTextUtilities;

class VTKRENDERINGFREETYPE_EXPORT vtkMathTextFreeTypeTextRenderer :
    public vtkTextRenderer
{
public:
  vtkTypeMacro(vtkMathTextFreeTypeTextRenderer, vtkTextRenderer)
  void PrintSelf(ostream &os, vtkIndent indent);

  static vtkMathTextFreeTypeTextRenderer *New();

protected:
  vtkMathTextFreeTypeTextRenderer();
  ~vtkMathTextFreeTypeTextRenderer();

  // Description:
  // Reimplemented from vtkTextRenderer.
  bool GetBoundingBoxInternal(vtkTextProperty *tprop, const vtkStdString &str,
                              int bbox[4], int dpi, int backend);
  bool GetBoundingBoxInternal(vtkTextProperty *tprop,
                              const vtkUnicodeString &str,
                              int bbox[4], int dpi, int backend);
  bool RenderStringInternal(vtkTextProperty *tprop, const vtkStdString &str,
                            vtkImageData *data, int textDims[2], int dpi,
                            int backend);
  bool RenderStringInternal(vtkTextProperty *tprop, const vtkUnicodeString &str,
                            vtkImageData *data, int textDims[2], int dpi,
                            int backend);
  int GetConstrainedFontSizeInternal(const vtkStdString &str,
                                     vtkTextProperty *tprop,
                                     int targetWidth, int targetHeight, int dpi,
                                     int backend);
  int GetConstrainedFontSizeInternal(const vtkUnicodeString &str,
                                     vtkTextProperty *tprop,
                                     int targetWidth, int targetHeight, int dpi,
                                     int backend);
  bool StringToPathInternal(vtkTextProperty *tprop, const vtkStdString &str,
                            vtkPath *path, int backend);
  bool StringToPathInternal(vtkTextProperty *tprop, const vtkUnicodeString &str,
                            vtkPath *path, int backend);
  void SetScaleToPowerOfTwoInternal(bool scale);

private:
  vtkMathTextFreeTypeTextRenderer(const vtkMathTextFreeTypeTextRenderer &); // Not implemented.
  void operator=(const vtkMathTextFreeTypeTextRenderer &); // Not implemented.

  vtkFreeTypeTools *FreeTypeTools;
  vtkMathTextUtilities *MathTextUtilities;
};

#endif //__vtkMathTextFreeTypeTextRenderer_h
