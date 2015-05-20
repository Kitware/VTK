/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextRendererStringToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTextRendererStringToImage - uses vtkTextRenderer to render the
// supplied text to an image.

#ifndef vtkTextRendererStringToImage_h
#define vtkTextRendererStringToImage_h

#include "vtkRenderingFreeTypeModule.h" // For export macro
#include "vtkStringToImage.h"

class VTKRENDERINGFREETYPE_EXPORT vtkTextRendererStringToImage :
    public vtkStringToImage
{
public:
  vtkTypeMacro(vtkTextRendererStringToImage, vtkStringToImage);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkTextRendererStringToImage *New();

  // Description:
  // Given a text property and a string, get the bounding box [xmin, xmax] x
  // [ymin, ymax]. Note that this is the bounding box of the area
  // where actual pixels will be written, given a text/pen/baseline location
  // of (0,0).
  // For example, if the string starts with a 'space', or depending on the
  // orientation, you can end up with a [-20, -10] x [5, 10] bbox (the math
  // to get the real bbox is straightforward).
  // Return 1 on success, 0 otherwise.
  // You can use IsBoundingBoxValid() to test if the computed bbox
  // is valid (it may not if GetBoundingBox() failed or if the string
  // was empty).
  virtual vtkVector2i GetBounds(vtkTextProperty *property,
                                const vtkUnicodeString& string,
                                int dpi);
  virtual vtkVector2i GetBounds(vtkTextProperty *property,
                                const vtkStdString& string,
                                int dpi);

  // Description:
  // Given a text property and a string, this function initializes the
  // vtkImageData *data and renders it in a vtkImageData. textDims, if provided,
  // will be overwritten by the pixel width and height of the rendered string.
  // This is useful when ScaleToPowerOfTwo is true, and the image dimensions may
  // not match the dimensions of the rendered text.
  virtual int RenderString(vtkTextProperty *property,
                           const vtkUnicodeString& string,
                           int dpi,
                           vtkImageData *data,
                           int textDims[2] = NULL);
  virtual int RenderString(vtkTextProperty *property,
                           const vtkStdString& string,
                           int dpi,
                           vtkImageData *data,
                           int textDims[2] = NULL);

  // Description:
  // Should we produce images at powers of 2, makes rendering on old OpenGL
  // hardware easier. Default is false.
  virtual void SetScaleToPowerOfTwo(bool scale);

  // Description:
  // Make a deep copy of the supplied utility class.
  void DeepCopy(vtkTextRendererStringToImage *utility);

protected:
  vtkTextRendererStringToImage();
  ~vtkTextRendererStringToImage();

  class Internals;
  Internals* Implementation;

private:
  vtkTextRendererStringToImage(const vtkTextRendererStringToImage &); // Not implemented.
  void operator=(const vtkTextRendererStringToImage &);   // Not implemented.
};

#endif //vtkTextRendererStringToImage_h
