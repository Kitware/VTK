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
// .NAME vtkCellLocatorInterpolatedVelocityField - A concrete class for
//  obtaining the interpolated velocity values at a point.
//
// .SECTION Description
//  vtkCellLocatorInterpolatedVelocityField acts as a continuous velocity
//  field via cell interpolation on a vtkDataSet, NumberOfIndependentVariables
//  = 4 (x,y,z,t) and NumberOfFunctions = 3 (u,v,w). As a concrete sub-class
//  of vtkAbstractInterpolatedVelocityField, it adopts vtkAbstractCellLocator's
//  sub-classes, e.g., vtkCellLocator and vtkModifiedBSPTree, without the use
//  of vtkPointLocator ( employed by vtkDataSet/vtkPointSet::FindCell() in
//  vtkInterpolatedVelocityField ). vtkCellLocatorInterpolatedVelocityField
//  adopts one level of cell caching. Specifically, if the next point is still
//  within the previous cell, cell location is then simply skipped and vtkCell::
//  EvaluatePosition() is called to obtain the new parametric coordinates and
//  weights that are used to interpolate the velocity function values across the
//  vertices of this cell. Otherwise a global cell (the target containing the next
//  point) location is instead directly invoked, without exploiting the clue that
//  vtkInterpolatedVelocityField makes use of from the previous cell (an immediate
//  neighbor). Although ignoring the neighbor cell may incur a relatively high
//  computational cost, vtkCellLocatorInterpolatedVelocityField is more robust in
//  locating the target cell than its sibling class vtkInterpolatedVelocityField.

// .SECTION Caveats
//  vtkCellLocatorInterpolatedVelocityField is not thread safe. A new instance
//  should be created by each thread.

// .SECTION See Also
//  vtkAbstractInterpolatedVelocityField vtkInterpolatedVelocityField
//  vtkGenericInterpolatedVelocityField vtkCachingInterpolatedVelocityField
//  vtkTemporalInterpolatedVelocityField vtkFunctionSet vtkStreamer vtkStreamTracer

#ifndef __vtkCellLocatorInterpolatedVelocityField_h
#define __vtkCellLocatorInterpolatedVelocityField_h

#include "vtkAbstractInterpolatedVelocityField.h"

class vtkAbstractCellLocator;
class vtkCellLocatorInterpolatedVelocityFieldCellLocatorsType;

class VTK_GRAPHICS_EXPORT vtkCellLocatorInterpolatedVelocityField : public vtkAbstractInterpolatedVelocityField
{
public:
  vtkTypeMacro( vtkCellLocatorInterpolatedVelocityField,
                        vtkAbstractInterpolatedVelocityField );
  void PrintSelf( ostream & os, vtkIndent indent );

  // Description:
  // Construct a vtkCellLocatorInterpolatedVelocityField without an initial
  // dataset. Caching is set on and LastCellId is set to -1.
  static vtkCellLocatorInterpolatedVelocityField * New();

  // Description:
  // Get the cell locator attached to the most recently visited dataset.
  vtkGetObjectMacro( LastCellLocator, vtkAbstractCellLocator );

  // Description:
  // Get the prototype of the cell locator that is used for interpolating the
  // velocity field during integration.
  vtkGetObjectMacro( CellLocatorPrototype, vtkAbstractCellLocator );

  // Description:
  // Set a prototype of the cell locator that is used for interpolating the
  // velocity field during integration.
  void SetCellLocatorPrototype( vtkAbstractCellLocator * prototype );

  // Description:
  // Import parameters. Sub-classes can add more after chaining.
  virtual void CopyParameters( vtkAbstractInterpolatedVelocityField * from );

  // Description:
  // Add a dataset coupled with a cell locator (of type vtkAbstractCellLocator)
  // for vector function evaluation. Note the use of a vtkAbstractCellLocator
  // enables robust cell location. If more than one dataset is added, the
  // evaluation point is searched in all until a match is found. THIS FUNCTION
  // DOES NOT CHANGE THE REFERENCE COUNT OF dataset FOR THREAD SAFETY REASONS.
  virtual void AddDataSet( vtkDataSet * dataset );

  // Description:
  // Evaluate the velocity field f at point (x, y, z).
  virtual int FunctionValues( double * x, double * f );

  // Description:
  // Set the cell id cached by the last evaluation within a specified dataset.
  virtual void SetLastCellId( vtkIdType c, int dataindex );

  // Description:
  // Set the cell id cached by the last evaluation.
  virtual void SetLastCellId( vtkIdType c )
    { this->Superclass::SetLastCellId( c ); }

protected:
  vtkCellLocatorInterpolatedVelocityField();
  ~vtkCellLocatorInterpolatedVelocityField();

  // Description:
  // Evaluate the velocity field f at point (x, y, z) in a specified dataset
  // (actually of type vtkPointSet only) through the use of the associated
  // vtkAbstractCellLocator::FindCell() (instead of involving vtkPointLocator)
  // to locate the next cell if the given point is outside the current cell.
  int FunctionValues( vtkDataSet * ds, vtkAbstractCellLocator * loc,
                      double * x, double * f );

  // Description:
  // Evaluate the velocity field f at point (x, y, z) in a specified dataset
  // (of type vtkImageData or vtkRectilinearGrid only) by invoking FindCell()
  // to locate the next cell if the given point is outside the current cell.
  virtual int FunctionValues( vtkDataSet * ds, double * x, double * f )
    { return this->Superclass::FunctionValues( ds, x, f ); }

private:
  vtkAbstractCellLocator * LastCellLocator;
  vtkAbstractCellLocator * CellLocatorPrototype;
  vtkCellLocatorInterpolatedVelocityFieldCellLocatorsType * CellLocators;

  vtkCellLocatorInterpolatedVelocityField
    ( const vtkCellLocatorInterpolatedVelocityField & );  // Not implemented.
  void operator = ( const vtkCellLocatorInterpolatedVelocityField & );  // Not implemented.
};

#endif
