/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingExistingLayoutStrategy.h
  
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkTreeRingExistingLayoutStrategy - lays out a tree using
//  using concentric rings
//
// .SECTION Description
// vtkTreeRingExistingLayoutStrategy partitions the space for child vertices
// circular sectors. Sectors are sized based on the subtended angles from a
// previously run radial tree layout.
//
// .SECTION Thanks to Jason Shepherd for this implementation.

#ifndef __vtkTreeRingExistingLayoutStrategy_h
#define __vtkTreeRingExistingLayoutStrategy_h

#include "vtkTreeRingLayoutStrategy.h"

class vtkIdList;

class VTK_INFOVIS_EXPORT vtkTreeRingExistingLayoutStrategy : public vtkTreeRingLayoutStrategy 
{
public:
  static vtkTreeRingExistingLayoutStrategy *New();

  vtkTypeRevisionMacro(vtkTreeRingExistingLayoutStrategy,vtkTreeRingLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The field name associated with the size of the vertex.
//   vtkGetStringMacro(SizeFieldName);
//   vtkSetStringMacro(SizeFieldName);

  // Description:
  // Perform the layout of a tree and place the results as 4-tuples in
  // coordsArray (innerRadius, outerRadius, startAngle, endAngle).
  void Layout(vtkTree *inputTree, vtkDataArray *coordsArray);

protected:
  vtkTreeRingExistingLayoutStrategy();
  ~vtkTreeRingExistingLayoutStrategy();

private:

//  char * SizeFieldName;

  void SetInteriorSubtendedAngles( vtkTree* tree, vtkIdType parent, vtkDataArray*& anglesArray, 
                                   double& min_angle, double& max_angle );
  
  void LayoutChildren( vtkTree *tree, vtkDataArray *coordsArray, vtkDataArray *anglesArray,
                       vtkIdType nchildren, vtkIdType parent, float parentInnerRad );

  vtkTreeRingExistingLayoutStrategy(const vtkTreeRingExistingLayoutStrategy&);  // Not implemented.
  void operator=(const vtkTreeRingExistingLayoutStrategy&);  // Not implemented.
};

#endif

