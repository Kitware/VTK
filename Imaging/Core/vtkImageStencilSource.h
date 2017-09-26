/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageStencilSource
 * @brief   generate an image stencil
 *
 * vtkImageStencilSource is a superclass for filters that generate image
 * stencils.  Given a clipping object such as a vtkImplicitFunction, it
 * will set up a list of clipping extents for each x-row through the
 * image data.  The extents for each x-row can be retrieved via the
 * GetNextExtent() method after the extent lists have been built
 * with the BuildExtents() method.  For large images, using clipping
 * extents is much more memory efficient (and slightly more time-efficient)
 * than building a mask.  This class can be subclassed to allow clipping
 * with objects other than vtkImplicitFunction.
 * @sa
 * vtkImplicitFunction vtkImageStencil vtkPolyDataToImageStencil
*/

#ifndef vtkImageStencilSource_h
#define vtkImageStencilSource_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkImageStencilAlgorithm.h"

class vtkImageStencilData;
class vtkImageData;

class VTKIMAGINGCORE_EXPORT vtkImageStencilSource :
  public vtkImageStencilAlgorithm
{
public:
  static vtkImageStencilSource *New();
  vtkTypeMacro(vtkImageStencilSource, vtkImageStencilAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set a vtkImageData that has the Spacing, Origin, and
   * WholeExtent that will be used for the stencil.  This
   * input should be set to the image that you wish to
   * apply the stencil to.  If you use this method, then
   * any values set with the SetOutputSpacing, SetOutputOrigin,
   * and SetOutputWholeExtent methods will be ignored.
   */
  virtual void SetInformationInput(vtkImageData*);
  vtkGetObjectMacro(InformationInput, vtkImageData);
  //@}

  //@{
  /**
   * Set the Origin to be used for the stencil.  It should be
   * set to the Origin of the image you intend to apply the
   * stencil to. The default value is (0,0,0).
   */
  vtkSetVector3Macro(OutputOrigin, double);
  vtkGetVector3Macro(OutputOrigin, double);
  //@}

  //@{
  /**
   * Set the Spacing to be used for the stencil. It should be
   * set to the Spacing of the image you intend to apply the
   * stencil to. The default value is (1,1,1)
   */
  vtkSetVector3Macro(OutputSpacing, double);
  vtkGetVector3Macro(OutputSpacing, double);
  //@}

  //@{
  /**
   * Set the whole extent for the stencil (anything outside
   * this extent will be considered to be "outside" the stencil).
   */
  vtkSetVector6Macro(OutputWholeExtent, int);
  vtkGetVector6Macro(OutputWholeExtent, int);
  //@}

  /**
   * Report object referenced by instances of this class.
   */
  void ReportReferences(vtkGarbageCollector*) override;

protected:
  vtkImageStencilSource();
  ~vtkImageStencilSource() override;

  int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *) override;

  vtkImageData *InformationInput;

  int OutputWholeExtent[6];
  double OutputOrigin[3];
  double OutputSpacing[3];

private:
  vtkImageStencilSource(const vtkImageStencilSource&) = delete;
  void operator=(const vtkImageStencilSource&) = delete;
};

#endif

