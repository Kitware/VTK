// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDataAssemblyVisitor
 * @brief visitor API for vtkDataAssembly
 *
 * vtkDataAssemblyVisitor defines a visitor API for vtkDataAssembly. A concrete
 * subclass of vtkDataAssemblyVisitor can be passed to `vtkDataAssembly::Visit`
 * to execute custom code on each node in the data-assembly.
 *
 * `vtkDataAssembly::Visit` will call `vtkDataAssemblyVisitor::Visit` on each
 * node in the assembly (or chosen subtree). The traversal order, i.e.
 * depth-first or breadth-first, is selected by the arguments passed to
 * `vtkDataAssembly::Visit`. Before traversing a sub-tree for a particular node,
 * `vtkDataAssemblyVisitor::GetTraverseSubtree` is called, if it returns false,
 * the subtree is skipped. If it returns true, then then
 * `vtkDataAssemblyVisitor::BeginSubTree` is called, followed by calls to
 * `vtkDataAssemblyVisitor::Visit` for each of the child nodes, and finally
 * `vtkDataAssemblyVisitor::EndSubTree` is called.
 *
 * In depth-first order, the subtree traversal is recursive. Thus, after
 * `BeginSubTree` is called for specific node, all its children and their
 * subtrees are traversed before `EndSubTree` gets called for that node.
 *
 * In breadth-first order, a first-in-first-out queue is used. A node is
 * visited, i.e. `vtkDataAssemblyVisitor::Visit` called on it, then if
 * `GetTraverseSubtree` returns true, `Visit` gets called on all its immediate
 * children one after another followed by `EndSubTree` on the parent node.
 * As each of the child nodes are visited, they get added to the queue.
 * Now, for each node in the queue, the process repeats i.e.
 * `GetTraverseSubtree` is called, followed by the subtree traversal for that
 * node. This continues until the queue empty.
 *
 * @sa vtkDataAssembly
 */

#ifndef vtkDataAssemblyVisitor_h
#define vtkDataAssemblyVisitor_h

#include "vtkCommonDataModelModule.h" // for export macros
#include "vtkObject.h"
#include <memory> // for std::unique_ptr
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataAssembly;

class VTKCOMMONDATAMODEL_EXPORT vtkDataAssemblyVisitor : public vtkObject
{
public:
  vtkTypeMacro(vtkDataAssemblyVisitor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkDataAssemblyVisitor();
  ~vtkDataAssemblyVisitor() override;

  /**
   * Provides access to the assembly during traversal, otherwise returns
   * nullptr.
   */
  const vtkDataAssembly* GetAssembly() const { return this->Assembly; }

  /**
   * Returns the current traversal order.
   */
  int GetTraversalOrder() const { return this->TraversalOrder; }

  /**
   * Called on every node being visited. @arg nodeid is the id of the node being
   * visited.
   *
   */
  virtual void Visit(int nodeid) = 0;

  /**
   * Called to check if the subtree anchored at `nodeid` is to be traversed.
   * Default implementation returns true. Subclasses can override this method to
   * skip traversing 'dead' branches.
   */
  virtual bool GetTraverseSubtree(int vtkNotUsed(nodeid)) { return true; }

  ///@{
  /**
   * Methods called at the start and end of a subtree traversal.
   */
  virtual void BeginSubTree(int vtkNotUsed(nodeid)) {}
  virtual void EndSubTree(int vtkNotUsed(nodeid)) {}
  ///@}

  ///@{
  /**
   * API to access information from the current node being processed.
   * It's preferred to use this API rather than API on vtkDataAssembly when
   * working with the current node for improved performance.
   */
  const char* GetCurrentNodeName() const;
  std::vector<unsigned int> GetCurrentDataSetIndices() const;
  ///@}

private:
  vtkDataAssemblyVisitor(const vtkDataAssemblyVisitor&) = delete;
  void operator=(const vtkDataAssemblyVisitor&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  const vtkDataAssembly* Assembly;
  int TraversalOrder;
  friend class vtkDataAssembly;
};

VTK_ABI_NAMESPACE_END
#endif
