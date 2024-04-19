// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

VTK_ABI_NAMESPACE_BEGIN
class vtkFreeTypeTools;
class vtkMathTextUtilities;

class VTKRENDERINGFREETYPE_EXPORT vtkMathTextFreeTypeTextRenderer : public vtkTextRenderer
{
public:
  vtkTypeMacro(vtkMathTextFreeTypeTextRenderer, vtkTextRenderer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkMathTextFreeTypeTextRenderer* New();

  ///@{
  /**
   * Test for availability of various backends
   */
  bool FreeTypeIsSupported() override;
  bool MathTextIsSupported() override;
  ///@}

protected:
  vtkMathTextFreeTypeTextRenderer();
  ~vtkMathTextFreeTypeTextRenderer() override;

  ///@{
  /**
   * Reimplemented from vtkTextRenderer.
   */
  bool GetBoundingBoxInternal(
    vtkTextProperty* tprop, const vtkStdString& str, int bbox[4], int dpi, int backend) override;
  bool GetMetricsInternal(vtkTextProperty* tprop, const vtkStdString& str, Metrics& metrics,
    int dpi, int backend) override;
  bool RenderStringInternal(vtkTextProperty* tprop, const vtkStdString& str, vtkImageData* data,
    int textDims[2], int dpi, int backend) override;
  int GetConstrainedFontSizeInternal(const vtkStdString& str, vtkTextProperty* tprop,
    int targetWidth, int targetHeight, int dpi, int backend) override;
  bool StringToPathInternal(
    vtkTextProperty* tprop, const vtkStdString& str, vtkPath* path, int dpi, int backend) override;
  void SetScaleToPowerOfTwoInternal(bool scale) override;
  ///@}

private:
  vtkMathTextFreeTypeTextRenderer(const vtkMathTextFreeTypeTextRenderer&) = delete;
  void operator=(const vtkMathTextFreeTypeTextRenderer&) = delete;

  vtkFreeTypeTools* FreeTypeTools;
  vtkMathTextUtilities* MathTextUtilities;
};

VTK_ABI_NAMESPACE_END
#endif // vtkMathTextFreeTypeTextRenderer_h
