/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeLayoutStrategy.h

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
 * @class   vtkTreeLayoutStrategy
 * @brief   hierarchical layout
 *
 *
 * Assigns points to the nodes of a tree in either a standard or radial layout.
 * The standard layout places each level on a horizontal line, while the
 * radial layout places each level on a concentric circle.
 * You may specify the sweep angle of the tree which constrains the tree
 * to be contained within a wedge. Also, you may indicate the log scale of
 * the tree, which diminishes the length of arcs at lower levels of the tree.
 * Values near zero give a large proportion of the space to the tree levels
 * near the root, while values near one give nearly equal proportions of space
 * to all tree levels.
 *
 * The user may also specify an array to use to indicate the distance from the
 * root, either vertically (for standard layout) or radially
 * (for radial layout).  You specify this with SetDistanceArrayName().
 *
 * If the input is not a tree but a general graph, this strategy first extracts
 * a tree from the graph using a breadth-first search starting at vertex ID 0.
*/

#ifndef vtkTreeLayoutStrategy_h
#define vtkTreeLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphLayoutStrategy.h"

class VTKINFOVISLAYOUT_EXPORT vtkTreeLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkTreeLayoutStrategy *New();

  vtkTypeMacro(vtkTreeLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Perform the tree layout.
   */
  void Layout() VTK_OVERRIDE;

  //@{
  /**
   * The sweep angle of the tree.
   * For a standard tree layout, this should be between 0 and 180.
   * For a radial tree layout, this can be between 0 and 360.
   */
  vtkSetClampMacro(Angle, double, 0, 360);
  vtkGetMacro(Angle, double);
  //@}

  //@{
  /**
   * If set, the tree is laid out with levels on concentric circles
   * around the root. If unset (default), the tree is laid out with
   * levels on horizontal lines.
   */
  vtkSetMacro(Radial, bool);
  vtkGetMacro(Radial, bool);
  vtkBooleanMacro(Radial, bool);
  //@}

  //@{
  /**
   * The spacing of tree levels. Levels near zero give more space
   * to levels near the root, while levels near one (the default)
   * create evenly-spaced levels. Levels above one give more space
   * to levels near the leaves.
   */
  vtkSetMacro(LogSpacingValue, double);
  vtkGetMacro(LogSpacingValue, double);
  //@}

  //@{
  /**
   * The spacing of leaves.  Levels near one evenly space leaves
   * with no gaps between subtrees.  Levels near zero creates
   * large gaps between subtrees.
   */
  vtkSetClampMacro(LeafSpacing, double, 0.0, 1.0);
  vtkGetMacro(LeafSpacing, double);
  //@}

  //@{
  /**
   * Get/Set the array to use to determine the distance from the
   * root.
   */
  vtkSetStringMacro(DistanceArrayName);
  vtkGetStringMacro(DistanceArrayName);
  //@}

  //@{
  /**
   * The amount of counter-clockwise rotation to apply after the
   * layout.
   */
  vtkSetMacro(Rotation, double);
  vtkGetMacro(Rotation, double);
  //@}

  //@{
  /**
   * If set and the input is not a tree but a general graph, the filter
   * will reverse the edges on the graph before extracting a tree using
   * breadth first search.
   */
  vtkSetMacro(ReverseEdges, bool);
  vtkGetMacro(ReverseEdges, bool);
  vtkBooleanMacro(ReverseEdges, bool);
  //@}

protected:
  vtkTreeLayoutStrategy();
  ~vtkTreeLayoutStrategy() VTK_OVERRIDE;

  double Angle;
  bool Radial;
  double LogSpacingValue;
  double LeafSpacing;
  char *DistanceArrayName;
  double Rotation;
  bool ReverseEdges;

private:

  vtkTreeLayoutStrategy(const vtkTreeLayoutStrategy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTreeLayoutStrategy&) VTK_DELETE_FUNCTION;
};

#endif

