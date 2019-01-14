/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConeLayoutStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//-------------------------------------------------------------------------
//Copyright 2008 Sandia Corporation.
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
//-------------------------------------------------------------------------

/**
 * @class   vtkConeLayoutStrategy
 * @brief   produce a cone-tree layout for a forest
 *
 * vtkConeLayoutStrategy positions the nodes of a tree(forest) in 3D
 * space based on the cone-tree approach first described by
 * Robertson, Mackinlay and Card in Proc. CHI'91.  This
 * implementation incorporates refinements to the layout
 * developed by Carriere and Kazman, and by Auber.
 *
 * The input graph must be a forest (i.e. a set of trees, or a
 * single tree); in the case of a forest, the input will be
 * converted to a single tree by introducing a new root node,
 * and connecting each root in the input forest to the meta-root.
 * The tree is then laid out, after which the meta-root is removed.
 *
 * The cones are positioned so that children lie in planes parallel
 * to the X-Y plane, with the axis of cones parallel to Z, and
 * with Z coordinate increasing with distance of nodes from the root.
 *
 * @par Thanks:
 * Thanks to David Duke from the University of Leeds for providing this
 * implementation.
*/

#ifndef vtkConeLayoutStrategy_h
#define vtkConeLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphLayoutStrategy.h"

class vtkPoints;

class VTKINFOVISLAYOUT_EXPORT vtkConeLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkConeLayoutStrategy *New();

  vtkTypeMacro(vtkConeLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Determine the compactness, the ratio between the
   * average width of a cone in the tree, and the
   * height of the cone.  The default setting is 0.75
   * which (empirically) seems reasonable, but this
   * will need adapting depending on the data.
   */
  vtkSetMacro(Compactness, float);
  vtkGetMacro(Compactness, float);
  //@}

  //@{
  /**
   * Determine if layout should be compressed, i.e. the
   * layout puts children closer together, possibly allowing
   * sub-trees to overlap.  This is useful if the tree is
   * actually the spanning tree of a graph.  For "real" trees,
   * non-compressed layout is best, and is the default.
   */
  vtkSetMacro(Compression, vtkTypeBool);
  vtkGetMacro(Compression, vtkTypeBool);
  vtkBooleanMacro(Compression, vtkTypeBool);
  //@}

  //@{
  /**
   * Set the spacing parameter that affects space between
   * layers of the tree.  If compression is on, Spacing is the
   * actual distance between layers.  If compression is off,
   * actual distance also includes a factor of the compactness
   * and maximum cone radius.
   */
  vtkSetMacro(Spacing, float);
  vtkGetMacro(Spacing, float);
  //@}


  /**
   * Perform the layout.
   */
  void Layout() override;

protected:
  vtkConeLayoutStrategy();
  ~vtkConeLayoutStrategy() override;

  /**
   * Helper operations for tree layout.  Layout is performed
   * in two traversals of the tree.  The first traversal finds
   * the position of child nodes relative to their parent. The
   * second traversal positions each node absolutely, working
   * from the initial position of the root node.
   */

  double LocalPlacement(vtkIdType root, vtkPoints *points);

  void GlobalPlacement(
    vtkIdType root,
    vtkPoints *points,
    double refX,         // absolute x-y coordinate of
    double refY,         // parent node; z coordinate
    double level );      // derived from level.

  float Compactness;     // factor used in mapping layer to Z
  vtkTypeBool   Compression;     // force a compact layout?
  float Spacing;         // Scale vertical spacing of cones.

  // Values accumulated for possible statistical use
  double MinRadius;
  double MaxRadius;
  int   NrCones;
  double SumOfRadii;

private:
  vtkConeLayoutStrategy(const vtkConeLayoutStrategy&) = delete;
  void operator=(const vtkConeLayoutStrategy&) = delete;
};

#endif

