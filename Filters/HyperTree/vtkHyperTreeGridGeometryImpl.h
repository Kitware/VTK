// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridGeometryImpl
 * @brief Interface for all vtkHyperTreeGridGeometry internal classes
 *
 * This class defines the common interface of all vtkHyperTreeGridGeometry internal classes.
 * The role of these classes is to perform the actual generation of the external surface
 * (geometry) of the input vtkHyperTreeGrid.
 *
 * The code is split into specific internal classes depending on the dimension of the input HTG.
 * Each class implement the pure virtual `GenerateGeometry` method, that achieve the construction
 * of the HTG surface.
 */

#ifndef vtkHyperTreeGridGeometryImpl_h
#define vtkHyperTreeGridGeometryImpl_h

#include "vtkHyperTreeGridGeometry.h"
#include "vtkIdTypeArray.h"

#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkMergePoints;
class vtkPoints;
class vtkUnsignedCharArray;

class vtkHyperTreeGridGeometryImpl
{
public:
  vtkHyperTreeGridGeometryImpl(vtkHyperTreeGrid* input, vtkPoints* outPoints,
    vtkCellArray* outCells, vtkDataSetAttributes* inCellDataAttributes,
    vtkDataSetAttributes* outCellDataAttributes, bool passThroughCellIds,
    const std::string& originalCellIdArrayName, bool fillMaterial);

  virtual ~vtkHyperTreeGridGeometryImpl() = default;

  /**
   * Generate the external surface of the input vtkHyperTreeGrid.
   * This method is implemented by subclasses, depending on the dimension of the HTG.
   */
  virtual void GenerateGeometry() = 0;

protected:
  ///@{
  /**
   * Compute the value of the distance from the given point to
   * the interface plane of the currently considered HTG cell.
   *
   * A and B refers to the two interfaces that can cut the HTG cell.
   *
   * XXX: The value returned by this function depends on the actual
   * values of CellIntercepts and CellNormals.
   */
  double ComputeDistanceToInterfaceA(const double* xyz) const;
  double ComputeDistanceToInterfaceB(const double* xyz) const;
  ///@}

  /**
   * Insert a new output cell from a list of point ids in the output polydata
   * and copy the data from the input HTG cell at cellId to the newly created
   * surface cell.
   */
  void CreateNewCellAndCopyData(const std::vector<vtkIdType>& outPointIds, vtkIdType cellId);

  /**
   * Returns true if the input HTG cell is masked or ghosted.
   */
  bool IsMaskedOrGhost(vtkIdType globalNodeId) const;

  /**
   * Determine if the input HTG at cellId contains an valid interface and
   * if yes, determine its characteristics, stored in the variables below.
   *
   * Returns true in case of a "valid" interface description.
   *
   * XXX: Populates:
   * - HasInterfaceOnThisCell
   * - CellInterfaceType
   * - CellNormals
   * - CellIntercepts
   */
  bool ProbeForCellInterface(vtkIdType cellId, bool invert = true);

  /**
   * Input parameters retrieved from constructor
   */
  vtkHyperTreeGrid* Input = nullptr;
  vtkPoints* OutPoints = nullptr;
  vtkCellArray* OutCells = nullptr;
  vtkDataSetAttributes* InCellDataAttributes = nullptr;
  vtkDataSetAttributes* OutCellDataAttributes = nullptr;
  bool FillMaterial = true;

  /**
   * Retrieved from input for quick access
   */
  vtkDataArray* InIntercepts = nullptr;
  vtkDataArray* InNormals = nullptr;

  /**
   * True if input HTG have an interface and if
   * InIntercepts and InNormals are defined
   */
  bool HasInterface = false;

  /**
   * True if the current cell have a "valid"
   * interface defined.
   *
   * XXX: cache for the "current" cell
   * XXX: depends on HasInterface, CellIntercepts and CellNormals
   */
  bool HasInterfaceOnThisCell = false;

  /**
   * Categorize the current cell interface type.
   * Possible values are:
   * -1 : mixed cell with an interface
   *      normal points to the "inside" of the cell
   *  0 : mixed cell with double interface
   *  1 : mixed cell with an interface
   *      normal points to the "outside" of the cell
   *  2 : pure cell (no interface)
   *
   * XXX: cache for the "current" cell
   * XXX: retrieved from CellIntercepts[2]
   */
  int CellInterfaceType = 2; // pure cell

private:
  /**
   * Retrieved from input for quick access
   */
  vtkUnsignedCharArray* InGhostArray = nullptr;
  vtkBitArray* InMaskArray = nullptr;

  /**
   * Input parameters retrieved from constructor
   */
  bool PassThroughCellIds = false;
  std::string OriginalCellIdArrayName;

  /**
   * Defines the way the current cell is cut by
   * the interface.
   *
   * CellIntercepts[0] and CellIntercepts[1] describe
   * the distance to the first and second interface
   * (A and B) respectively.
   *
   * CellIntercepts[2] correspond to the type of cell
   * (mixed/pure), like CellInterfaceType.
   *
   * XXX: cache for the "current" cell
   */
  std::vector<double> CellIntercepts;

  /**
   * Defines the normal of the current cell interface.
   * These are 3D coordinates, some coordinates being
   * ignored depending on the dimension of the HTG.
   *
   * XXX: cache for the "current" cell
   */
  std::vector<double> CellNormals;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGeometryImpl_h */
