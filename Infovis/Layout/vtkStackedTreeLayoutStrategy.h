/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStackedTreeLayoutStrategy.h

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
/**
 * @class   vtkStackedTreeLayoutStrategy
 * @brief   lays out tree in stacked boxes or rings
 *
 *
 * Performs a tree ring layout or "icicle" layout on a tree.
 * This involves assigning a sector region to each vertex in the tree,
 * and placing that information in a data array with four components per
 * tuple representing (innerRadius, outerRadius, startAngle, endAngle).
 *
 * This class may be assigned as the layout strategy to vtkAreaLayout.
 *
 * @par Thanks:
 * Thanks to Jason Shepherd from Sandia National Laboratories
 * for help developing this class.
*/

#ifndef vtkStackedTreeLayoutStrategy_h
#define vtkStackedTreeLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkAreaLayoutStrategy.h"

class vtkTree;
class vtkDataArray;

class VTKINFOVISLAYOUT_EXPORT vtkStackedTreeLayoutStrategy :
  public vtkAreaLayoutStrategy
{
public:
  static vtkStackedTreeLayoutStrategy* New();
  vtkTypeMacro(vtkStackedTreeLayoutStrategy,vtkAreaLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Perform the layout of the input tree, and store the sector
   * bounds of each vertex as a tuple
   * (innerRadius, outerRadius, startAngle, endAngle)
   * in a data array.
   */
  void Layout(vtkTree *inputTree, vtkDataArray *sectorArray,
      vtkDataArray* sizeArray) VTK_OVERRIDE;

  /**
   * Fill edgeRoutingTree with points suitable for routing edges of
   * an overlaid graph.
   */
  void LayoutEdgePoints(vtkTree *inputTree, vtkDataArray *sectorArray,
      vtkDataArray* sizeArray, vtkTree *edgeRoutingTree) VTK_OVERRIDE;

  //@{
  /**
   * Define the tree ring's interior radius.
   */
  vtkSetMacro(InteriorRadius, double);
  vtkGetMacro(InteriorRadius, double);
  //@}

  //@{
  /**
   * Define the thickness of each of the tree rings.
   */
  vtkSetMacro(RingThickness, double);
  vtkGetMacro(RingThickness, double);
  //@}

  //@{
  /**
   * Define the start angle for the root node.
   * NOTE: It is assumed that the root end angle is greater than the
   * root start angle and subtends no more than 360 degrees.
   */
  vtkSetMacro(RootStartAngle, double);
  vtkGetMacro(RootStartAngle, double);
  //@}

  //@{
  /**
   * Define the end angle for the root node.
   * NOTE: It is assumed that the root end angle is greater than the
   * root start angle and subtends no more than 360 degrees.
   */
  vtkSetMacro(RootEndAngle, double);
  vtkGetMacro(RootEndAngle, double);
  //@}

  //@{
  /**
   * Define whether or not rectangular coordinates are being used
   * (as opposed to polar coordinates).
   */
  vtkSetMacro(UseRectangularCoordinates, bool);
  vtkGetMacro(UseRectangularCoordinates, bool);
  vtkBooleanMacro(UseRectangularCoordinates, bool);
  //@}

  //@{
  /**
   * Define whether to reverse the order of the tree stacks from
   * low to high.
   */
  vtkSetMacro(Reverse, bool);
  vtkGetMacro(Reverse, bool);
  vtkBooleanMacro(Reverse, bool);
  //@}

  //@{
  /**
   * The spacing of tree levels in the edge routing tree.
   * Levels near zero give more space
   * to levels near the root, while levels near one (the default)
   * create evenly-spaced levels. Levels above one give more space
   * to levels near the leaves.
   */
  vtkSetMacro(InteriorLogSpacingValue, double);
  vtkGetMacro(InteriorLogSpacingValue, double);
  //@}

  /**
   * Returns the vertex id that contains pnt (or -1 if no one contains it).
   */
  vtkIdType FindVertex(vtkTree* tree, vtkDataArray* array, float pnt[2]) VTK_OVERRIDE;

protected:
  vtkStackedTreeLayoutStrategy();
  ~vtkStackedTreeLayoutStrategy() VTK_OVERRIDE;

  float InteriorRadius;
  float RingThickness;
  float RootStartAngle;
  float RootEndAngle;
  bool UseRectangularCoordinates;
  bool Reverse;
  double InteriorLogSpacingValue;

  void ComputeEdgeRoutingPoints(
      vtkTree* inputTree, vtkDataArray* coordsArray, vtkTree* outputTree);

  void LayoutChildren(
      vtkTree *tree, vtkDataArray *coordsArray, vtkDataArray *sizeArray,
      vtkIdType nchildren, vtkIdType parent, vtkIdType begin,
      float parentInnerRad, float parentOuterRad,
      float parentStartAng, float parentEndAng);

private:
  vtkStackedTreeLayoutStrategy(const vtkStackedTreeLayoutStrategy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStackedTreeLayoutStrategy&) VTK_DELETE_FUNCTION;
};

#endif
