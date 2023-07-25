// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkLabelHierarchyIterator
 * @brief   iterator over vtkLabelHierarchy
 *
 *
 * Abstract superclass for iterators over vtkLabelHierarchy.
 */

#ifndef vtkLabelHierarchyIterator_h
#define vtkLabelHierarchyIterator_h

#include "vtkObject.h"
#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkStdString.h"            // for std string

VTK_ABI_NAMESPACE_BEGIN
class vtkIdTypeArray;
class vtkLabelHierarchy;
class vtkPolyData;

class VTKRENDERINGLABEL_EXPORT vtkLabelHierarchyIterator : public vtkObject
{
public:
  vtkTypeMacro(vtkLabelHierarchyIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initializes the iterator. lastLabels is an array holding labels
   * which should be traversed before any other labels in the hierarchy.
   * This could include labels placed during a previous rendering or
   * a label located under the mouse pointer. You may pass a null pointer.
   */
  virtual void Begin(vtkIdTypeArray*) {}

  /**
   * Advance the iterator.
   */
  virtual void Next() {}

  /**
   * Returns true if the iterator is at the end.
   */
  virtual bool IsAtEnd() { return true; }

  /**
   * Retrieves the current label location.
   */
  virtual void GetPoint(double x[3]);

  /**
   * Retrieves the current label size.
   */
  virtual void GetSize(double sz[2]);

  /**
   * Retrieves the current label maximum width in world coordinates.
   */
  virtual void GetBoundedSize(double sz[2]);

  /**
   * Retrieves the current label type.
   */
  virtual int GetType();

  /**
   * Retrieves the current label string.
   */
  virtual vtkStdString GetLabel();

  /**
   * Retrieves the current label orientation.
   */
  virtual double GetOrientation();

  /**
   * Retrieves the current label id.
   */
  virtual vtkIdType GetLabelId() { return -1; }

  ///@{
  /**
   * Get the label hierarchy associated with the current label.
   */
  vtkGetObjectMacro(Hierarchy, vtkLabelHierarchy);
  ///@}

  /**
   * Sets a polydata to fill with geometry representing
   * the bounding boxes of the traversed octree nodes.
   */
  virtual void SetTraversedBounds(vtkPolyData*);

  /**
   * Retrieve the coordinates of the center of the current hierarchy node
   * and the size of the node.
   * Nodes are n-cubes, so the size is the length of any edge of the cube.
   * This is used by BoxNode().
   */
  virtual void GetNodeGeometry(double ctr[3], double& size) = 0;

  /**
   * Add a representation to TraversedBounds for the current octree node.
   * This should be called by subclasses inside Next().
   * Does nothing if TraversedBounds is NULL.
   */
  virtual void BoxNode();

  /**
   * Add a representation for all existing octree nodes to the specified polydata.
   * This is equivalent to setting TraversedBounds, iterating over the entire hierarchy,
   * and then resetting TraversedBounds to its original value.
   */
  virtual void BoxAllNodes(vtkPolyData*);

  ///@{
  /**
   * Set/get whether all nodes in the hierarchy should be added to the TraversedBounds
   * polydata or only those traversed.
   * When non-zero, all nodes will be added.
   * By default, AllBounds is 0.
   */
  vtkSetMacro(AllBounds, int);
  vtkGetMacro(AllBounds, int);
  ///@}

protected:
  vtkLabelHierarchyIterator();
  ~vtkLabelHierarchyIterator() override;

  void BoxNodeInternal3(const double* ctr, double sz);
  void BoxNodeInternal2(const double* ctr, double sz);

  /**
   * The hierarchy being traversed by this iterator.
   */
  virtual void SetHierarchy(vtkLabelHierarchy* h);

  vtkLabelHierarchy* Hierarchy;
  vtkPolyData* TraversedBounds;
  double BoundsFactor;
  int AllBounds;
  int AllBoundsRecorded;

private:
  vtkLabelHierarchyIterator(const vtkLabelHierarchyIterator&) = delete;
  void operator=(const vtkLabelHierarchyIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkLabelHierarchyIterator_h
