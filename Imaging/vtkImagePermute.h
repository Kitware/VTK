/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePermute.h
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
// .NAME vtkImagePermute -  Permutes axes of input.
// .SECTION Description
// vtkImagePermute reorders the axes of the input. Filtered axes specify
// the input axes which become X, Y, Z.  The input has to have the
// same scalar type of the output. The filter does copy the 
// data when it executes. 

#ifndef __vtkImagePermute_h
#define __vtkImagePermute_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImagePermute : public vtkImageToImageFilter
{
public:
  static vtkImagePermute *New();
  vtkTypeRevisionMacro(vtkImagePermute,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The filtered axes are the input axes that get relabeled to X,Y,Z.
  vtkSetVector3Macro(FilteredAxes, int);
  vtkGetVector3Macro(FilteredAxes, int);
  
protected:
  vtkImagePermute();
  ~vtkImagePermute() {};

  int  FilteredAxes[3];
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
                       int ext[6], int id);
private:
  vtkImagePermute(const vtkImagePermute&);  // Not implemented.
  void operator=(const vtkImagePermute&);  // Not implemented.
};

#endif



