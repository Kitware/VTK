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
// This vtkContextItem draws the supplied image in the scene. Optionally showing
// the label as a tooltip on mouse over.

#ifndef __vtkImageItem_h
#define __vtkImageItem_h

#include "vtkContextItem.h"

class vtkContext2D;
class vtkImageData;

class VTK_CHARTS_EXPORT vtkImageItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkImageItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkImageItem *New();

  // Description:
  // Paint event for the item.
  virtual bool Paint(vtkContext2D *painter);

//BTX
  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse enter event.
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse leave event.
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button down event.
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);
//ETX

  // Description:
  // Set the mouse over label for the item.
  vtkSetStringMacro(Label);

  // Description:
  // Get the mouse over label for the item.
  vtkGetStringMacro(Label);

  // Description:
  // Set the image of the item.
  void SetImage(vtkImageData *image);

  // Description:
  // Get the image of the item.
  vtkGetObjectMacro(Image, vtkImageData);

  // Description:
  // Set the dimensions of the item, bottom corner, width, height.
  vtkSetVector4Macro(Dimensions, int);

  // Description:
  // Get the dimensions of the item, bottom corner, width, height.
  vtkGetVector4Macro(Dimensions, int);

//BTX
  void SetScalarFunctor(double (*scalarFunction)(double, double));
//ETX

//BTX
protected:
  vtkImageItem();
  ~vtkImageItem();

  int Dimensions[4];

  int LastPosition[2];

  char *Label;
  vtkImageData *Image;

  bool MouseOver;
  int MouseButtonPressed;

  // Some function pointers to optionally do funky things...
  double (*ScalarFunction)(double, double);

private:
  vtkImageItem(const vtkImageItem &); // Not implemented.
  void operator=(const vtkImageItem &);   // Not implemented.
//ETX
};

#endif //__vtkImageItem_h
