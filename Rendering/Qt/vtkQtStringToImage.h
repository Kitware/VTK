/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtStringToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkQtStringToImage - uses Qt to render the supplied text to an image.
//
// .SECTION Description
//

#ifndef vtkQtStringToImage_h
#define vtkQtStringToImage_h

#include "vtkRenderingQtModule.h" // For export macro
#include "vtkStringToImage.h"
#include "vtkSmartPointer.h" // For SP ivars

class vtkQImageToImageSource;

class VTKRENDERINGQT_EXPORT vtkQtStringToImage : public vtkStringToImage
{
public:
  vtkTypeMacro(vtkQtStringToImage, vtkStringToImage);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkQtStringToImage *New();

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
                                const vtkUnicodeString& string);
  virtual vtkVector2i GetBounds(vtkTextProperty *property,
                                const vtkStdString& string);

  // Description:
  // Given a text property and a string, this function initializes the
  // vtkImageData *data and renders it in a vtkImageData. textDims, if provided,
  // will be overwritten by the pixel width and height of the rendered string.
  virtual int RenderString(vtkTextProperty *property,
                           const vtkUnicodeString& string,
                           vtkImageData *data,
                           int textDims[2] = NULL);
  virtual int RenderString(vtkTextProperty *property,
                           const vtkStdString& string,
                           vtkImageData *data,
                           int textDims[2] = NULL);

  // Description:
  // Make a deep copy of the supplied utility class.
  void DeepCopy(vtkQtStringToImage *utility);

protected:
  vtkQtStringToImage();
  ~vtkQtStringToImage();

  class Internals;
  Internals* Implementation;

  vtkSmartPointer<vtkQImageToImageSource> QImageToImage;

private:
  vtkQtStringToImage(const vtkQtStringToImage &); // Not implemented.
  void operator=(const vtkQtStringToImage &);   // Not implemented.
};

#endif //vtkQtStringToImage_h
