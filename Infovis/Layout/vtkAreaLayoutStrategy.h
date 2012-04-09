/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAreaLayoutStrategy.h

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
// .NAME vtkAreaLayoutStrategy - abstract superclass for all area layout strategies
//
// .SECTION Description
// All subclasses of this class perform a area layout on a tree.
// This involves assigning a region to each vertex in the tree,
// and placing that information in a data array with four components per
// tuple representing (innerRadius, outerRadius, startAngle, endAngle).
//
// Instances of subclasses of this class may be assigned as the layout
// strategy to vtkAreaLayout
//
// .SECTION Thanks
// Thanks to Jason Shepherd from Sandia National Laboratories
// for help developing this class.

#ifndef __vtkAreaLayoutStrategy_h
#define __vtkAreaLayoutStrategy_h


#include "vtkObject.h"

class vtkTree;
class vtkDataArray;

class VTK_INFOVIS_EXPORT vtkAreaLayoutStrategy : public vtkObject
{
public:
  vtkTypeMacro(vtkAreaLayoutStrategy,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform the layout of the input tree, and store the sector
  // bounds of each vertex as a tuple in a data array.
  // For radial layout, this is
  // (innerRadius, outerRadius, startAngle, endAngle).
  // For rectangular layout, this is
  // (xmin, xmax, ymin, ymax).
  //
  // The sizeArray may be NULL, or may contain the desired
  // size of each vertex in the tree.
  virtual void Layout(vtkTree *inputTree, vtkDataArray *areaArray,
      vtkDataArray* sizeArray) = 0;

  // Modify edgeLayoutTree to have point locations appropriate
  // for routing edges on a graph overlaid on the tree.
  // Layout() is called before this method, so inputTree will contain the
  // layout locations.
  // If you do not override this method,
  // the edgeLayoutTree vertex locations are the same as the input tree.
  virtual void LayoutEdgePoints(vtkTree *inputTree, vtkDataArray *areaArray,
      vtkDataArray* sizeArray, vtkTree *edgeLayoutTree);

  // Description:
  // Returns the vertex id that contains pnt (or -1 if no one contains it)
  virtual vtkIdType FindVertex(vtkTree* tree, vtkDataArray* array, float pnt[2]) = 0;

  // Descripiton:
  // The amount that the regions are shrunk as a value from
  // 0.0 (full size) to 1.0 (shrink to nothing).
  vtkSetClampMacro(ShrinkPercentage, double, 0.0, 1.0);
  vtkGetMacro(ShrinkPercentage, double);

protected:
  vtkAreaLayoutStrategy();
  ~vtkAreaLayoutStrategy();

  double ShrinkPercentage;

private:
  vtkAreaLayoutStrategy(const vtkAreaLayoutStrategy&);  // Not implemented.
  void operator=(const vtkAreaLayoutStrategy&);  // Not implemented.
};

#endif

