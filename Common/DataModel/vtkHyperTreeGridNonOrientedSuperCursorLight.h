// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridNonOrientedSuperCursorLight
 * @brief   Objects for traversal a HyperTreeGrid.
 *
 * Objects that can perform depth traversal of a hyper tree grid,
 * take into account more parameters (related to the grid structure) than
 * the compact hyper tree cursor implemented in vtkHyperTree can.
 * This is an abstract class.
 * Cursors are created by the HyperTreeGrid implementation.
 *
 * @sa
 * vtkHyperTree vtkHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien, 2014.
 * This class was re-written by Philippe Pebay, 2016.
 * This class was re-written and optimized by Jacques-Bernard Lekien,
 * Guenole Harel and Jerome Dubois, 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridNonOrientedSuperCursorLight_h
#define vtkHyperTreeGridNonOrientedSuperCursorLight_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // Used internally

#include "vtkHyperTreeGridLevelEntry.h" // Used internally

#include <cassert> // Used internally
#include <vector>  // std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTree;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridNonOrientedSuperCursorLight : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTreeGridNonOrientedSuperCursorLight, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create a copy of `this'.
   * \post results_exists:result!=0
   */
  virtual vtkHyperTreeGridNonOrientedSuperCursorLight* Clone();

  /**
   * Initialize cursor at root of given tree index in grid.
   * "create" only applies on the central HT
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
   * Return if a Tree pointing exist
   */
  bool HasTree(unsigned int icursor);

  ///@{
  /**
   * Set the hyper tree to which the cursor is pointing.
   */
  vtkHyperTree* GetTree();
  vtkHyperTree* GetTree(unsigned int icursor);
  ///@}

  /**
   * Return the index of the current vertex in the tree.
   */
  vtkIdType GetVertexId();
  vtkIdType GetVertexId(unsigned int icursor);

  /**
   * Return the global index (relative to the grid) of the
   * current vertex in the tree.
   */
  vtkIdType GetGlobalNodeIndex();

  /**
   * Return the global index (relative to the grid) of the
   * neighbor icursor current vertex in the tree.
   */
  vtkIdType GetGlobalNodeIndex(unsigned int icursor);

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

  void SetGlobalIndexStart(vtkIdType index);

  void SetGlobalIndexFromLocal(vtkIdType index);

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
   * Bounding box coordinates
   */
  void GetBounds(double bounds[6]);

  /**
   * Mesh center coordinates
   */
  void GetPoint(double point[3]);

  /**
   * Is the cursor pointing to a leaf?
   */
  bool IsLeaf();
  bool IsLeaf(unsigned int icursor);

  void SubdivideLeaf();

  /**
   * Is the cursor at tree root?
   */
  bool IsRoot();

  /**
   * Get the level of the tree vertex pointed by the cursor.
   */
  unsigned int GetLevel();
  unsigned int GetLevel(unsigned int icursor);

  /**
   * Move the cursor to child `child' of the current vertex.
   * \pre Non_leaf: !IsLeaf()
   * \pre valid_child: ichild>=0 && ichild<this->GetNumberOfChildren()
   */
  void ToChild(unsigned char);

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

  unsigned int GetNumberOfCursors() { return this->NumberOfCursors; }

protected:
  /**
   * Constructor
   */
  vtkHyperTreeGridNonOrientedSuperCursorLight();

  /**
   * Destructor
   */
  ~vtkHyperTreeGridNonOrientedSuperCursorLight() override;

  /**
   * Currently processed Hyper Tree
   */
  vtkHyperTreeGrid* Grid;

  vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor> CentralCursor;

  /**
   * Hyper tree grid to which the cursor is attached
   */
  unsigned int CurrentFirstNonValidEntryByLevel;
  std::vector<unsigned int> FirstNonValidEntryByLevel;
  std::vector<vtkHyperTreeGridLevelEntry> Entries;

  /**
   * The last valid reference to describe neighbors.
   * It is also the offset of the first neighbor at the deepest level.
   */
  unsigned int FirstCurrentNeighboorReferenceEntry;
  std::vector<unsigned int> ReferenceEntries;

  unsigned int GetIndiceEntry(unsigned int icursor)
  {
    assert("pre: icursor != IndiceCentralCursor" && icursor != this->IndiceCentralCursor);
    assert("pre: valid_icursor" && icursor < this->NumberOfCursors);
    if (icursor > this->IndiceCentralCursor)
    {
      assert("pre: valid_icursor" &&
        0 <= long(this->FirstCurrentNeighboorReferenceEntry + icursor) - 1 &&
        long(this->FirstCurrentNeighboorReferenceEntry + icursor) - 1 <
          long(this->ReferenceEntries.size()));
      assert("pre: valid_icursor" &&
        this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry + icursor - 1] <
          this->Entries.size());
      return this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry + icursor - 1];
    }
    else
    {
      assert("pre: valid_icursor" &&
        this->FirstCurrentNeighboorReferenceEntry + icursor < this->ReferenceEntries.size());
      assert("pre: valid_icursor" &&
        this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry + icursor] <
          this->Entries.size());
      return this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry + icursor];
    }
  }

  /**
   * The previous value. In the neighborhood, it does not have to be a parent.
   */
  unsigned int GetIndicePreviousEntry(unsigned int icursor)
  {
    assert("pre: icursor != IndiceCentralCursor" && icursor != IndiceCentralCursor);
    assert("pre: valid_icursor" && icursor < this->NumberOfCursors);
    if (icursor > this->IndiceCentralCursor)
    {
      assert("pre: valid_icursor" &&
        0 <=
          long(this->FirstCurrentNeighboorReferenceEntry - (this->NumberOfCursors - 1) + icursor) -
            1 &&
        long(this->FirstCurrentNeighboorReferenceEntry - (this->NumberOfCursors - 1) + icursor) -
            1 <
          long(this->ReferenceEntries.size()));
      assert("pre: valid_icursor" &&
        this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry -
          (this->NumberOfCursors - 1) + icursor - 1] < this->Entries.size());
      return this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry -
        (this->NumberOfCursors - 1) + icursor - 1];
    }
    else
    {
      assert("pre: valid_icursor" &&
        this->FirstCurrentNeighboorReferenceEntry - (this->NumberOfCursors - 1) + icursor <
          this->ReferenceEntries.size());
      assert("pre: valid_icursor" &&
        this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry -
          (this->NumberOfCursors - 1) + icursor] < this->Entries.size());
      return this->ReferenceEntries[this->FirstCurrentNeighboorReferenceEntry -
        (this->NumberOfCursors - 1) + icursor];
    }
  }

  unsigned int IndiceCentralCursor;

  // Number of cursors in supercursor
  unsigned int NumberOfCursors;

  // Super cursor traversal table to go retrieve the parent index for each cursor
  // of the child node. There are f^d * NumberOfCursors entries in the table.
  const unsigned int* ChildCursorToParentCursorTable;

  // Super cursor traversal table to go retrieve the child index for each cursor
  // of the child node. There are f^d * NumberOfCursors entries in the table.
  const unsigned int* ChildCursorToChildTable;

private:
  vtkHyperTreeGridNonOrientedSuperCursorLight(
    const vtkHyperTreeGridNonOrientedSuperCursorLight&) = delete;
  void operator=(const vtkHyperTreeGridNonOrientedSuperCursorLight&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
