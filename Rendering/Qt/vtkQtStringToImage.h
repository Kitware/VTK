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

/**
 * @class   vtkQtStringToImage
 * @brief   uses Qt to render the supplied text to an image.
 *
 *
 *
*/

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
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  static vtkQtStringToImage *New();

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
  vtkVector2i GetBounds(vtkTextProperty *property,
                                const vtkUnicodeString& string, int dpi) VTK_OVERRIDE;
  vtkVector2i GetBounds(vtkTextProperty *property,
                                const vtkStdString& string, int dpi) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Given a text property and a string, this function initializes the
   * vtkImageData *data and renders it in a vtkImageData. textDims, if provided,
   * will be overwritten by the pixel width and height of the rendered string.
   */
  int RenderString(vtkTextProperty *property,
                           const vtkUnicodeString& string, int dpi,
                           vtkImageData *data,
                           int textDims[2] = NULL) VTK_OVERRIDE;
  int RenderString(vtkTextProperty *property,
                           const vtkStdString& string, int dpi,
                           vtkImageData *data,
                           int textDims[2] = NULL) VTK_OVERRIDE;
  //@}

  /**
   * Make a deep copy of the supplied utility class.
   */
  void DeepCopy(vtkQtStringToImage *utility);

protected:
  vtkQtStringToImage();
  ~vtkQtStringToImage() VTK_OVERRIDE;

  class Internals;
  Internals* Implementation;

  vtkSmartPointer<vtkQImageToImageSource> QImageToImage;

private:
  vtkQtStringToImage(const vtkQtStringToImage &) VTK_DELETE_FUNCTION;
  void operator=(const vtkQtStringToImage &) VTK_DELETE_FUNCTION;
};

#endif //vtkQtStringToImage_h
