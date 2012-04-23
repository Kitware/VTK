/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmicTreeLayoutStrategy.h

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkCosmicTreeLayoutStrategy - tree layout strategy reminiscent of astronomical systems
//
// .SECTION Description
// This layout strategy takes an input tree and places all the children of a
// node into a containing circle. The placement is such that each child
// placed can be represented with a circle tangent to the containing circle
// and (usually) 2 other children. The interior of the circle is left empty
// so that graph edges drawn on top of the tree will not obfuscate the tree.
// However, when one child is much larger than all the others, it may
// encroach on the center of the containing circle; that's OK, because it's
// large enough not to be obscured by edges drawn atop it.
//
// .SECTION Thanks
// Thanks to the galaxy and David Thompson hierarchically nested inside it
// for inspiring this layout strategy.

#ifndef __vtkCosmicTreeLayoutStrategy_h
#define __vtkCosmicTreeLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphLayoutStrategy.h"

class vtkDoubleArray;
class vtkDataArray;
class vtkPoints;
class vtkTree;

class VTKINFOVISLAYOUT_EXPORT vtkCosmicTreeLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkCosmicTreeLayoutStrategy* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeMacro(vtkCosmicTreeLayoutStrategy,vtkGraphLayoutStrategy);

  // Description:
  // Perform the layout.
  virtual void Layout();

  // Description:
  // Should node size specifications be obeyed at leaf nodes only or
  // (with scaling as required to meet constraints) at every node in
  // the tree?
  // This defaults to true, so that leaf nodes are scaled according to
  // the size specification provided, and the parent node sizes are
  // calculated by the algorithm.
  vtkSetMacro(SizeLeafNodesOnly,int);
  vtkGetMacro(SizeLeafNodesOnly,int);
  vtkBooleanMacro(SizeLeafNodesOnly,int);

  // Description:
  // How many levels of the tree should be laid out?
  // For large trees, you may wish to set the root and maximum depth
  // in order to retrieve the layout for the visible portion of the tree.
  // When this value is zero or negative, all nodes below and including
  // the LayoutRoot will be presented.
  // This defaults to 0.
  vtkSetMacro(LayoutDepth,int);
  vtkGetMacro(LayoutDepth,int);

  // Description:
  // What is the top-most tree node to lay out?
  // This node will become the largest containing circle in the layout.
  // Use this in combination with SetLayoutDepth to retrieve the
  // layout of a subtree of interest for rendering.
  // Setting LayoutRoot to a negative number signals that the root node
  // of the tree should be used as the root node of the layout.
  // This defaults to -1.
  vtkSetMacro(LayoutRoot,vtkIdType);
  vtkGetMacro(LayoutRoot,vtkIdType);

  // Description:
  // Set the array to be used for sizing nodes.
  // If this is set to an empty string or NULL (the default),
  // then all leaf nodes (or all nodes, when SizeLeafNodesOnly is false)
  // will be assigned a unit size.
  vtkSetStringMacro(NodeSizeArrayName);
  vtkGetStringMacro(NodeSizeArrayName);

protected:

  //BTX
  /// How are node sizes specified?
  enum RadiusMode
    {
    NONE,     //!< No node sizes specified... unit radius is assumed.
    LEAVES,   //!< Only leaf node sizes specified... parents are calculated during layout.
    ALL       //!< All node sizes specified (overconstrained, so a scale factor for each parent is calculated during layout).
    };
  //ETX

  vtkCosmicTreeLayoutStrategy();
  virtual ~vtkCosmicTreeLayoutStrategy();

  //BTX
  // Description:
  // Recursive routine used to lay out tree nodes. Called from Layout().
  void LayoutChildren(
    vtkTree* tree, vtkPoints* newPoints, vtkDoubleArray* radii, vtkDoubleArray* scale,
    vtkIdType root, int depth, RadiusMode mode );

  // Description:
  // Recursive routine that adds each parent node's (x,y) position to its children.
  // This must be done only after all the children have been laid out at the origin
  // since we will not know the parent's position until after the child radii have
  // been determined.
  void OffsetChildren(
    vtkTree* tree, vtkPoints* pts, vtkDoubleArray* radii, vtkDoubleArray* scale,
    double parent[4], vtkIdType root, int depth, RadiusMode mode );
  //ETX

  // Description:
  // Create an array to hold radii, named appropriately (depends on \a NodeSizeArrayName)
  // and initialized to either (a) -1.0 for each node or (b) a deep copy of an existing array.
  // @param numVertices  The number of vertices on the tree.
  // @param initialValue The starting value of each node's radius. Only used when \a inputRadii is NULL.
  // @param inputRadii   Either NULL or the address of another array to be copied into the output array
  // @retval             The array of node radii to be set on the output
  vtkDoubleArray* CreateRadii( vtkIdType numVertices, double initialValue, vtkDataArray* inputRadii );

  // Description:
  // Create an array to hold scale factors, named appropriately (depends on \a NodeSizeArrayName)
  // and initialized to -1.0.
  // @param numVertices The number of vertices on the tree.
  // @retval            The array of node scale factors to be set on the output
  vtkDoubleArray* CreateScaleFactors( vtkIdType numVertices );

  int SizeLeafNodesOnly;
  int LayoutDepth;
  vtkIdType LayoutRoot;
  char* NodeSizeArrayName;

private:
  vtkCosmicTreeLayoutStrategy( const vtkCosmicTreeLayoutStrategy& ); // Not implemented.
  void operator = ( const vtkCosmicTreeLayoutStrategy& ); // Not implemented.
};

#endif // __vtkCosmicTreeLayoutStrategy_h
