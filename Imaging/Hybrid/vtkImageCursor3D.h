/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCursor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageCursor3D - Paints a cursor on top of an image or volume.
// .SECTION Description
// vtkImageCursor3D will draw a cursor on a 2d image or 3d volume.

#ifndef __vtkImageCursor3D_h
#define __vtkImageCursor3D_h

#include "vtkImageInPlaceFilter.h"

class VTK_IMAGING_EXPORT vtkImageCursor3D : public vtkImageInPlaceFilter
{
public:
  static vtkImageCursor3D *New();
  vtkTypeMacro(vtkImageCursor3D,vtkImageInPlaceFilter);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Sets/Gets the center point of the 3d cursor.
  vtkSetVector3Macro(CursorPosition, double);
  vtkGetVector3Macro(CursorPosition, double);

  // Description:
  // Sets/Gets what pixel value to draw the cursor in.
  vtkSetMacro(CursorValue, double);
  vtkGetMacro(CursorValue, double);
  
  // Description:
  // Sets/Gets the radius of the cursor. The radius determines
  // how far the axis lines project out from the cursors center.
  vtkSetMacro(CursorRadius, int);
  vtkGetMacro(CursorRadius, int);
  
  
protected:
  vtkImageCursor3D();
  ~vtkImageCursor3D() {};

  double CursorPosition[3];
  double CursorValue;
  int CursorRadius;
  
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

private:
  vtkImageCursor3D(const vtkImageCursor3D&);  // Not implemented.
  void operator=(const vtkImageCursor3D&);  // Not implemented.
};



#endif



