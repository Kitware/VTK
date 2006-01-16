// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGradientFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkGradientFilter - A general filter for gradient estimation.
//
// .SECTION Description
// Estimates the gradient of a scalar field in a data set.  This class
// is basically designed for unstructured data sets (i.e.
// vtkUnstructuredGrid).  More efficient filters exist for vtkImageData.
//

#ifndef _vtkGradientFilter_h
#define _vtkGradientFilter_h

#include "vtkDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkGradientFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkGradientFilter, vtkDataSetAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkGradientFilter *New();

  // Description:
  // These are basically a convenience method that calls SetInputArrayToProcess
  // to set the array used as the input scalars.  The fieldAssociation comes
  // from the vtkDataObject::FieldAssocations enum.  The fieldAttributeType
  // comes from the vtkDataSetAttributes::AttributeTypes enum.
  virtual void SetInputScalars(int fieldAssociation, const char *name);
  virtual void SetInputScalars(int fieldAssociation, int fieldAttributeType);

  // Description:
  // Get/Set the name of the resulting array to create.  If NULL (the
  // default) then the output array will be named "Gradients".
  vtkGetStringMacro(ResultArrayName);
  vtkSetStringMacro(ResultArrayName);

protected:
  vtkGradientFilter();
  ~vtkGradientFilter();

  virtual int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  char *ResultArrayName;

private:
  vtkGradientFilter(const vtkGradientFilter &); // Not implemented
  void operator=(const vtkGradientFilter &);    // Not implemented
};

#endif //_vtkGradientFilter_h
