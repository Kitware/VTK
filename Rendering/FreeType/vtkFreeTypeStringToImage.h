// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkFreeTypeStringToImage
 * @brief   render the supplied text to an image.
 *
 */

#ifndef vtkFreeTypeStringToImage_h
#define vtkFreeTypeStringToImage_h

#include "vtkRenderingFreeTypeModule.h" // For export macro
#include "vtkSmartPointer.h"            // For SP ivars
#include "vtkStringToImage.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGFREETYPE_EXPORT vtkFreeTypeStringToImage : public vtkStringToImage
{
public:
  vtkTypeMacro(vtkFreeTypeStringToImage, vtkStringToImage);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkFreeTypeStringToImage* New();

  ///@{
  /**
   * Given a text property and a string, get the bounding box [xmin, xmax] x
   * [ymin, ymax]. Note that this is the bounding box of the area
   * where actual pixels will be written, given a text/pen/baseline location
   * of (0,0).
   * For example, if the string starts with a 'space', or depending on the
   * orientation, you can end up with a [-20, -10] x [5, 10] bbox (the math
   * to get the real bbox is straightforward).
   * Return 1 on success, 0 otherwise.
   * You can use IsBoundingBoxValid() to test if the computed bbox
   * is valid (it may not if GetBoundingBox() failed or if the string
   * was empty).
   */
  vtkVector2i GetBounds(vtkTextProperty* property, const vtkStdString& string, int dpi) override;
  ///@}

  ///@{
  /**
   * Given a text property and a string, this function initializes the
   * vtkImageData *data and renders it in a vtkImageData. textDims, if provided,
   * will be overwritten by the pixel width and height of the rendered string.
   * This is useful when ScaleToPowerOfTwo is true, and the image dimensions may
   * not match the dimensions of the rendered text.
   */
  int RenderString(vtkTextProperty* property, const vtkStdString& string, int dpi,
    vtkImageData* data, int textDims[2] = nullptr) override;
  ///@}

  /**
   * Should we produce images at powers of 2, makes rendering on old OpenGL
   * hardware easier. Default is false.
   */
  void SetScaleToPowerOfTwo(bool scale) override;

  /**
   * Make a deep copy of the supplied utility class.
   */
  void DeepCopy(vtkFreeTypeStringToImage* utility);

protected:
  vtkFreeTypeStringToImage();
  ~vtkFreeTypeStringToImage() override;

  class Internals;
  Internals* Implementation;

private:
  vtkFreeTypeStringToImage(const vtkFreeTypeStringToImage&) = delete;
  void operator=(const vtkFreeTypeStringToImage&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkFreeTypeStringToImage_h
