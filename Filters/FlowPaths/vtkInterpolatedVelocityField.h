/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInterpolatedVelocityField
 * @brief   A concrete class for obtaining
 *  the interpolated velocity values at a point.
 *
 *
 *  vtkInterpolatedVelocityField acts as a continuous velocity field via
 *  cell interpolation on a vtkDataSet, NumberOfIndependentVariables = 4
 *  (x,y,z,t) and NumberOfFunctions = 3 (u,v,w). As a concrete sub-class
 *  of vtkCompositeInterpolatedVelocityField, this class adopts two levels
 *  of cell caching for faster though less robust cell location than its
 *  sibling class vtkCellLocatorInterpolatedVelocityField. Level #0 begins
 *  with intra-cell caching. Specifically, if the previous cell is valid
 *  and the nex point is still within it, ( vtkCell::EvaluatePosition()
 *  returns 1, coupled with the new parametric coordinates and weights ),
 *  the function values are interpolated and vtkCell::EvaluatePosition()
 *  is invoked only. If it fails, level #1 follows by inter-cell location
 *  of the target cell (that contains the next point). By inter-cell, the
 *  previous cell gives an important clue / guess or serves as an immediate
 *  neighbor to aid in the location of the target cell (as is typically the
 *  case with integrating a streamline across cells) by means of vtkDataSet::
 *  FindCell(). If this still fails, a global cell search is invoked via
 *  vtkDataSet::FindCell().
 *
 *  Regardless of inter-cell or global search, vtkPointLocator is employed
 *  as a crucial tool underlying the cell locator. The use of vtkPointLocator
 *  causes vtkInterpolatedVelocityField to return false target cells for
 *  datasets defined on complex grids.
 *
 * @warning
 *  vtkInterpolatedVelocityField is not thread safe. A new instance should be
 *  created by each thread.
 *
 * @sa
 *  vtkCompositeInterpolatedVelocityField vtkCellLocatorInterpolatedVelocityField
 *  vtkGenericInterpolatedVelocityField vtkCachingInterpolatedVelocityField
 *  vtkTemporalInterpolatedVelocityField vtkFunctionSet vtkStreamTracer
*/

#ifndef vtkInterpolatedVelocityField_h
#define vtkInterpolatedVelocityField_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkCompositeInterpolatedVelocityField.h"

class VTKFILTERSFLOWPATHS_EXPORT vtkInterpolatedVelocityField
  : public vtkCompositeInterpolatedVelocityField
{
public:
  vtkTypeMacro( vtkInterpolatedVelocityField,
                        vtkCompositeInterpolatedVelocityField );
  void PrintSelf( ostream & os, vtkIndent indent ) VTK_OVERRIDE;

  /**
   * Construct a vtkInterpolatedVelocityField without an initial dataset.
   * Caching is set on and LastCellId is set to -1.
   */
  static vtkInterpolatedVelocityField * New();

  /**
   * Add a dataset used for the implicit function evaluation. If more than
   * one dataset is added, the evaluation point is searched in all until a
   * match is found. THIS FUNCTION DOES NOT CHANGE THE REFERENCE COUNT OF
   * DATASET FOR THREAD SAFETY REASONS.
   */
  void AddDataSet( vtkDataSet * dataset ) VTK_OVERRIDE;

  /**
   * Evaluate the velocity field f at point (x, y, z).
   */
  int FunctionValues( double * x, double * f ) VTK_OVERRIDE;

  /**
   * Project the provided point on current cell, current dataset.
   */
  virtual int SnapPointOnCell(double* pOrigin, double* pProj);

  /**
   * Set the cell id cached by the last evaluation within a specified dataset.
   */
  void SetLastCellId( vtkIdType c, int dataindex ) VTK_OVERRIDE;

  /**
   * Set the cell id cached by the last evaluation.
   */
  void SetLastCellId( vtkIdType c ) VTK_OVERRIDE
    { this->Superclass::SetLastCellId( c ); }

protected:
  vtkInterpolatedVelocityField() { }
  ~vtkInterpolatedVelocityField() VTK_OVERRIDE { }

  /**
   * Evaluate the velocity field f at point (x, y, z) in a specified dataset
   * by either involving vtkPointLocator, via vtkPointSet::FindCell(), in
   * locating the next cell (for datasets of type vtkPointSet) or simply
   * invoking vtkImageData/vtkRectilinearGrid::FindCell() to fulfill the same
   * task if the point is outside the current cell.
   */
  int FunctionValues( vtkDataSet * ds, double * x, double * f ) VTK_OVERRIDE
    { return this->Superclass::FunctionValues( ds, x, f ); }

private:
  vtkInterpolatedVelocityField
    ( const vtkInterpolatedVelocityField & ) VTK_DELETE_FUNCTION;
  void operator =
    ( const vtkInterpolatedVelocityField & ) VTK_DELETE_FUNCTION;
};

#endif
