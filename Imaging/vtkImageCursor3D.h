/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCursor3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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
  vtkTypeRevisionMacro(vtkImageCursor3D,vtkImageInPlaceFilter);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Sets/Gets the center point of the 3d cursor.
  vtkSetVector3Macro(CursorPosition, float);
  vtkGetVector3Macro(CursorPosition, float);

  // Description:
  // Sets/Gets what pixel value to draw the cursor in.
  vtkSetMacro(CursorValue, float);
  vtkGetMacro(CursorValue, float);
  
  // Description:
  // Sets/Gets the radius of the cursor. The radius determines
  // how far the axis lines project out from the cursors center.
  vtkSetMacro(CursorRadius, int);
  vtkGetMacro(CursorRadius, int);
  
  
protected:
  vtkImageCursor3D();
  ~vtkImageCursor3D() {};

  float CursorPosition[3];
  float CursorValue;
  int CursorRadius;
  
  // not threaded because it's too simple a filter
  void ExecuteData(vtkDataObject *outData);
private:
  vtkImageCursor3D(const vtkImageCursor3D&);  // Not implemented.
  void operator=(const vtkImageCursor3D&);  // Not implemented.
};



#endif



