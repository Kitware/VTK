/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingDefaultLayoutStrategy.h
  
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
// .NAME vtkTreeRingDefaultLayoutStrategy - lays out a tree using
//  using concentric rings
//
// .SECTION Description
// vtkTreeRingDefaultLayoutStrategy partitions the space for child vertices
// circular sectors. Sectors are sized based on the relative vertex size.
//
// .SECTION Thanks to Jason Shepherd for this implementation.

#ifndef __vtkTreeRingDefaultLayoutStrategy_h
#define __vtkTreeRingDefaultLayoutStrategy_h

#include "vtkTreeRingLayoutStrategy.h"

class vtkIdList;

class VTK_INFOVIS_EXPORT vtkTreeRingDefaultLayoutStrategy : public vtkTreeRingLayoutStrategy 
{
public:
  static vtkTreeRingDefaultLayoutStrategy *New();

  vtkTypeRevisionMacro(vtkTreeRingDefaultLayoutStrategy,vtkTreeRingLayoutStrategy);
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
  vtkTreeRingDefaultLayoutStrategy();
  ~vtkTreeRingDefaultLayoutStrategy();

private:

  char * SizeFieldName;

  void LayoutChildren(
    vtkTree *tree, vtkDataArray *coordsArray, vtkDataArray *sizeArray,
    vtkIdType nchildren, vtkIdType parent, vtkIdType begin,  
    float parentOuterRad, float parentStartAng, float parentEndAng);

  vtkTreeRingDefaultLayoutStrategy(const vtkTreeRingDefaultLayoutStrategy&);  // Not implemented.
  void operator=(const vtkTreeRingDefaultLayoutStrategy&);  // Not implemented.
};

#endif

