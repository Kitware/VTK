/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridNonOrientedGeometryCursor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright Nonice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridNonOrientedGeometryCursor
 * @brief   Objects for traversal a HyperTreeGrid.
 *
 * JB A REVOIR
 * NonOriented ne peut pas remonter plus haut qu'a sa creation.
 * Objects that can perform depth traversal of a hyper tree grid,
 * take into account more parameters (related to the grid structure) than
 * the compact hyper tree cursor implemented in vtkHyperTree can.
 * This is an abstract class.
 * Cursors are created by the HyperTreeGrid implementation.
 *
 * @sa
 * vtkHyperTreeCursor vtkHyperTree vtkHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien, 2014.
 * This class was re-written by Philippe Pebay, 2016.
 * JB This class was re-written for more optimisation by Jacques-Bernard Lekien,
 * Guenole Harel and Jerome Dubois, 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridNonOrientedGeometryCursor_h
#define vtkHyperTreeGridNonOrientedGeometryCursor_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkHyperTreeGridGeometryEntry.h" // Used internally
#include "vtkHyperTreeGridTools.h"         // for HasTree
#include "vtkSmartPointer.h"               // Used internally
#include <memory>                          // std::shared_ptr
#include <vector>                          // std::vector

class vtkHyperTree;
class vtkHyperTreeGrid;
class vtkHyperTreeGridScales;
class vtkHyperTreeGridOrientedGeometryCursor;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridNonOrientedGeometryCursor : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTreeGridNonOrientedGeometryCursor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkHyperTreeGridNonOrientedGeometryCursor* New();

  void Dump(ostream& os);

  // JB TODO 102018 On autorise le ToParent que jusqu'à ce que Level soit celui de la creation...
  // mais sans toRoot ? Une variante... qui serait utile aussi au niveau des SC

  /**
   * Create a copy of `this'.
   * \post results_exists:result!=0
   */
  virtual vtkHyperTreeGridNonOrientedGeometryCursor* Clone();

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  void Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false);

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  void Initialize(vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level,
    vtkHyperTreeGridGeometryEntry& entry);

  /**
   * JB
   */
  void Initialize(vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level, vtkIdType index,
    double* origin);

  /**
   * JB
   */
  void Initialize(vtkHyperTreeGridNonOrientedGeometryCursor* cursor);

  //@{
  /**
   * Return if a Tree pointing exist
   */
  bool HasTree() const { return vtk::hypertreegrid::HasTree(*this); }
  //@}

  //@{
  /**
   * Set the hyper tree to which the cursor is pointing.
   */
  vtkHyperTree* GetTree() const { return this->Tree; }
  //@}

  /**
   * Return the index of the current vertex in the tree.
   */
  vtkIdType GetVertexId();

  /**
   * Return the global index (relative to the grid) of the
   * current vertex in the tree.
   */
  vtkIdType GetGlobalNodeIndex();

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
   * JB
   */
  void GetBounds(double bounds[6]);
  void GetPoint(double point[3]);

  /**
   * Set the blanking mask is empty or not
   * \pre not_tree: tree
   */
  void SetMask(bool state);

  /**
   * Determine whether blanking mask is empty or not
   */
  bool IsMasked();

  /**
   * Is the cursor pointing to a leaf?
   */
  bool IsLeaf();

  /**
   * JB Fait chier normalement on devrait passer par GetEntry
   */
  void SubdivideLeaf();

  /**
   * Is the cursor at tree root?
   */
  bool IsRoot();

  /**
   * Get the level of the tree vertex pointed by the cursor.
   */
  unsigned int GetLevel();

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
   * JB Create a vtkHyperTreeGridOrientedGeometryCursor from input grid and
   * current entry data
   */
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> GetHyperTreeGridOrientedGeometryCursor(
    vtkHyperTreeGrid* grid);

protected:
  /**
   * Constructor
   * JB Just pour vtkHyperTreeGridNonOrientedVonNeumannSuperCursor et Moore
   */
  vtkHyperTreeGridNonOrientedGeometryCursor();

  /**
   * Destructor
   * JB Just pour vtkHyperTreeGridNonOrientedVonNeumannSuperCursor et Moore
   */
  ~vtkHyperTreeGridNonOrientedGeometryCursor() override;

  /**
   * JB Reference sur l'hyper tree grid parcouru actuellement.
   */
  vtkHyperTreeGrid* Grid;

  /**
   * JB
   */
  vtkHyperTree* Tree;

  /**
   * JB Storage of pre-computed per-level cell scales
   */
  std::shared_ptr<vtkHyperTreeGridScales> Scales;

  /**
   * JB
   */
  unsigned int Level;

  /**
   * JB La derniere entree valide.
   */
  int LastValidEntry;

  // Hyper tree grid to which the cursor is attached
  std::vector<vtkHyperTreeGridGeometryEntry> Entries;

private:
  vtkHyperTreeGridNonOrientedGeometryCursor(
    const vtkHyperTreeGridNonOrientedGeometryCursor&) = delete;
  void operator=(const vtkHyperTreeGridNonOrientedGeometryCursor&) = delete;
};
#endif
