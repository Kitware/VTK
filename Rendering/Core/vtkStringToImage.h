/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkStdString;
class vtkUnicodeString;
class vtkTextProperty;
class vtkImageData;
class vtkVector2i;

class VTKRENDERINGCORE_EXPORT vtkStringToImage : public vtkObject
{
public:
  vtkTypeMacro(vtkStringToImage, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  //@{
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
  virtual vtkVector2i GetBounds(vtkTextProperty *property,
                                const vtkUnicodeString& string, int dpi) = 0;
  virtual vtkVector2i GetBounds(vtkTextProperty *property,
                                const vtkStdString& string, int dpi) = 0;
  //@}

  //@{
  /**
   * Given a text property and a string, this function initializes the
   * vtkImageData *data and renders it in a vtkImageData. textDims, if provided,
   * will be overwritten by the pixel width and height of the rendered string.
   * This is useful when ScaleToPowerOfTwo is true, and the image dimensions may
   * not match the dimensions of the rendered text.
   */
  virtual int RenderString(vtkTextProperty *property,
                           const vtkUnicodeString& string, int dpi,
                           vtkImageData *data,
                           int textDims[2] = NULL) = 0;
  virtual int RenderString(vtkTextProperty *property,
                           const vtkStdString& string, int dpi,
                           vtkImageData *data,
                           int text_dims[2] = NULL) = 0;
  //@}

  /**
   * Should we produce images at powers of 2, makes rendering on old OpenGL
   * hardware easier. Default is false.
   */
  virtual void SetScaleToPowerOfTwo(bool scale);
  vtkGetMacro(ScaleToPowerOfTwo, bool)

protected:
  vtkStringToImage();
  ~vtkStringToImage();

  bool Antialias;
  bool ScaleToPowerOfTwo;

private:
  vtkStringToImage(const vtkStringToImage &) VTK_DELETE_FUNCTION;
  void operator=(const vtkStringToImage &) VTK_DELETE_FUNCTION;
};

#endif //vtkStringToImage_h
