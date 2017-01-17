/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToWindowLevelColors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageMapToWindowLevelColors
 * @brief   map the input image through a lookup table and window / level it
 *
 * The vtkImageMapToWindowLevelColors filter will take an input image of any
 * valid scalar type, and map the first component of the image through a
 * lookup table.  This resulting color will be modulated with value obtained
 * by a window / level operation. The result is an image of type
 * VTK_UNSIGNED_CHAR. If the lookup table is not set, or is set to NULL, then
 * the input data will be passed through if it is already of type
 * VTK_UNSIGNED_CHAR.
 *
 * @sa
 * vtkLookupTable vtkScalarsToColors
*/

#ifndef vtkImageMapToWindowLevelColors_h
#define vtkImageMapToWindowLevelColors_h


#include "vtkImagingColorModule.h" // For export macro
#include "vtkImageMapToColors.h"

class VTKIMAGINGCOLOR_EXPORT vtkImageMapToWindowLevelColors : public vtkImageMapToColors
{
public:
  static vtkImageMapToWindowLevelColors *New();
  vtkTypeMacro(vtkImageMapToWindowLevelColors,vtkImageMapToColors);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set / Get the Window to use -> modulation will be performed on the
   * color based on (S - (L - W/2))/W where S is the scalar value, L is
   * the level and W is the window.
   */
  vtkSetMacro( Window, double );
  vtkGetMacro( Window, double );
  //@}

  //@{
  /**
   * Set / Get the Level to use -> modulation will be performed on the
   * color based on (S - (L - W/2))/W where S is the scalar value, L is
   * the level and W is the window.
   */
  vtkSetMacro( Level, double );
  vtkGetMacro( Level, double );
  //@}

protected:
  vtkImageMapToWindowLevelColors();
  ~vtkImageMapToWindowLevelColors() VTK_OVERRIDE;

  int RequestInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id) VTK_OVERRIDE;
  int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) VTK_OVERRIDE;

  double Window;
  double Level;

private:
  vtkImageMapToWindowLevelColors(const vtkImageMapToWindowLevelColors&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageMapToWindowLevelColors&) VTK_DELETE_FUNCTION;
};

#endif







