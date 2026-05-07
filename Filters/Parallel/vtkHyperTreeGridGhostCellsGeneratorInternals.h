// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkHyperTreeGridGhostCellsGeneratorInternals
 * @brief Internal class for vtkHyperTreeGridGhostCellsGenerator
 *
 * This class provides processing subroutines for vtkHyperTreeGridGhostCellsGenerator.
 * It keeps an internal state shared across routines called sequentially.
 *
 * It should be instantiated by the ProcessTrees method of vtkHyperTreeGridGhostCellsGenerator,
 * after the output HTG has been copied from the input.
 */

#ifndef vtkHyperTreeGridGhostCellsGeneratorInternals_h
#define vtkHyperTreeGridGhostCellsGeneratorInternals_h

#include "vtkAbstractArray.h"
#include "vtkBitArray.h"
#include "vtkDataArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGhostCellsGenerator.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridOrientedCursor.h"
#include "vtkMultiProcessController.h"
#include "vtkSmartPointer.h"

#include <map>
#include <unordered_map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkHyperTreeGridGhostCellsGeneratorInternals
{

public:
  /**
   * @param self reference to the GCG object, used for debug and error prints
   * @param controller reference to the MPI controller used for parallel operations
   * @param inputHTG reference to the input HyperTreeGrid
   * @param outputHTG reference to the output HyperTreeGrid
   */
  vtkHyperTreeGridGhostCellsGeneratorInternals(vtkHyperTreeGridGhostCellsGenerator* self,
    vtkMultiProcessController* controller, vtkHyperTreeGrid* inputHTG, vtkHyperTreeGrid* outputHTG);

  /**
   * Initialize the internal cell data implicit array handler, with the
   * cell arrays as the first entries of as many implicit composite arrays.
   */
  void InitializeCellData();

  /**
   * Subroutine performing an MPI AllReduce operation,
   * filling the vector HyperTreesMapToProcesses where `HyperTreesMapToProcesses[i]` is the rank of
   * the process where the root tree indexed `i` is located.
   */
  void BroadcastTreeLocations();

  /**
   * Compute the index of neighboring trees and record those that belong to other processes and
   * should be sent to become ghost cells.
   */
  void DetermineNeighbors();

  /**
   * Exchange the number of ghost cells to be sent between ranks.
   * Send an array to every other processes containing the number of cells in each tree to be sent.
   * Return 1 if the operation was successful, 0 otherwise.
   */
  int ExchangeSizes();

  /**
   * Routine to send and receive tree decomposition, and mask values if present for each tree.
   * Return 1 if the operation was successful, 0 otherwise
   */
  int ExchangeTreeDecomposition();

  /**
   * Exchange cell data information with the other process to fill in values for ghost cells.
   * Return 1 if the operation was successful, 0 otherwise
   */
  int ExchangeCellData();

  /**
   * ProcessTrees subroutine creating the output ghost array and adding it to the output HTG.
   */
  void FinalizeCellData();

private:
  // Internal structures used for MPI message exchanges
  struct SendBuffer
  {
    SendBuffer()
      : count(0)
      , mask(0)
    {
    }
    vtkIdType count;                // len buffer
    unsigned int mask;              // ghost mask
    std::vector<vtkIdType> indices; // indices for selected cells
    vtkNew<vtkBitArray> isParent;   // decomposition amr tree
    vtkNew<vtkBitArray> isMasked;   // decomposition amr tree
  };

  struct RecvBuffer
  {
    RecvBuffer()
      : count(0)
      , offset(0)
    {
    }
    vtkIdType count;  // len buffer
    vtkIdType offset; // offset in field vector
    std::vector<vtkIdType> indices;
  };

  typedef std::map<unsigned int, SendBuffer> SendTreeBufferMap;
  typedef std::map<unsigned int, SendTreeBufferMap> SendProcessBufferMap;
  typedef std::map<unsigned int, RecvBuffer> RecvTreeBufferMap;
  typedef std::map<unsigned int, RecvTreeBufferMap> RecvProcessBufferMap;

  enum FlagType
  {
    NOT_TREATED,     // process has not been dealth with yet
    INITIALIZE_TREE, // ghost tree has been created, values not filled yet
    INITIALIZE_FIELD // cell data values have been set
  };

  // Associate the process id with its ghost tree processing state
  typedef std::unordered_map<unsigned int, FlagType> FlagMap;

  // Handling receive and send buffer.
  // The structure is as follows:
  // SendBuffer[id] or RecvBuffer[id] == process id of neighbor with whom to communicate buffer
  // SendBuffer[id][jd] or RecvBuffer[id][jd] tells which tree index is being sent.
  SendProcessBufferMap SendBuffer;
  RecvProcessBufferMap RecvBuffer;

  FlagMap Flags;

  std::vector<int> HyperTreesMapToProcesses;

public:
  // Store the values for a single cell data array, composed of two parts
  // * InternalArray is the cell array internal to this HTG (ShallowCopied)
  // * GhostCDBuffer is the buffer with values from ghost cells
  struct CellDataArray
  {
    vtkDataArray* InternalArray;
    vtkDataArray* GhostCDBuffer;
  };
  // All cell data attributes composing a vtkCellData, accessed by name.
  using CellDataAttributes = std::map<std::string, CellDataArray>;

private:
  vtkHyperTreeGridGhostCellsGenerator* Self = nullptr;
  vtkMultiProcessController* Controller = nullptr;
  vtkHyperTreeGrid* InputHTG = nullptr;
  vtkHyperTreeGrid* OutputHTG = nullptr;
  vtkSmartPointer<vtkBitArray> OutputMask;
  CellDataAttributes ImplicitCD;
  vtkIdType NumberOfVertices = 0;
  vtkIdType InitialNumberOfVertices = 0;
};

VTK_ABI_NAMESPACE_END
#endif
