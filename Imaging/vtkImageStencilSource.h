/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilSource.h
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
// .NAME vtkImageStencilSource - helper class for clipping images
// .SECTION Description
// vtkImageStencilSource is a helper class for vtkImageToImageFilter
// classes.  Given a clipping object such as a vtkImplicitFunction, it
// will set up a list of clipping extents for each x-row through the
// image data.  The extents for each x-row can be retrieved via the 
// GetNextExtent() method after the extent lists have been built
// with the BuildExtents() method.  For large images, using clipping
// extents is much more memory efficient (and slightly more time-efficient)
// than building a mask.  This class can be subclassed to allow clipping
// with objects other than vtkImplicitFunction.
// .SECTION see also
// vtkImplicitFunction vtkImageStencil vtkImagePolyDataStencilSource

#ifndef __vtkImageStencilSource_h
#define __vtkImageStencilSource_h


#include "vtkSource.h"
#include "vtkImageStencilData.h"

class VTK_IMAGING_EXPORT vtkImageStencilSource : public vtkSource
{
public:
  static vtkImageStencilSource *New();
  vtkTypeRevisionMacro(vtkImageStencilSource, vtkSource);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get or set the output for this source.
  void SetOutput(vtkImageStencilData *output);
  vtkImageStencilData *GetOutput();

protected:
  vtkImageStencilSource();
  ~vtkImageStencilSource();

  void ExecuteData(vtkDataObject *out);
  vtkImageStencilData *AllocateOutputData(vtkDataObject *out);

  // Description:
  // Override this method to support clipping with different kinds
  // of objects.  Eventually the extent could be split up and handled
  // by multiple threads, but it isn't for now.  But please ensure
  // that all code inside this method is thread-safe.
  virtual void ThreadedExecute(vtkImageStencilData *output,
                               int extent[6], int threadId);
private:
  vtkImageStencilSource(const vtkImageStencilSource&);  // Not implemented.
  void operator=(const vtkImageStencilSource&);  // Not implemented.
};

#endif
