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

/**
 * @class   vtkImageItem
 * @brief   a vtkContextItem that draws a supplied image in the
 * scene.
 *
 *
 * This vtkContextItem draws the supplied image in the scene.
*/

#ifndef vtkImageItem_h
#define vtkImageItem_h

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

  /**
   * Paint event for the item.
   */
  virtual bool Paint(vtkContext2D *painter);

  /**
   * Set the image of the item.
   */
  void SetImage(vtkImageData *image);

  //@{
  /**
   * Get the image of the item.
   */
  vtkGetObjectMacro(Image, vtkImageData);
  //@}

  //@{
  /**
   * Set the position of the bottom corner of the image.
   */
  vtkSetVector2Macro(Position, float);
  //@}

  //@{
  /**
   * Get the position of the bottom corner of the image.
   */
  vtkGetVector2Macro(Position, float);
  //@}

protected:
  vtkImageItem();
  ~vtkImageItem();

  float Position[2];

  vtkImageData *Image;

private:
  vtkImageItem(const vtkImageItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageItem &) VTK_DELETE_FUNCTION;

};

#endif //vtkImageItem_h
