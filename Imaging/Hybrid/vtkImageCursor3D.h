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
/**
 * @class   vtkImageCursor3D
 * @brief   Paints a cursor on top of an image or volume.
 *
 * vtkImageCursor3D will draw a cursor on a 2d image or 3d volume.
*/

#ifndef vtkImageCursor3D_h
#define vtkImageCursor3D_h

#include "vtkImagingHybridModule.h" // For export macro
#include "vtkImageInPlaceFilter.h"

class VTKIMAGINGHYBRID_EXPORT vtkImageCursor3D : public vtkImageInPlaceFilter
{
public:
  static vtkImageCursor3D *New();
  vtkTypeMacro(vtkImageCursor3D,vtkImageInPlaceFilter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;


  //@{
  /**
   * Sets/Gets the center point of the 3d cursor.
   */
  vtkSetVector3Macro(CursorPosition, double);
  vtkGetVector3Macro(CursorPosition, double);
  //@}

  //@{
  /**
   * Sets/Gets what pixel value to draw the cursor in.
   */
  vtkSetMacro(CursorValue, double);
  vtkGetMacro(CursorValue, double);
  //@}

  //@{
  /**
   * Sets/Gets the radius of the cursor. The radius determines
   * how far the axis lines project out from the cursors center.
   */
  vtkSetMacro(CursorRadius, int);
  vtkGetMacro(CursorRadius, int);
  //@}


protected:
  vtkImageCursor3D();
  ~vtkImageCursor3D() VTK_OVERRIDE {}

  double CursorPosition[3];
  double CursorValue;
  int CursorRadius;

  int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) VTK_OVERRIDE;

private:
  vtkImageCursor3D(const vtkImageCursor3D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageCursor3D&) VTK_DELETE_FUNCTION;
};



#endif



