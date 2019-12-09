/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGhostCellsGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkBitArray;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;
class vtkPointData;

class VTKFILTERSPARALLEL_EXPORT vtkHyperTreeGridGhostCellsGenerator
  : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridGhostCellsGenerator* New();
  vtkTypeMacro(vtkHyperTreeGridGhostCellsGenerator, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, vtkIndent) override;

protected:
  vtkHyperTreeGridGhostCellsGenerator();
  ~vtkHyperTreeGridGhostCellsGenerator() override;

  struct vtkInternals;

  /**
   * For this algorithm the output is a vtkHyperTreeGrid instance
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to extract cells based on thresholded value
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Copies the input to the output, filling memory gaps if present.
   */
  void CopyInputTreeToOutput(vtkHyperTreeGridNonOrientedCursor* inCursor,
    vtkHyperTreeGridNonOrientedCursor* outCursor, vtkPointData* inPointData,
    vtkPointData* outPointData, vtkBitArray* inMask, vtkBitArray* outMask);

  /**
   * Reads the input interface with neighbor processes.
   * This method is built in mirror with vtkHyperTreeGridGhostCellsGenerator::CreateGhostTree
   *
   * @param inCursor Cursor on the current tree to read from the input
   * @param isParent A bit array being produced by this filter,
   * telling if the corresponding node is parent or not. A node is
   * a parent if it is not a leaf. The map of the tracking is stored in indices.
   * For example, if the data array of the input is called inArray,
   * isParent->GetValue(m) equals one if inArray->GetTuple1(indices[m]) is not a leaf.
   * @param indices An array produced by this filter mapping the nodes of the interface with their
   * location in the input data array.
   * @param grid Input vtkHyperTreeGrid used to have the neighborhood profile. This neighborhood
   * profile is tested with the mask parameter to know wether to descend or not in the current hyper
   * tree.
   * @param mask Input parameter which should be shaped as vtkHyperTreeGrid::GetChildMask() of the
   * input. This parameter is used to only descend on the interface with the other processes.
   * @param pos This parameter will be equal to the number of nodes in the hyper tree to send to the
   * other processes.
   */
  void ExtractInterface(vtkHyperTreeGridNonOrientedCursor* inCursor, vtkBitArray* isParent,
    std::vector<vtkIdType>& indices, vtkHyperTreeGrid* grid, unsigned int mask, vtkIdType& pos);

  /**
   * Creates a ghost tree in the output. It is built in mirror with
   * vtkHyperTreeGridGhostCellsGenerator::ExtractInterface.
   *
   * @param outCursor Cursor on the output tree that will create the hyper tree.
   * @param isParent Input vtkBitArray produced by a neighbor process to tell if the current node is
   * a leaf or not.
   * @param indices Output array mapping the created nodes to their position in the output data
   * arrays.
   * @param pos Parameter which should be left untouched, it is used to keep track of the number of
   * inserted data.
   */
  vtkIdType CreateGhostTree(vtkHyperTreeGridNonOrientedCursor* outCursor, vtkBitArray* isParent,
    vtkIdType* indices, vtkIdType&& pos = 0);

  vtkInternals* Internals;

private:
  vtkHyperTreeGridGhostCellsGenerator(const vtkHyperTreeGridGhostCellsGenerator&) = delete;
  void operator=(const vtkHyperTreeGridGhostCellsGenerator&) = delete;
};

#endif /* vtkHyperTreeGridGhostCellsGenerator */
