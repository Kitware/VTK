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

// .NAME vtkStringToImage - uses Qt to render the supplied text to an image.
//
// .SECTION Description
//

#ifndef __vtkStringToImage_h
#define __vtkStringToImage_h

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
                                const vtkUnicodeString& string) = 0;
  virtual vtkVector2i GetBounds(vtkTextProperty *property,
                                const vtkStdString& string) = 0;

  // Description:
  // Given a text property and a string, this function initializes the
  // vtkImageData *data and renders it in a vtkImageData.
  virtual int RenderString(vtkTextProperty *property,
                           const vtkUnicodeString& string,
                           vtkImageData *data) = 0;
  virtual int RenderString(vtkTextProperty *property,
                           const vtkStdString& string,
                           vtkImageData *data) = 0;

  // Description:
  // Should we produce images at powers of 2, makes rendering on old OpenGL
  // hardware easier. Default is false.
  virtual void SetScaleToPowerOfTwo(bool scale);
  vtkGetMacro(ScaleToPowerOfTwo, bool)

protected:
  vtkStringToImage();
  ~vtkStringToImage();

  bool Antialias;
  bool ScaleToPowerOfTwo;

private:
  vtkStringToImage(const vtkStringToImage &); // Not implemented.
  void operator=(const vtkStringToImage &);   // Not implemented.
};

#endif //__vtkStringToImage_h
