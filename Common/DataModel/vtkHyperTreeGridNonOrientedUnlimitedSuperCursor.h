// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridNonOrientedUnlimitedSuperCursor
 * @brief   Objects for traversal a HyperTreeGrid.
 *
 * @sa
 * vtkHyperTreeGridNonOrientedSuperCursor vtkHyperTreeCursor vtkHyperTree vtkHyperTreeGrid
 */

#ifndef vtkHyperTreeGridNonOrientedUnlimitedSuperCursor_h
#define vtkHyperTreeGridNonOrientedUnlimitedSuperCursor_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // Used internally

#include "vtkHyperTreeGridGeometryUnlimitedLevelEntry.h" // Used Internally

#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTree;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedGeometryCursor;
class vtkHyperTreeGridOrientedGeometryCursor;
class vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridNonOrientedUnlimitedSuperCursor : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTreeGridNonOrientedUnlimitedSuperCursor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create a copy of `this'.
   * \post results_exists:result!=0
   */
  virtual vtkHyperTreeGridNonOrientedUnlimitedSuperCursor* Clone();

  /**
   * Initialize cursor at root of given tree index in grid.
   * JB Le create ne s'applique que sur le HT central.
   */
  virtual void Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false) = 0;

  ///@{
  /**
   * Set the hyper tree grid to which the cursor is pointing.
   */
  vtkHyperTreeGrid* GetGrid();
  ///@}

  ///@{
  /**
   * Return if a Tree pointing exist
   */
  bool HasTree();
  ///@}

  /**
   * JB Return if a Tree pointing exist
   */
  bool HasTree(unsigned int icursor);

  ///@{
  /**
   * Set the hyper tree to which the cursor is pointing.
   */
  vtkHyperTree* GetTree();
  vtkHyperTree* GetTree(unsigned int icursor);
  ///@}

  ///@{
  /**
   * Return the index of the current vertex in the tree.
   */
  vtkIdType GetVertexId();
  vtkIdType GetVertexId(unsigned int icursor);
  ///@}

  /**
   * Return the global index (relative to the grid) of the
   * current vertex in the tree.
   */
  vtkIdType GetGlobalNodeIndex();

  /**
   * JB Return the global index (relative to the grid) of the
   * neighbor icursor current vertex in the tree.
   */
  vtkIdType GetGlobalNodeIndex(unsigned int icursor);

  /**
   * JB
   */
  vtkHyperTree* GetInformation(
    unsigned int icursor, unsigned int& level, bool& leaf, vtkIdType& id);

  /**
   * Return the dimension of the tree.
   * \post positive_result: result>0
   */
  unsigned char GetDimension();

  /**
   * Return the number of children for each node (non-vertex leaf) of the tree.
   * \post positive_number: result>0
   */
  unsigned char GetNumberOfChildren();

  /**
   * JB
   */
  void SetGlobalIndexStart(vtkIdType index);

  /**
   * JB
   */
  void SetGlobalIndexFromLocal(vtkIdType index);

  /**
   * JB
   */
  double* GetOrigin();
  double* GetSize();

  /**
   * Set the blanking mask is empty or not
   * \pre not_tree: tree
   */
  void SetMask(bool state);
  void SetMask(unsigned int icursor, bool state);

  /**
   * Determine whether blanking mask is empty or not
   */
  bool IsMasked();
  bool IsMasked(unsigned int icursor);

  /**
   * JB Coordonnees de la boite englobante
   */
  void GetBounds(double bounds[6]);
  void GetBounds(unsigned int icursor, double bounds[6]);

  /**
   * JB Coordonnees du centre de la maille
   */
  void GetPoint(double point[3]);
  void GetPoint(unsigned int icursor, double point[3]);

  ///@{
  /**
   * Is the cursor pointing to a leaf?
   */
  bool IsLeaf();
  bool IsLeaf(unsigned int icursor);
  bool IsRealLeaf();
  bool IsRealLeaf(unsigned int icursor);
  ///@}

  ///@{
  /**
   * Is the cursor pointing to a real node in the tree
   */
  bool IsVirtualLeaf();
  bool IsVirtualLeaf(unsigned int icursor);
  ///@}

  ///@{
  /**
   * returns the value of the ratio to be applied to extensive
   * value for the current cursor, related to the last real
   * value of the cell. Return 1 for real cells, otherwise
   * return the portion of the area covered by the subdivieded cell.
   * For intensive valued fields this ratio should not be used.
   */
  double GetExtensivePropertyRatio();
  double GetExtensivePropertyRatio(vtkIdType index);
  ///@}

  /**
   * Is the cursor at tree root?
   */
  bool IsRoot();

  ///@{
  /**
   * Get the level of the tree vertex pointed by the cursor.
   */
  unsigned int GetLevel();
  unsigned int GetLevel(unsigned int icursor);
  unsigned int GetLastRealLevel();
  unsigned int GetLastRealLevel(unsigned int icursor);
  ///@}

  /**
   * Move the cursor to child `child' of the current vertex.
   * \pre not_tree: HasTree()
   * \pre not_leaf: !IsLeaf()
   * \pre valid_child: ichild>=0 && ichild<GetNumberOfChildren()
   * \pre depth_limiter: GetLevel() <= GetDepthLimiter()
   */
  void ToChild(unsigned char ichild);

  /**
   * Move the cursor to the root vertex.
   * \pre can be root
   * \post is_root: IsRoot()
   */
  void ToRoot();

  /**
   * Move the cursor to the parent of the current vertex.
   * Authorized if HasHistory return true.
   * \pre Non_root: !IsRoot()
   */
  void ToParent();

  /**
   * JB
   */
  unsigned int GetNumberOfCursors() { return this->NumberOfCursors; }

  /**
   * Return the cursor pointing into i-th neighbor.
   * The neighborhood definition depends on the type of cursor.
   * NB: Only super cursors keep track of neighborhoods.
   */
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> GetOrientedGeometryCursor(
    unsigned int icursor);

  /**
   * Return the cursor pointing into i-th neighbor.
   * The neighborhood definition depends on the type of cursor.
   * NB: Only super cursors keep track of neighborhoods.
   */
  vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor> GetNonOrientedGeometryCursor(
    unsigned int icursor);

protected:
  /**
   * Constructor
   */
  vtkHyperTreeGridNonOrientedUnlimitedSuperCursor();

  /**
   * Destructor
   */
  ~vtkHyperTreeGridNonOrientedUnlimitedSuperCursor() override;

  /**
   * JB Reference sur l'hyper tree grid parcouru actuellement.
   */
  vtkHyperTreeGrid* Grid = nullptr;

  /**
   * JB
   */
  vtkSmartPointer<vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor> CentralCursor;

  /**
   * JB Hyper tree grid to which the cursor is attached
   */
  unsigned int CurrentFirstNonValidEntryByLevel = 0;
  std::vector<unsigned int> FirstNonValidEntryByLevel;
  std::vector<vtkHyperTreeGridGeometryUnlimitedLevelEntry> Entries;

  /**
   * JB La derniere reference valide pour decrire tous les voisins.
   * C'est donc aussi l'offset du premier voisin du dernier niveau.
   */
  unsigned int FirstCurrentNeighboorReferenceEntry = 0;
  std::vector<unsigned int> ReferenceEntries;

  /**
   * JB
   */
  unsigned int GetIndiceEntry(unsigned int icursor);

  /**
   * JB La valeur precedente. Dans le voisinage, ce n'est pas forcement un parent.
   */
  unsigned int GetIndicePreviousEntry(unsigned int icursor);

  /**
   * JB
   */
  unsigned int IndiceCentralCursor = 0;

  // Number of cursors in supercursor
  unsigned int NumberOfCursors = 0;

  // Super cursor traversal table to go retrieve the parent index for each cursor
  // of the child node. There are f^d * NumberOfCursors entries in the table.
  const unsigned int* ChildCursorToParentCursorTable = nullptr;

  // Super cursor traversal table to go retrieve the child index for each cursor
  // of the child node. There are f^d * NumberOfCursors entries in the table.
  const unsigned int* ChildCursorToChildTable = nullptr;

private:
  vtkHyperTreeGridNonOrientedUnlimitedSuperCursor(
    const vtkHyperTreeGridNonOrientedUnlimitedSuperCursor&) = delete;
  void operator=(const vtkHyperTreeGridNonOrientedUnlimitedSuperCursor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
