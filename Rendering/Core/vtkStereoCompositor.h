// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkStereoCompositor
 * @brief helper class to generate composited stereo images.
 *
 * vtkStereoCompositor is used by vtkRenderWindow to composite left and right
 * eye rendering results into a single color buffer.
 *
 * Note that all methods on vtkStereoCompositor take in pointers to the left and
 * right rendering results and generate the result in the buffer passed for the
 * left eye.
 */

#ifndef vtkStereoCompositor_h
#define vtkStereoCompositor_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkUnsignedCharArray;

class VTKRENDERINGCORE_EXPORT vtkStereoCompositor : public vtkObject
{
public:
  static vtkStereoCompositor* New();
  vtkTypeMacro(vtkStereoCompositor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Methods for compositing left and right eye images based on various
   * supported modes. See vtkRenderWindow::SetStereoType for explanation of each
   * of these modes. Note that all these methods generate the result in the
   * buffer passed for the left eye.
   */
  bool RedBlue(vtkUnsignedCharArray* rgbLeftNResult, vtkUnsignedCharArray* rgbRight);

  bool Anaglyph(vtkUnsignedCharArray* rgbLeftNResult, vtkUnsignedCharArray* rgbRight,
    float colorSaturation, const int colorMask[2]);

  bool Interlaced(
    vtkUnsignedCharArray* rgbLeftNResult, vtkUnsignedCharArray* rgbRight, const int size[2]);

  bool Dresden(
    vtkUnsignedCharArray* rgbLeftNResult, vtkUnsignedCharArray* rgbRight, const int size[2]);

  bool Checkerboard(
    vtkUnsignedCharArray* rgbLeftNResult, vtkUnsignedCharArray* rgbRight, const int size[2]);

  bool SplitViewportHorizontal(
    vtkUnsignedCharArray* rgbLeftNResult, vtkUnsignedCharArray* rgbRight, const int size[2]);
  ///@}

protected:
  vtkStereoCompositor();
  ~vtkStereoCompositor() override;

  bool Validate(
    vtkUnsignedCharArray* rgbLeft, vtkUnsignedCharArray* rgbRight, const int* size = nullptr);

private:
  vtkStereoCompositor(const vtkStereoCompositor&) = delete;
  void operator=(const vtkStereoCompositor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
