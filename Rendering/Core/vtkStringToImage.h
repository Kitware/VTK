// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkStringToImage
 * @brief   base class for classes that render supplied text
 * to an image.
 *
 *
 *
 */

#ifndef vtkStringToImage_h
#define vtkStringToImage_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkStdString;
class vtkTextProperty;
class vtkImageData;
class vtkVector2i;

class VTKRENDERINGCORE_EXPORT vtkStringToImage : public vtkObject
{
public:
  vtkTypeMacro(vtkStringToImage, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  virtual vtkVector2i GetBounds(vtkTextProperty* property, const vtkStdString& string, int dpi) = 0;
  ///@}

  ///@{
  /**
   * Given a text property and a string, this function initializes the
   * vtkImageData *data and renders it in a vtkImageData. textDims, if provided,
   * will be overwritten by the pixel width and height of the rendered string.
   * This is useful when ScaleToPowerOfTwo is true, and the image dimensions may
   * not match the dimensions of the rendered text.
   */
  virtual int RenderString(vtkTextProperty* property, const vtkStdString& string, int dpi,
    vtkImageData* data, int text_dims[2] = nullptr) = 0;
  ///@}

  /**
   * Should we produce images at powers of 2, makes rendering on old OpenGL
   * hardware easier. Default is false.
   */
  virtual void SetScaleToPowerOfTwo(bool scale);
  vtkGetMacro(ScaleToPowerOfTwo, bool);

protected:
  vtkStringToImage();
  ~vtkStringToImage() override;

  bool Antialias;
  bool ScaleToPowerOfTwo;

private:
  vtkStringToImage(const vtkStringToImage&) = delete;
  void operator=(const vtkStringToImage&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkStringToImage_h
