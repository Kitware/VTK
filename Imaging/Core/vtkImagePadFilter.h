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
/**
 * @class   vtkImagePadFilter
 * @brief   Super class for filters that fill in extra pixels.
 *
 * vtkImagePadFilter Changes the image extent of an image.  If the image
 * extent is larger than the input image extent, the extra pixels are
 * filled by an algorithm determined by the subclass.
 * The image extent of the output has to be specified.
*/

#ifndef vtkImagePadFilter_h
#define vtkImagePadFilter_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCORE_EXPORT vtkImagePadFilter : public vtkThreadedImageAlgorithm
{
public:
  static vtkImagePadFilter *New();
  vtkTypeMacro(vtkImagePadFilter,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The image extent of the output has to be set explicitly.
   */
  void SetOutputWholeExtent(int extent[6]);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY,
                            int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int *GetOutputWholeExtent() {return this->OutputWholeExtent;}
  //@}

  //@{
  /**
   * Set/Get the number of output scalar components.
   */
  vtkSetMacro(OutputNumberOfScalarComponents, int);
  vtkGetMacro(OutputNumberOfScalarComponents, int);
  //@}

protected:
  vtkImagePadFilter();
  ~vtkImagePadFilter() VTK_OVERRIDE {}

  int OutputWholeExtent[6];
  int OutputNumberOfScalarComponents;

  int RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*) VTK_OVERRIDE;
  int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*) VTK_OVERRIDE;

  virtual void ComputeInputUpdateExtent (int inExt[6], int outExt[6],
                                         int wExt[6]);

private:
  vtkImagePadFilter(const vtkImagePadFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImagePadFilter&) VTK_DELETE_FUNCTION;
};

#endif



