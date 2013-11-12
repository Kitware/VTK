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

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCORE_EXPORT vtkImagePadFilter : public vtkThreadedImageAlgorithm
{
public:
  static vtkImagePadFilter *New();
  vtkTypeMacro(vtkImagePadFilter,vtkThreadedImageAlgorithm);
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
  ~vtkImagePadFilter() {}

  int OutputWholeExtent[6];
  int OutputNumberOfScalarComponents;

  virtual int RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  virtual void ComputeInputUpdateExtent (int inExt[6], int outExt[6],
                                         int wExt[6]);

private:
  vtkImagePadFilter(const vtkImagePadFilter&);  // Not implemented.
  void operator=(const vtkImagePadFilter&);  // Not implemented.
};

#endif



