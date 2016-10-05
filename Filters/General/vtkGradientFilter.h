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

/**
 * @class   vtkGradientFilter
 * @brief   A general filter for gradient estimation.
 *
 *
 * Estimates the gradient of a field in a data set.  The gradient calculation
 * is dependent on the input dataset type.  The created gradient array
 * is of the same type as the array it is calculated from (e.g. point data
 * or cell data) as well as data type (e.g. float, double).  At the boundary
 * the gradient is not central differencing.  The output array has
 * 3*number of components of the input data array.  The ordering for the
 * output tuple will be {du/dx, du/dy, du/dz, dv/dx, dv/dy, dv/dz, dw/dx,
 * dw/dy, dw/dz} for an input array {u, v, w}. There are also the options
 * to additionally compute the vorticity and Q criterion of a vector field.
*/

#ifndef vtkGradientFilter_h
#define vtkGradientFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkGradientFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkGradientFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  static vtkGradientFilter *New();

  //@{
  /**
   * These are basically a convenience method that calls SetInputArrayToProcess
   * to set the array used as the input scalars.  The fieldAssociation comes
   * from the vtkDataObject::FieldAssocations enum.  The fieldAttributeType
   * comes from the vtkDataSetAttributes::AttributeTypes enum.
   */
  virtual void SetInputScalars(int fieldAssociation, const char *name);
  virtual void SetInputScalars(int fieldAssociation, int fieldAttributeType);
  //@}

  //@{
  /**
   * Get/Set the name of the gradient array to create.  This is only
   * used if ComputeGradient is non-zero. If NULL (the
   * default) then the output array will be named "Gradients".
   */
  vtkGetStringMacro(ResultArrayName);
  vtkSetStringMacro(ResultArrayName);
  //@}

  //@{
  /**
   * Get/Set the name of the divergence array to create. This is only
   * used if ComputeDivergence is non-zero. If NULL (the
   * default) then the output array will be named "Divergence".
   */
  vtkGetStringMacro(DivergenceArrayName);
  vtkSetStringMacro(DivergenceArrayName);
  //@}

  //@{
  /**
   * Get/Set the name of the vorticity array to create. This is only
   * used if ComputeVorticity is non-zero. If NULL (the
   * default) then the output array will be named "Vorticity".
   */
  vtkGetStringMacro(VorticityArrayName);
  vtkSetStringMacro(VorticityArrayName);
  //@}

  //@{
  /**
   * Get/Set the name of the Q criterion array to create. This is only
   * used if ComputeQCriterion is non-zero. If NULL (the
   * default) then the output array will be named "Q-criterion".
   */
  vtkGetStringMacro(QCriterionArrayName);
  vtkSetStringMacro(QCriterionArrayName);
  //@}

 //@{
 /**
  * When this flag is on (default is off), the gradient filter will provide a
  * less accurate (but close) algorithm that performs fewer derivative
  * calculations (and is therefore faster).  The error contains some smoothing
  * of the output data and some possible errors on the boundary.  This
  * parameter has no effect when performing the gradient of cell data.
  * This only applies if the input grid is a vtkUnstructuredGrid or a
  * vtkPolyData.
  */
  vtkGetMacro(FasterApproximation, int);
  vtkSetMacro(FasterApproximation, int);
  vtkBooleanMacro(FasterApproximation, int);
 //@}

  //@{
  /**
   * Add the gradient to the output field data.  The name of the array
   * will be ResultArrayName and will be the same type as the input
   * array. The default is on.
   */
  vtkSetMacro(ComputeGradient, int);
  vtkGetMacro(ComputeGradient, int);
  vtkBooleanMacro(ComputeGradient, int);
  //@}

  //@{
  /**
   * Add divergence to the output field data.  The name of the array
   * will be DivergenceArrayName and will be the same type as the input
   * array.  The input array must have 3 components in order to
   * compute this. The default is off.
   */
  vtkSetMacro(ComputeDivergence, int);
  vtkGetMacro(ComputeDivergence, int);
  vtkBooleanMacro(ComputeDivergence, int);
  //@}

  //@{
  /**
   * Add voriticity/curl to the output field data.  The name of the array
   * will be VorticityArrayName and will be the same type as the input
   * array.  The input array must have 3 components in order to
   * compute this. The default is off.
   */
  vtkSetMacro(ComputeVorticity, int);
  vtkGetMacro(ComputeVorticity, int);
  vtkBooleanMacro(ComputeVorticity, int);
  //@}

  //@{
  /**
   * Add Q-criterion to the output field data.  The name of the array
   * will be QCriterionArrayName and will be the same type as the input
   * array.  The input array must have 3 components in order to
   * compute this.  Note that Q-citerion is a balance of the rate
   * of vorticity and the rate of strain. The default is off.
   */
  vtkSetMacro(ComputeQCriterion, int);
  vtkGetMacro(ComputeQCriterion, int);
  vtkBooleanMacro(ComputeQCriterion, int);
  //@}

protected:
  vtkGradientFilter();
  ~vtkGradientFilter() VTK_OVERRIDE;

  int RequestUpdateExtent(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

  /**
   * Compute the gradients for grids that are not a vtkImageData,
   * vtkRectilinearGrid, or vtkStructuredGrid.
   * Returns non-zero if the operation was successful.
   */
  virtual int ComputeUnstructuredGridGradient(
    vtkDataArray* Array, int fieldAssociation, vtkDataSet* input,
    bool computeVorticity, bool computeQCriterion, bool computeDivergence,
    vtkDataSet* output);

  /**
   * Compute the gradients for either a vtkImageData, vtkRectilinearGrid or
   * a vtkStructuredGrid.  Computes the gradient using finite differences.
   * Returns non-zero if the operation was successful.
   */
  virtual int ComputeRegularGridGradient(
    vtkDataArray* Array, int fieldAssociation, bool computeVorticity,
    bool computeQCriterion, bool computeDivergence, vtkDataSet* output);

  /**
   * If non-null then it contains the name of the outputted gradient array.
   * By derault it is "Gradients".
   */
  char *ResultArrayName;

  /**
   * If non-null then it contains the name of the outputted divergence array.
   * By derault it is "Divergence".
   */
  char *DivergenceArrayName;

  /**
   * If non-null then it contains the name of the outputted vorticity array.
   * By derault it is "Vorticity".
   */
  char *VorticityArrayName;

  /**
   * If non-null then it contains the name of the outputted Q criterion array.
   * By derault it is "Q-criterion".
   */
  char *QCriterionArrayName;

  /**
   * When this flag is on (default is off), the gradient filter will provide a
   * less accurate (but close) algorithm that performs fewer derivative
   * calculations (and is therefore faster).  The error contains some smoothing
   * of the output data and some possible errors on the boundary.  This
   * parameter has no effect when performing the gradient of cell data.
   * This only applies if the input grid is a vtkUnstructuredGrid or a
   * vtkPolyData.
   */
  int FasterApproximation;

  /**
   * Flag to indicate that the gradient of the input vector is to
   * be computed. By default ComputeDivergence is on.
   */
  int ComputeGradient;

  /**
   * Flag to indicate that the divergence of the input vector is to
   * be computed.  The input array to be processed must have
   * 3 components.  By default ComputeDivergence is off.
   */
  int ComputeDivergence;

  /**
   * Flag to indicate that the Q-criterion of the input vector is to
   * be computed.  The input array to be processed must have
   * 3 components.  By default ComputeQCriterion is off.
   */
  int ComputeQCriterion;

  /**
   * Flag to indicate that vorticity/curl of the input vector is to
   * be computed.  The input array to be processed must have
   * 3 components.  By default ComputeVorticity is off.
   */
  int ComputeVorticity;

private:
  vtkGradientFilter(const vtkGradientFilter &) VTK_DELETE_FUNCTION;
  void operator=(const vtkGradientFilter &) VTK_DELETE_FUNCTION;
};

#endif //_vtkGradientFilter_h
