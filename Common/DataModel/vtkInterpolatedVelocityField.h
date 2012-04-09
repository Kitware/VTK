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
// .NAME vtkInterpolatedVelocityField - A concrete class for obtaining
//  the interpolated velocity values at a point.
//
// .SECTION Description
//  vtkInterpolatedVelocityField acts as a continuous velocity field via
//  cell interpolation on a vtkDataSet, NumberOfIndependentVariables = 4
//  (x,y,z,t) and NumberOfFunctions = 3 (u,v,w). As a concrete sub-class 
//  of vtkAbstractInterpolatedVelocityField, this class adopts two levels 
//  of cell caching for faster though less robust cell location than its 
//  sibling class vtkCellLocatorInterpolatedVelocityField. Level #0 begins 
//  with intra-cell caching. Specifically, if the previous cell is valid 
//  and the nex point is still within it, ( vtkCell::EvaluatePosition() 
//  returns 1, coupled with the new parametric coordinates and weights ), 
//  the function values are interpolated and vtkCell::EvaluatePosition() 
//  is invoked only. If it fails, level #1 follows by inter-cell location 
//  of the target cell (that contains the next point). By inter-cell, the 
//  previous cell gives an important clue / guess or serves as an immediate
//  neighbor to aid in the location of the target cell (as is typically the 
//  case with integrating a streamline across cells) by means of vtkDataSet::
//  FindCell(). If this still fails, a global cell search is invoked via 
//  vtkDataSet::FindCell(). 
//  
//  Regardless of inter-cell or global search, vtkPointLocator is employed 
//  as a crucial tool underlying the cell locator. The use of vtkPointLocator
//  casues vtkInterpolatedVelocityField to return false target cells for 
//  datasets defined on complex grids.
//
// .SECTION Caveats
//  vtkInterpolatedVelocityField is not thread safe. A new instance should be
//  created by each thread.

// .SECTION See Also
//  vtkAbstractInterpolatedVelocityField vtkCellLocatorInterpolatedVelocityField
//  vtkGenericInterpolatedVelocityField vtkCachingInterpolatedVelocityField
//  vtkTemporalInterpolatedVelocityField vtkFunctionSet vtkStreamer vtkStreamTracer

#ifndef __vtkInterpolatedVelocityField_h
#define __vtkInterpolatedVelocityField_h

#include "vtkAbstractInterpolatedVelocityField.h"

class VTK_FILTERING_EXPORT vtkInterpolatedVelocityField 
  : public vtkAbstractInterpolatedVelocityField
{
public:
  vtkTypeMacro( vtkInterpolatedVelocityField,
                        vtkAbstractInterpolatedVelocityField );
  void PrintSelf( ostream & os, vtkIndent indent );

  // Description:
  // Construct a vtkInterpolatedVelocityField without an initial dataset.
  // Caching is set on and LastCellId is set to -1.
  static vtkInterpolatedVelocityField * New();
  
  // Description:
  // Add a dataset used for the implicit function evaluation. If more than
  // one dataset is added, the evaluation point is searched in all until a 
  // match is found. THIS FUNCTION DOES NOT CHANGE THE REFERENCE COUNT OF 
  // DATASET FOR THREAD SAFETY REASONS.
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
  vtkInterpolatedVelocityField() { }
  ~vtkInterpolatedVelocityField() { }

  // Description:
  // Evaluate the velocity field f at point (x, y, z) in a specified dataset
  // by either involving vtkPointLocator, via vtkPointSet::FindCell(), in
  // locating the next cell (for datasets of type vtkPointSet) or simply
  // invoking vtkImageData/vtkRectilinearGrid::FindCell() to fulfill the same
  // task if the point is outside the current cell.
  virtual int FunctionValues( vtkDataSet * ds, double * x, double * f )
    { return this->Superclass::FunctionValues( ds, x, f ); }

private:
  vtkInterpolatedVelocityField
    ( const vtkInterpolatedVelocityField & );  // Not implemented.
  void operator = 
    ( const vtkInterpolatedVelocityField & );  // Not implemented.
};

#endif
