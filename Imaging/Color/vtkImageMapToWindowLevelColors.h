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
// .NAME vtkImageMapToWindowLevelColors - map the input image through a lookup table and window / level it
// .SECTION Description
// The vtkImageMapToWindowLevelColors filter will take an input image of any
// valid scalar type, and map the first component of the image through a
// lookup table.  This resulting color will be modulated with value obtained
// by a window / level operation. The result is an image of type 
// VTK_UNSIGNED_CHAR. If the lookup table is not set, or is set to NULL, then 
// the input data will be passed through if it is already of type 
// UNSIGNED_CHAR.
//
// .SECTION See Also
// vtkLookupTable vtkScalarsToColors

#ifndef __vtkImageMapToWindowLevelColors_h
#define __vtkImageMapToWindowLevelColors_h


#include "vtkImageMapToColors.h"

class VTK_IMAGING_EXPORT vtkImageMapToWindowLevelColors : public vtkImageMapToColors
{
public:
  static vtkImageMapToWindowLevelColors *New();
  vtkTypeMacro(vtkImageMapToWindowLevelColors,vtkImageMapToColors);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / Get the Window to use -> modulation will be performed on the 
  // color based on (S - (L - W/2))/W where S is the scalar value, L is
  // the level and W is the window.
  vtkSetMacro( Window, double );
  vtkGetMacro( Window, double );
  
  // Description:
  // Set / Get the Level to use -> modulation will be performed on the 
  // color based on (S - (L - W/2))/W where S is the scalar value, L is
  // the level and W is the window.
  vtkSetMacro( Level, double );
  vtkGetMacro( Level, double );
  
protected:
  vtkImageMapToWindowLevelColors();
  ~vtkImageMapToWindowLevelColors();

  virtual int RequestInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id);
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  double Window;
  double Level;
  
private:
  vtkImageMapToWindowLevelColors(const vtkImageMapToWindowLevelColors&);  // Not implemented.
  void operator=(const vtkImageMapToWindowLevelColors&);  // Not implemented.
};

#endif







