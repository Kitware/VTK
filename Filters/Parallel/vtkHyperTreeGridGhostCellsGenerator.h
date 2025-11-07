// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridGhostCellsGenerator
 * @brief   Generate ghost cells for distributed vtkHyperTreeGrids
 *
 * This filter generates ghost cells for vtkHyperTreeGrid type data.
 * It can also take a vtkPartitionedDataSet composed of HyperTreeGrid partition,
 * where each process has a single non-null partition. In case of a PartitionedDataSet, the output
 * structure is identical to the input structure.
 *
 * This filter produces ghost hyper trees at the interfaces between different processes,
 * only composed of the nodes and leaves at this interface to avoid data waste.
 *
 * This filter should be used in a multi-processes environment, and is only required if wanting to
 * filter a vtkHyperTreeGrid with algorithms using Von Neumann or Moore supercursors afterwards.
 *
 * All processes should have a single HTG with a correct extent, even if it does not contain any
 * actual unmasked cells.
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
#include "vtkWeakPointer.h" // for vtkWeakPointer

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

  ///@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkMultiProcessController* GetController();
  ///@}

protected:
  vtkHyperTreeGridGhostCellsGenerator();
  ~vtkHyperTreeGridGhostCellsGenerator() override;

  struct vtkInternals;

  /**
   * Input must be either HTG or vtkPartitionnedDataSet composed of HTG partitions.
   */
  int FillInputPortInformation(int, vtkInformation*) override;

  /**
   * Override RequestData, to make sure every HTG piece can be processed, hence avoiding that one
   * rank waits for the others which will actually never enter the filter.
   */
  int RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Main routine to generate ghost cells using information
   * from the neighboring HTGs.
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

private:
  vtkHyperTreeGridGhostCellsGenerator(const vtkHyperTreeGridGhostCellsGenerator&) = delete;
  void operator=(const vtkHyperTreeGridGhostCellsGenerator&) = delete;

  /**
   * Exchange HTG metadata between processes
   * Duplicated from vtkHyperTreeGridRedistribute, to be merged when both filters are moved
   * to the same vtk module (currently vtkHyperTreeGridRedistribute is in VTK::ParallelMPI,
   * and vtkHyperTreeGridGhostCellsGenerator is in VTK::Parallel).
   */
  void ExchangeHTGMetadata(vtkHyperTreeGrid* inputHTG);

  vtkWeakPointer<vtkMultiProcessController> Controller;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGhostCellsGenerator */
