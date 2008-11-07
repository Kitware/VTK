/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingLayoutStrategy.h

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
// .NAME vtkTreeRingLayoutStrategy - abstract superclass for all tree ring layout strategies
//
// .SECTION Description
// All subclasses of this class perform a tree ring layout on a tree.
// This involves assigning a sector region to each vertex in the tree,
// and placing that information in a data array with four components per
// tuple representing (innerRadius, outerRadius, startAngle, endAngle).
//
// Instances of subclasses of this class may be assigned as the layout
// strategy to vtkTreeRingLayout
//
// .SECTION Thanks
// Thanks to Jason Shepherd from Sandia National Laboratories
// for help developing this class.

#ifndef __vtkTreeRingLayoutStrategy_h
#define __vtkTreeRingLayoutStrategy_h


#include "vtkObject.h"

class vtkTree;
class vtkDataArray;

class VTK_INFOVIS_EXPORT vtkTreeRingLayoutStrategy : public vtkObject 
{
public:
  vtkTypeRevisionMacro(vtkTreeRingLayoutStrategy,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform the layout of the input tree, and store the sector
  // bounds of each vertex as a tuple (innerRadius, outerRadius, startAngle, endAngle)
  // in a data array.
  virtual void Layout(vtkTree *inputTree, vtkDataArray *sectorArray) = 0;

  // Description:
  // Define the tree ring's interior radius.
  vtkSetMacro(InteriorRadius, double);
  vtkGetMacro(InteriorRadius, double);

  // Description:
  // Define the thickness of each of the tree rings.
  vtkSetMacro(RingThickness, double);
  vtkGetMacro(RingThickness, double);

  // Description:
  // Define the start angle for the root node.
  // NOTE: It is assumed that the root end angle is greater than the 
  // root start angle and subtends no more than 360 degrees.
  vtkSetMacro(RootStartAngle, double);
  vtkGetMacro(RootStartAngle, double);

  // Description:
  // Define the end angle for the root node.
  // NOTE: It is assumed that the root end angle is greater than the 
  // root start angle and subtends no more than 360 degrees.
  vtkSetMacro(RootEndAngle, double);
  vtkGetMacro(RootEndAngle, double);
  
protected:
  vtkTreeRingLayoutStrategy();
  ~vtkTreeRingLayoutStrategy();

  float InteriorRadius;
  float RingThickness;
  float RootStartAngle;
  float RootEndAngle;

private:  
  vtkTreeRingLayoutStrategy(const vtkTreeRingLayoutStrategy&);  // Not implemented.
  void operator=(const vtkTreeRingLayoutStrategy&);  // Not implemented.
};

#endif

