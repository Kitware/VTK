/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNonMergingPointLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNonMergingPointLocator - direct / check-free point insertion.
//
// .SECTION Description
//  As a special sub-class of vtkPointLocator, vtkNonMergingPointLocator is
//  intended for direct / check-free insertion of points into a vtkPoints 
//  object. In other words, any given point is always directly inserted. 
//  The name emphasizes the difference between this class and its sibling 
//  class vtkMergePoints in that the latter class performs check-based zero 
//  tolerance point insertion (or to 'merge' exactly duplicate / coincident
//  points) by exploiting the uniform bin mechanism employed by the parent 
//  class vtkPointLocator. vtkPointLocator allows for generic (zero and non-
//  zero) tolerance point insertion as well as point location.
//
// .SECTION See Also
//  vtkIncrementalPointLocator vtkPointLocator vtkMergePoints

#ifndef __vtkNonMergingPointLocator_h
#define __vtkNonMergingPointLocator_h

#include "vtkPointLocator.h"

class vtkPoints;

class VTK_FILTERING_EXPORT vtkNonMergingPointLocator : public vtkPointLocator
{
public:
  static vtkNonMergingPointLocator * New();
  
  vtkTypeMacro( vtkNonMergingPointLocator, vtkPointLocator );
  void PrintSelf( ostream & os, vtkIndent indent );

//BTX
  // Description:
  // Determine whether a given point x has been inserted into the points list.
  // Return the id of the already inserted point if it is true, or -1 else.
  // Note this function always returns -1 since any point is always inserted.
  virtual vtkIdType IsInsertedPoint( const double [3] ) { return -1; }
  virtual vtkIdType IsInsertedPoint( double, double, double ) { return -1; }
//ETX

  // Description:
  // Determine whether a given point x has been inserted into the points list.
  // Return 0 if a duplicate has been inserted in the list, or 1 else. Note
  // this function always returns 1 since any point is always inserted. The 
  // index of the point is returned via ptId.
  virtual int InsertUniquePoint( const double x[3], vtkIdType & ptId );
  
protected:
  vtkNonMergingPointLocator() { };
  ~vtkNonMergingPointLocator() { };
  
private:
  vtkNonMergingPointLocator( const vtkNonMergingPointLocator & );  // Not implemented.
  void operator = ( const vtkNonMergingPointLocator & );  // Not implemented.
};

#endif


