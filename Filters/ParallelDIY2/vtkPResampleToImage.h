/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPResampleToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPResampleToImage
 * @brief   sample dataset on a uniform grid in parallel
 *
 * vtkPResampleToImage is a parallel filter that resamples the input dataset on
 * a uniform grid. It internally uses vtkProbeFilter to do the probing.
 * @sa
 * vtkResampleToImage vtkProbeFilter
*/

#ifndef vtkPResampleToImage_h
#define vtkPResampleToImage_h

#include "vtkFiltersParallelDIY2Module.h" // For export macro
#include "vtkResampleToImage.h"

class vtkDataSet;
class vtkImageData;
class vtkMultiProcessController;

class VTKFILTERSPARALLELDIY2_EXPORT vtkPResampleToImage : public vtkResampleToImage
{
public:
  vtkTypeMacro(vtkPResampleToImage, vtkResampleToImage);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkPResampleToImage *New();

  //@{
  /**
   * By defualt this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPResampleToImage();
  ~vtkPResampleToImage();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  vtkMultiProcessController *Controller;

private:
  vtkPResampleToImage(const vtkPResampleToImage&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPResampleToImage&) VTK_DELETE_FUNCTION;
};

#endif
