/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoxLayoutStrategy.h

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
// .NAME vtkBoxLayoutStrategy - a tree map layout that puts nodes in square-ish boxes
//
// .SECTION Description
// vtkBoxLayoutStrategy recursively partitions the space for children nodes
// in a tree-map into square regions (or regions very close to a square).
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for creating this class.

#ifndef __vtkBoxLayoutStrategy_h
#define __vtkBoxLayoutStrategy_h

#include "vtkTreeMapLayoutStrategy.h"

class VTK_INFOVIS_EXPORT vtkBoxLayoutStrategy : public vtkTreeMapLayoutStrategy 
{
public:
  static vtkBoxLayoutStrategy *New();

  vtkTypeRevisionMacro(vtkBoxLayoutStrategy,vtkTreeMapLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform the layout of a tree and place the results as 4-tuples in
  // coordsArray (Xmin, Xmax, Ymin, Ymax).
  void Layout(vtkTree *inputTree, vtkDataArray *coordsArray);
protected:
  vtkBoxLayoutStrategy();
  ~vtkBoxLayoutStrategy();

private:

  void LayoutChildren(vtkTree *inputTree, vtkDataArray *coordsArray,
    vtkIdType parentId, 
    float parentMinX, float parentMaxX, 
    float parentMinY, float parentMaxY);

  vtkBoxLayoutStrategy(const vtkBoxLayoutStrategy&);  // Not implemented.
  void operator=(const vtkBoxLayoutStrategy&);  // Not implemented.
};

#endif

