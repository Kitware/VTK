/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCheckerboard.h
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
// .NAME vtkImageCheckerboard - show two images at once using a checkboard pattern
// .SECTION Description
//  vtkImageCheckerboard displays two images as one using a checkerboard
//  pattern. This filter can be used to compare two images. The
//  checkerboard pattern is controlled by the NumberOfDivisions
//  ivar. This controls the number of checkerboard divisions in the whole
//  extent of the image.

#ifndef __vtkImageCheckerboard_h
#define __vtkImageCheckerboard_h

#include "vtkImageTwoInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageCheckerboard : public vtkImageTwoInputFilter
{
public:
  static vtkImageCheckerboard *New();
  vtkTypeRevisionMacro(vtkImageCheckerboard,vtkImageTwoInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the number of divisions along each axis.
  vtkSetVector3Macro(NumberOfDivisions,int);
  vtkGetVectorMacro(NumberOfDivisions,int,3);

protected:
  vtkImageCheckerboard();
  ~vtkImageCheckerboard() {};

  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
                       int extent[6], int id);
  int NumberOfDivisions[3];
private:
  vtkImageCheckerboard(const vtkImageCheckerboard&);  // Not implemented.
  void operator=(const vtkImageCheckerboard&);  // Not implemented.
};

#endif













