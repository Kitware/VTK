/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToImageFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetToImageFilter - abstract filter class
// .SECTION Description
// vtkDataSetToImageFilter is an abstract filter class whose subclasses take
// as input any dataset and generate image data on output.

// .SECTION See Also
// vtkGaussianSplatter vtkImplicitModeller vtkShepardMethod vtkVoxelModeller

#ifndef __vtkDataSetToImageFilter_h
#define __vtkDataSetToImageFilter_h

#include "vtkImageSource.h"

class vtkDataSet;

class VTK_FILTERING_EXPORT vtkDataSetToImageFilter : public vtkImageSource
{
public:
  vtkTypeMacro(vtkDataSetToImageFilter,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkDataSet *input);
  vtkDataSet *GetInput();
  
protected:
  vtkDataSetToImageFilter();
  ~vtkDataSetToImageFilter();

  // All the DataSetToImageFilters require all their input.
  void ComputeInputUpdateExtents(vtkDataObject *output);

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkDataSetToImageFilter(const vtkDataSetToImageFilter&);  // Not implemented.
  void operator=(const vtkDataSetToImageFilter&);  // Not implemented.
};

#endif





