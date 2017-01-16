/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLocatorInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellLocatorInterpolatedVelocityField
 * @brief   A concrete class for
 *  obtaining the interpolated velocity values at a point.
 *
 *
 *  vtkCellLocatorInterpolatedVelocityField acts as a continuous velocity
 *  field via cell interpolation on a vtkDataSet, NumberOfIndependentVariables
 *  = 4 (x,y,z,t) and NumberOfFunctions = 3 (u,v,w). As a concrete sub-class
 *  of vtkCompositeInterpolatedVelocityField, it adopts vtkAbstractCellLocator's
 *  sub-classes, e.g., vtkCellLocator and vtkModifiedBSPTree, without the use
 *  of vtkPointLocator ( employed by vtkDataSet/vtkPointSet::FindCell() in
 *  vtkInterpolatedVelocityField ). vtkCellLocatorInterpolatedVelocityField
 *  adopts one level of cell caching. Specifically, if the next point is still
 *  within the previous cell, cell location is then simply skipped and vtkCell::
 *  EvaluatePosition() is called to obtain the new parametric coordinates and
 *  weights that are used to interpolate the velocity function values across the
 *  vertices of this cell. Otherwise a global cell (the target containing the next
 *  point) location is instead directly invoked, without exploiting the clue that
 *  vtkInterpolatedVelocityField makes use of from the previous cell (an immediate
 *  neighbor). Although ignoring the neighbor cell may incur a relatively high
 *  computational cost, vtkCellLocatorInterpolatedVelocityField is more robust in
 *  locating the target cell than its sibling class vtkInterpolatedVelocityField.
 *
 * @warning
 *  vtkCellLocatorInterpolatedVelocityField is not thread safe. A new instance
 *  should be created by each thread.
 *
 * @sa
 *  vtkCompositeInterpolatedVelocityField vtkInterpolatedVelocityField
 *  vtkGenericInterpolatedVelocityField vtkCachingInterpolatedVelocityField
 *  vtkTemporalInterpolatedVelocityField vtkFunctionSet vtkStreamTracer
*/

#ifndef vtkCellLocatorInterpolatedVelocityField_h
#define vtkCellLocatorInterpolatedVelocityField_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkCompositeInterpolatedVelocityField.h"

class vtkAbstractCellLocator;
class vtkCellLocatorInterpolatedVelocityFieldCellLocatorsType;

class VTKFILTERSFLOWPATHS_EXPORT vtkCellLocatorInterpolatedVelocityField : public vtkCompositeInterpolatedVelocityField
{
public:
  vtkTypeMacro( vtkCellLocatorInterpolatedVelocityField,
                        vtkCompositeInterpolatedVelocityField );
  void PrintSelf( ostream & os, vtkIndent indent ) VTK_OVERRIDE;

  /**
   * Construct a vtkCellLocatorInterpolatedVelocityField without an initial
   * dataset. Caching is set on and LastCellId is set to -1.
   */
  static vtkCellLocatorInterpolatedVelocityField * New();

  //@{
  /**
   * Get the cell locator attached to the most recently visited dataset.
   */
  vtkGetObjectMacro( LastCellLocator, vtkAbstractCellLocator );
  //@}

  //@{
  /**
   * Get the prototype of the cell locator that is used for interpolating the
   * velocity field during integration.
   */
  vtkGetObjectMacro( CellLocatorPrototype, vtkAbstractCellLocator );
  //@}

  /**
   * Set a prototype of the cell locator that is used for interpolating the
   * velocity field during integration.
   */
  void SetCellLocatorPrototype( vtkAbstractCellLocator * prototype );

  /**
   * Import parameters. Sub-classes can add more after chaining.
   */
  void CopyParameters( vtkAbstractInterpolatedVelocityField * from ) VTK_OVERRIDE;
  /**
   * Add a dataset coupled with a cell locator (of type vtkAbstractCellLocator)
   * for vector function evaluation. Note the use of a vtkAbstractCellLocator
   * enables robust cell location. If more than one dataset is added, the
   * evaluation point is searched in all until a match is found. THIS FUNCTION
   * DOES NOT CHANGE THE REFERENCE COUNT OF dataset FOR THREAD SAFETY REASONS.
   */
  void AddDataSet( vtkDataSet * dataset ) VTK_OVERRIDE;

  /**
   * Evaluate the velocity field f at point (x, y, z).
   */
  int FunctionValues( double * x, double * f ) VTK_OVERRIDE;

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
  vtkCellLocatorInterpolatedVelocityField();
  ~vtkCellLocatorInterpolatedVelocityField() VTK_OVERRIDE;

  /**
   * Evaluate the velocity field f at point (x, y, z) in a specified dataset
   * (actually of type vtkPointSet only) through the use of the associated
   * vtkAbstractCellLocator::FindCell() (instead of involving vtkPointLocator)
   * to locate the next cell if the given point is outside the current cell.
   */
  int FunctionValues( vtkDataSet * ds, vtkAbstractCellLocator * loc,
                      double * x, double * f );

  /**
   * Evaluate the velocity field f at point (x, y, z) in a specified dataset
   * (of type vtkImageData or vtkRectilinearGrid only) by invoking FindCell()
   * to locate the next cell if the given point is outside the current cell.
   */
  int FunctionValues( vtkDataSet * ds, double * x, double * f ) VTK_OVERRIDE
    { return this->Superclass::FunctionValues( ds, x, f ); }

private:
  vtkAbstractCellLocator * LastCellLocator;
  vtkAbstractCellLocator * CellLocatorPrototype;
  vtkCellLocatorInterpolatedVelocityFieldCellLocatorsType * CellLocators;

  vtkCellLocatorInterpolatedVelocityField
    ( const vtkCellLocatorInterpolatedVelocityField & ) VTK_DELETE_FUNCTION;
  void operator = ( const vtkCellLocatorInterpolatedVelocityField & ) VTK_DELETE_FUNCTION;
};

#endif
