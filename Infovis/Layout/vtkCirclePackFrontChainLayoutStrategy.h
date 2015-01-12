/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCirclePackFrontChainLayoutStrategy.h

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
// .NAME vtkCirclePackFrontChainLayoutStrategy - layout a vtkTree into packed circles
// using the front chain algorithm.
//
// .SECTION Description
// vtkCirclePackFrontChainLayoutStrategy assigns circles to each node of the input vtkTree
// using the front chain algorithm.  The algorithm packs circles by searching a "front
// chain" of circles around the perimeter of the circles that have already been packed for
// the current level in the tree hierarchy.  Searching the front chain is in general faster
// than searching all of the circles that have been packed at the current level.
//
// WARNING:  The algorithm tends to break down and produce packings with overlapping
// circles when there is a large difference in the radii of the circles at a given
// level of the tree hierarchy.  Roughly on the order a 1000:1 ratio of circle radii.
//
// Please see the following reference for more details on the algorithm.
//
// Title: "Visualization of large hierarchical data by circle packing"
// Authors:  Weixin Wang, Hui Wang, Guozhong Dai, Hongan Wang
// Conference: Proceedings of the SIGCHI conference on Human Factors in computing systems
// Year: 2006
//

#ifndef vtkCirclePackFrontChainLayoutStrategy_h
#define vtkCirclePackFrontChainLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkCirclePackLayoutStrategy.h"

class vtkCirclePackFrontChainLayoutStrategyImplementation;

class VTKINFOVISLAYOUT_EXPORT vtkCirclePackFrontChainLayoutStrategy : public vtkCirclePackLayoutStrategy
{
public:
  static vtkCirclePackFrontChainLayoutStrategy *New();

  vtkTypeMacro(vtkCirclePackFrontChainLayoutStrategy,vtkCirclePackLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform the layout of the input tree, and store the circle
  // bounds of each vertex as a tuple in a data array.
  // (Xcenter, Ycenter, Radius).
  //
  virtual void Layout(vtkTree *inputTree, vtkDataArray *areaArray,
                      vtkDataArray* sizeArray);

  // Description:
  // Width and Height define the size of the output window that the
  // circle packing is placed inside.  Defaults to Width 1, Height 1
  vtkGetMacro(Width, int);
  vtkSetMacro(Width, int);
  vtkGetMacro(Height, int);
  vtkSetMacro(Height, int);

protected:
  vtkCirclePackFrontChainLayoutStrategy();
  ~vtkCirclePackFrontChainLayoutStrategy();

  char * CirclesFieldName;
  int Width;
  int Height;

private:

  vtkCirclePackFrontChainLayoutStrategyImplementation* pimpl; // Private implementation

  vtkCirclePackFrontChainLayoutStrategy(const vtkCirclePackFrontChainLayoutStrategy&);  // Not implemented.
  void operator=(const vtkCirclePackFrontChainLayoutStrategy&);  // Not implemented.
};

#endif
