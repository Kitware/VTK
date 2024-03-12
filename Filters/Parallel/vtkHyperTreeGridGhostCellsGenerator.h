// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridGhostCellsGenerator
 * @brief   Generated ghost cells (HyperTree's distributed).
 *
 * This filter generates ghost cells for vtkHyperTreeGrid type data. The input vtkHyperTreeGrid
 * should have hyper trees distributed to a single process. This filter produces ghost hyper trees
 * at the interfaces between different processes, only composed of the nodes and leafs at this
 * interface to avoid data waste.
 *
 * This filter should be used in a multi-processes environment, and is only required if wanting to
 * filter a vtkHyperTreeGrid with algorithms using Von Neumann or Moore supercursors afterwards.
 *
 * @par Thanks:
 * This class was written by Jacques-Bernard Lekien, 2019
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridGhostCellsGenerator_h
#define vtkHyperTreeGridGhostCellsGenerator_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

#include <vector> // For vtkHypertreeGridGhostCellsGenerator::ExtractInterface

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkCellData;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;
class vtkMultiProcessController;
class vtkUnsignedCharArray;

class VTKFILTERSPARALLEL_EXPORT vtkHyperTreeGridGhostCellsGenerator
  : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridGhostCellsGenerator* New();
  vtkTypeMacro(vtkHyperTreeGridGhostCellsGenerator, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkHyperTreeGridGhostCellsGenerator();
  ~vtkHyperTreeGridGhostCellsGenerator() override = default;

  struct vtkInternals;

  /**
   * For this algorithm the output is a vtkHyperTreeGrid instance
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to generate ghost cells using information
   * from the neighboring HTGs.
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively copy the input tree (cell data and mask information)
   * pointed by the cursor to the output. Fills memory gaps if present.
   */
  VTK_DEPRECATED_IN_9_4_0(
    "This method is now defined in an internal class, and cannot be overridden anymore.")
  void CopyInputTreeToOutput(vtkHyperTreeGridNonOrientedCursor* inCursor,
    vtkHyperTreeGridNonOrientedCursor* outCursor, vtkCellData* inCellData, vtkCellData* outCellData,
    vtkBitArray* inMask, vtkBitArray* outMask);

  /**
   * Reads the input interface with neighbor processes.
   * This method is built in mirror with vtkHyperTreeGridGhostCellsGenerator::CreateGhostTree
   */
  VTK_DEPRECATED_IN_9_4_0(
    "This method is now defined in an internal class, and cannot be overridden anymore.")
  void ExtractInterface(vtkHyperTreeGridNonOrientedCursor* inCursor, vtkBitArray* isParent,
    std::vector<vtkIdType>& indices, vtkHyperTreeGrid* grid, unsigned int mask, vtkIdType& pos);

  /**
   * Creates a ghost tree in the output. It is built in mirror with
   * vtkHyperTreeGridGhostCellsGenerator::ExtractInterface.
   */
  VTK_DEPRECATED_IN_9_4_0(
    "This method is now defined in an internal class, and cannot be overridden anymore.")
  vtkIdType CreateGhostTree(vtkHyperTreeGridNonOrientedCursor* outCursor, vtkBitArray* isParent,
    vtkIdType* indices, vtkIdType&& pos = 0);

private:
  vtkHyperTreeGridGhostCellsGenerator(const vtkHyperTreeGridGhostCellsGenerator&) = delete;
  void operator=(const vtkHyperTreeGridGhostCellsGenerator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGhostCellsGenerator */
