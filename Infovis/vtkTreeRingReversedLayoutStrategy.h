/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingReversedLayoutStrategy.h
  
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
// .NAME vtkTreeRingReversedLayoutStrategy - lays out a tree using
//  using concentric rings
//
// .SECTION Description
// vtkTreeRingReversedLayoutStrategy partitions the space for child vertices
// circular sectors. Sectors are sized based on the relative vertex size.
//
// .SECTION Thanks to Jason Shepherd for this implementation.

#ifndef __vtkTreeRingReversedLayoutStrategy_h
#define __vtkTreeRingReversedLayoutStrategy_h

#include "vtkTreeRingLayoutStrategy.h"

class vtkIdList;

class VTK_INFOVIS_EXPORT vtkTreeRingReversedLayoutStrategy : public vtkTreeRingLayoutStrategy 
{
public:
  static vtkTreeRingReversedLayoutStrategy *New();

  vtkTypeRevisionMacro(vtkTreeRingReversedLayoutStrategy,vtkTreeRingLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The field name associated with the size of the vertex.
  vtkGetStringMacro(SizeFieldName);
  vtkSetStringMacro(SizeFieldName);

  // Description:
  // Perform the layout of a tree and place the results as 4-tuples in
  // coordsArray (innerRadius, outerRadius, startAngle, endAngle).
  void Layout(vtkTree *inputTree, vtkDataArray *coordsArray);

protected:
  vtkTreeRingReversedLayoutStrategy();
  ~vtkTreeRingReversedLayoutStrategy();

private:

  char * SizeFieldName;

  void LayoutChildren(
    vtkTree *tree, 
    vtkDataArray *coordsArray,
    vtkDataArray *sizeArray,
    vtkIdType nchildren,
    vtkIdType parent,
    vtkIdType begin, 
    float parentInnerRad,
    float parentStartAng, float parentEndAng);

  vtkTreeRingReversedLayoutStrategy(const vtkTreeRingReversedLayoutStrategy&);  // Not implemented.
  void operator=(const vtkTreeRingReversedLayoutStrategy&);  // Not implemented.
};

#endif

