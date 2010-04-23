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
// .NAME vtkImageStencilSource - generate an image stencil
// .SECTION Description
// vtkImageStencilSource is a superclass for filters that generate image
// stencils.  Given a clipping object such as a vtkImplicitFunction, it
// will set up a list of clipping extents for each x-row through the
// image data.  The extents for each x-row can be retrieved via the 
// GetNextExtent() method after the extent lists have been built
// with the BuildExtents() method.  For large images, using clipping
// extents is much more memory efficient (and slightly more time-efficient)
// than building a mask.  This class can be subclassed to allow clipping
// with objects other than vtkImplicitFunction.
// .SECTION see also
// vtkImplicitFunction vtkImageStencil vtkPolyDataToImageStencil

#ifndef __vtkImageStencilSource_h
#define __vtkImageStencilSource_h


#include "vtkAlgorithm.h"

class vtkImageStencilData;

class VTK_IMAGING_EXPORT vtkImageStencilSource : public vtkAlgorithm
{
public:
  static vtkImageStencilSource *New();
  vtkTypeMacro(vtkImageStencilSource, vtkAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get or set the output for this source.
  void SetOutput(vtkImageStencilData *output);
  vtkImageStencilData *GetOutput();

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

protected:
  vtkImageStencilSource();
  ~vtkImageStencilSource();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *) { return 1; }
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *) { return 1; }
  vtkImageStencilData *AllocateOutputData(vtkDataObject *out, int* updateExt);

  virtual int FillOutputPortInformation(int, vtkInformation*);

private:
  vtkImageStencilSource(const vtkImageStencilSource&);  // Not implemented.
  void operator=(const vtkImageStencilSource&);  // Not implemented.
};

#endif

