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

/**
 * @class   vtkMathTextFreeTypeTextRenderer
 * @brief   Default implementation of
 * vtkTextRenderer.
 *
 *
 * Default implementation of vtkTextRenderer using vtkFreeTypeTools and
 * vtkMathTextUtilities.
 *
 * @warning
 * The MathText backend does not currently support UTF16 strings, thus
 * UTF16 strings passed to the MathText renderer will be converted to
 * UTF8.
*/

#ifndef vtkMathTextFreeTypeTextRenderer_h
#define vtkMathTextFreeTypeTextRenderer_h

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

  //@{
  /**
   * Test for availability of various backends
   */
  virtual bool FreeTypeIsSupported();
  virtual bool MathTextIsSupported();
  //@}

protected:
  vtkMathTextFreeTypeTextRenderer();
  ~vtkMathTextFreeTypeTextRenderer();

  //@{
  /**
   * Reimplemented from vtkTextRenderer.
   */
  bool GetBoundingBoxInternal(vtkTextProperty *tprop, const vtkStdString &str,
                              int bbox[4], int dpi, int backend);
  bool GetBoundingBoxInternal(vtkTextProperty *tprop,
                              const vtkUnicodeString &str,
                              int bbox[4], int dpi, int backend);
  bool GetMetricsInternal(vtkTextProperty *tprop, const vtkStdString &str,
                          Metrics &metrics, int dpi, int backend);
  bool GetMetricsInternal(vtkTextProperty *tprop, const vtkUnicodeString &str,
                          Metrics &metrics, int dpi, int backend);
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
                            vtkPath *path, int dpi, int backend);
  bool StringToPathInternal(vtkTextProperty *tprop, const vtkUnicodeString &str,
                            vtkPath *path, int dpi, int backend);
  void SetScaleToPowerOfTwoInternal(bool scale);
  //@}

private:
  vtkMathTextFreeTypeTextRenderer(const vtkMathTextFreeTypeTextRenderer &) VTK_DELETE_FUNCTION;
  void operator=(const vtkMathTextFreeTypeTextRenderer &) VTK_DELETE_FUNCTION;

  vtkFreeTypeTools *FreeTypeTools;
  vtkMathTextUtilities *MathTextUtilities;
};

#endif //vtkMathTextFreeTypeTextRenderer_h
