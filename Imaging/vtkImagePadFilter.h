/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePadFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImagePadFilter - Super class for filters that fill in extra pixels.
// .SECTION Description
// vtkImagePadFilter Changes the image extent of an image.  If the image
// extent is larger than the input image extent, the extra pixels are
// filled by an algorithm determined by the subclass.
// The image extent of the output has to be specified.


#ifndef __vtkImagePadFilter_h
#define __vtkImagePadFilter_h

#include "vtkImageAlgorithm.h"

class VTK_IMAGING_EXPORT vtkImagePadFilter : public vtkImageAlgorithm
{
public:
  static vtkImagePadFilter *New();
  vtkTypeRevisionMacro(vtkImagePadFilter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The image extent of the output has to be set explicitly.
  void SetOutputWholeExtent(int extent[6]);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY, 
                            int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int *GetOutputWholeExtent() {return this->OutputWholeExtent;}

  // Description:
  // Set/Get the number of output scalar components.
  vtkSetMacro(OutputNumberOfScalarComponents, int);
  vtkGetMacro(OutputNumberOfScalarComponents, int);
  
protected:
  vtkImagePadFilter();
  ~vtkImagePadFilter() {};

  int OutputWholeExtent[6];
  int OutputNumberOfScalarComponents;

  void ExecuteInformation(vtkInformation *,
                          vtkInformationVector *,
                          vtkInformationVector *);
  void ComputeInputUpdateExtent(vtkInformation *,
                                vtkInformationVector *,
                                vtkInformationVector *);
  
private:
  vtkImagePadFilter(const vtkImagePadFilter&);  // Not implemented.
  void operator=(const vtkImagePadFilter&);  // Not implemented.
};

#endif



