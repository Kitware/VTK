/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeInterpolatedVelocityField - An abstract class for
//  obtaining the interpolated velocity values at a point
//
// .SECTION Description
//  vtkCompositeInterpolatedVelocityField acts as a continuous velocity field
//  by performing cell interpolation on the underlying vtkDataSet. This is an
//  abstract sub-class of vtkFunctionSet, NumberOfIndependentVariables = 4
//  (x,y,z,t) and NumberOfFunctions = 3 (u,v,w). With a brute-force scheme,
//  every time an evaluation is performed, the target cell containing point
//  (x,y,z) needs to be found by calling FindCell(), via either vtkDataSet or
//  vtkAbstractCelllocator's sub-classes (vtkCellLocator & vtkModifiedBSPTree).
//  As it incurs a large cost, one (for vtkCellLocatorInterpolatedVelocityField
//  via vtkAbstractCellLocator) or two (for vtkInterpolatedVelocityField via
//  vtkDataSet that involves vtkPointLocator in addressing vtkPointSet) levels
//  of cell caching may be exploited to increase the performance.
//
//  For vtkInterpolatedVelocityField, level #0 begins with intra-cell caching.
//  Specifically if the previous cell is valid and the next point is still in
//  it ( i.e., vtkCell::EvaluatePosition() returns 1, coupled with newly created
//  parametric coordinates & weights ), the function values can be interpolated
//  and only vtkCell::EvaluatePosition() is invoked. If this fails, then level #1
//  follows by inter-cell search for the target cell that contains the next point.
//  By an inter-cell search, the previous cell provides an important clue or serves
//  as an immediate neighbor to aid in locating the target cell via vtkPointSet::
//  FindCell(). If this still fails, a global cell location / search is invoked via
//  vtkPointSet::FindCell(). Here regardless of either inter-cell or global search,
//  vtkPointLocator is in fact employed (for datasets of type vtkPointSet only, note
//  vtkImageData and vtkRectilinearGrid are able to provide rapid and robust cell
//  location due to the simple mesh topology) as a crucial tool underlying the cell
//  locator. However, the use of vtkPointLocator makes vtkInterpolatedVelocityField
//  non-robust in cell location for vtkPointSet.
//
//  For vtkCellLocatorInterpolatedVelocityField, the only caching (level #0) works
//  by intra-cell trial. In case of failure, a global search for the target cell is
//  invoked via vtkAbstractCellLocator::FindCell() and the actual work is done by
//  either vtkCellLocator or vtkModifiedBSPTree (for datasets of type vtkPointSet
//  only, while vtkImageData and vtkRectilinearGrid themselves are able to provide
//  fast robust cell location). Without the involvement of vtkPointLocator, robust
//  cell location is achieved for vtkPointSet.
//
// .SECTION Caveats
//  vtkCompositeInterpolatedVelocityField is not thread safe. A new instance
//  should be created by each thread.

// .SECTION See Also
//  vtkInterpolatedVelocityField vtkCellLocatorInterpolatedVelocityField
//  vtkGenericInterpolatedVelocityField vtkCachingInterpolatedVelocityField
//  vtkTemporalInterpolatedVelocityField vtkFunctionSet vtkStreamer vtkStreamTracer

#ifndef vtkCompositeInterpolatedVelocityField_h
#define vtkCompositeInterpolatedVelocityField_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkAbstractInterpolatedVelocityField.h"

//BTX
#include <vector> // STL Header; Required for vector
//ETX

class vtkDataSet;
//BTX
class vtkDataArray;
//ETX
class vtkPointData;
class vtkGenericCell;
class vtkCompositeInterpolatedVelocityFieldDataSetsType;

class VTKFILTERSFLOWPATHS_EXPORT vtkCompositeInterpolatedVelocityField : public vtkAbstractInterpolatedVelocityField
{
public:
  vtkTypeMacro( vtkCompositeInterpolatedVelocityField, vtkAbstractInterpolatedVelocityField);
  void PrintSelf( ostream & os, vtkIndent indent );

  // Description:
  // Get the most recently visited dataset and it id. The dataset is used
  // for a guess regarding where the next point will be, without searching
  // through all datasets. When setting the last dataset, care is needed as
  // no reference counting or checks are performed. This feature is intended
  // for custom interpolators only that cache datasets independently.
  vtkGetMacro( LastDataSetIndex, int );
  vtkGetObjectMacro( LastDataSet, vtkDataSet );


  // Description:
  // Add a dataset for implicit velocity function evaluation. If more than
  // one dataset is added, the evaluation point is searched in all until a
  // match is found. THIS FUNCTION DOES NOT CHANGE THE REFERENCE COUNT OF
  // dataset FOR THREAD SAFETY REASONS.
  virtual void AddDataSet( vtkDataSet * dataset ) = 0;


protected:
  vtkCompositeInterpolatedVelocityField();
  ~vtkCompositeInterpolatedVelocityField();

  static const double TOLERANCE_SCALE;


  int       LastDataSetIndex;
  vtkCompositeInterpolatedVelocityFieldDataSetsType * DataSets;

private:
  vtkCompositeInterpolatedVelocityField
    ( const vtkCompositeInterpolatedVelocityField & );  // Not implemented.
  void operator = ( const vtkCompositeInterpolatedVelocityField & );  // Not implemented.
};

//BTX
typedef std::vector< vtkDataSet * > DataSetsTypeBase;
class   vtkCompositeInterpolatedVelocityFieldDataSetsType: public DataSetsTypeBase { };
//ETX

#endif
