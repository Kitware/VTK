/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCorrelation.h
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
// .NAME vtkImageCorrelation - Correlation imageof the two inputs.
// .SECTION Description
// vtkImageCorrelation finds the correlation between two data sets. 
// SetDimensionality determines
// whether the Correlation will be 3D, 2D or 1D.  
// The default is a 2D Correlation.  The Output type will be float.
// The output size will match the size of the first input.
// The second input is considered the correlation kernel.

#ifndef __vtkImageCorrelation_h
#define __vtkImageCorrelation_h



#include "vtkImageTwoInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageCorrelation : public vtkImageTwoInputFilter
{
public:
  static vtkImageCorrelation *New();
  vtkTypeRevisionMacro(vtkImageCorrelation,vtkImageTwoInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Determines how the input is interpreted (set of 2d slices ...)
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);
  
protected:
  vtkImageCorrelation();
  ~vtkImageCorrelation() {};

  int Dimensionality;
  void ExecuteInformation(vtkImageData **inDatas, vtkImageData *outData);
  virtual void ComputeInputUpdateExtent(int inExt[6], int outExt[6],
                                        int whichInput);
  void ExecuteInformation(){this->vtkImageTwoInputFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageCorrelation(const vtkImageCorrelation&);  // Not implemented.
  void operator=(const vtkImageCorrelation&);  // Not implemented.
};

#endif



