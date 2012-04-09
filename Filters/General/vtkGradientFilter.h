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
// Estimates the gradient of a field in a data set.  The gradient calculation
// is dependent on the input dataset type.  The created gradient array
// is of the same type as the array it is calculated from (e.g. point data
// or cell data) as well as data type (e.g. float, double).  At the boundary 
// the gradient is not central differencing.  The output array has 
// 3*number of components of the input data array.  The ordering for the 
// output tuple will be {du/dx, du/dy, du/dz, dv/dx, dv/dy, dv/dz, dw/dx,
// dw/dy, dw/dz} for an input array {u, v, w}.


#ifndef __vtkGradientFilter_h
#define __vtkGradientFilter_h

#include "vtkDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkGradientFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkGradientFilter, vtkDataSetAlgorithm);
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

  // Description:
  // When this flag is on (default is off), the gradient filter will provide a
  // less accurate (but close) algorithm that performs fewer derivative
  // calculations (and is therefore faster).  The error contains some smoothing
  // of the output data and some possible errors on the boundary.  This
  // parameter has no effect when performing the gradient of cell data.
  // This only applies if the input grid is a vtkUnstructuredGrid or a
  // vtkPolyData.
  vtkGetMacro(FasterApproximation, int);
  vtkSetMacro(FasterApproximation, int);
  vtkBooleanMacro(FasterApproximation, int);

  // Description:
  // Set the resultant array to be vorticity/curl of the input
  // array.  The input array must have 3 components.
  vtkSetMacro(ComputeVorticity, int);
  vtkGetMacro(ComputeVorticity, int);
  vtkBooleanMacro(ComputeVorticity, int);

  // Description:
  // Add Q-criterion to the output field data.  The name of the array
  // will be "Q-Criterion" and will be the same type as the input
  // array.  The input array must have 3 components in order to
  // compute this.  Note that Q-citerion is a balance of the rate
  // of vorticity and the rate of strain.
  vtkSetMacro(ComputeQCriterion, int);
  vtkGetMacro(ComputeQCriterion, int);
  vtkBooleanMacro(ComputeQCriterion, int);

protected:
  vtkGradientFilter();
  ~vtkGradientFilter();

  virtual int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  // Description:
  // Compute the gradients for grids that are not a vtkImageData, 
  // vtkRectilinearGrid, or vtkStructuredGrid.
  // Returns non-zero if the operation was successful.
  virtual int ComputeUnstructuredGridGradient(
    vtkDataArray* Array, int fieldAssociation, vtkDataSet* input,
    vtkDataSet* output);

  // Description:
  // Compute the gradients for either a vtkImageData, vtkRectilinearGrid or 
  // a vtkStructuredGrid.  Computes the gradient using finite differences.
  // Returns non-zero if the operation was successful.
  virtual int ComputeRegularGridGradient(
    vtkDataArray* Array, int fieldAssociation, vtkDataSet* output);
  
  // Description:
  // If non-null then it contains the name of the outputted gradient array
  char *ResultArrayName;

  // Description:
  // When this flag is on (default is off), the gradient filter will provide a
  // less accurate (but close) algorithm that performs fewer derivative
  // calculations (and is therefore faster).  The error contains some smoothing
  // of the output data and some possible errors on the boundary.  This
  // parameter has no effect when performing the gradient of cell data.
  // This only applies if the input grid is a vtkUnstructuredGrid or a
  // vtkPolyData.
  int FasterApproximation;

  // Description:
  // Flag to indicate that the Q-criterion of the input vector is to
  // be computed.  The input array to be processed must have
  // 3 components.  By default ComputeQCriterion is off.
  int ComputeQCriterion;

  // Description:
  // Flag to indicate that vorticity/curl of the input vector is to 
  // be computed.  The input array to be processed must have
  // 3 components.  By default ComputeVorticity is off.
  int ComputeVorticity;

private:
  vtkGradientFilter(const vtkGradientFilter &); // Not implemented
  void operator=(const vtkGradientFilter &);    // Not implemented
};

#endif //_vtkGradientFilter_h
