/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkImageItem - a vtkContextItem that draws a supplied image in the
// scene.
//
// .SECTION Description
// This vtkContextItem draws the supplied image in the scene.

#ifndef __vtkImageItem_h
#define __vtkImageItem_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkContextItem.h"
#include "vtkSmartPointer.h" // For SP ivars.

class vtkImageData;

class VTKRENDERINGCONTEXT2D_EXPORT vtkImageItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkImageItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkImageItem *New();

  // Description:
  // Paint event for the item.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Set the image of the item.
  void SetImage(vtkImageData *image);

  // Description:
  // Get the image of the item.
  vtkGetObjectMacro(Image, vtkImageData);

  // Description:
  // Set the position of the bottom corner of the image.
  vtkSetVector2Macro(Position, float);

  // Description:
  // Get the position of the bottom corner of the image.
  vtkGetVector2Macro(Position, float);

//BTX
protected:
  vtkImageItem();
  ~vtkImageItem();

  float Position[2];

  vtkImageData *Image;

private:
  vtkImageItem(const vtkImageItem &); // Not implemented.
  void operator=(const vtkImageItem &);   // Not implemented.
//ETX
};

#endif //__vtkImageItem_h
