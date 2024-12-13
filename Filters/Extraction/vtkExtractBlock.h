// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkExtractBlock
 * @brief extracts blocks from a vtkDataObjectTree subclass.
 *
 * vtkExtractBlock is a filter that extracts blocks from a vtkDataObjectTree
 * subclass such as vtkPartitionedDataSet, vtkPartitionedDataSetCollection, etc.
 * using their composite-ids (also called flat-index).
 *
 * The composite-id can be obtained by performing a pre-order traversal of the
 * tree (including empty nodes). For example, consider a tree with nodes named
 * `A(B (D, E), C(F, G))`.  Pre-order traversal yields: `A, B, D, E, C, F, G`;
 * hence, composite-id of `A` is `0`, while index of `C` is `4`.
 *
 * `0` identifies the root-node. Thus, choosing `0` will result in the entire
 * input dataset being passed to the output.
 *
 * @sa vtkGroupDataSetsFilter vtkMergeBlocks
 */

#ifndef vtkExtractBlock_h
#define vtkExtractBlock_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObjectTreeIterator;
class vtkPartitionedDataSet;
class vtkMultiBlockDataSet;
class vtkDataObjectTree;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractBlock : public vtkPassInputTypeAlgorithm
{
  class vtkSet;

public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkExtractBlock* New();
  vtkTypeMacro(vtkExtractBlock, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@{

  ///@{
  /**
   * Select the block indices to extract.  Each node in the multi-block tree
   * is identified by an \c index. The index can be obtained by performing a
   * preorder traversal of the tree (including empty nodes). eg. A(B (D, E),
   * C(F, G)).  Inorder traversal yields: A, B, D, E, C, F, G Index of A is
   * 0, while index of C is 4. (Note: specifying node 0 means the input is
   * copied to the output.)
   */
  void AddIndex(unsigned int index);
  void RemoveIndex(unsigned int index);
  void RemoveAllIndices();
  ///@}

  ///@{
  /**
   * When set, the output multiblock dataset will be pruned to remove empty
   * nodes. On by default.
   *
   * This has no effect for vtkPartitionedDataSetCollection.
   */
  vtkSetMacro(PruneOutput, vtkTypeBool);
  vtkGetMacro(PruneOutput, vtkTypeBool);
  vtkBooleanMacro(PruneOutput, vtkTypeBool);
  ///@}

  ///@{
  /**
   * This is used only when PruneOutput is ON. By default, when pruning the
   * output i.e. remove empty blocks, if node has only 1 non-null child block,
   * then that node is removed. To preserve these parent nodes, set this flag to
   * true. Off by default.
   *
   * This has no effect for vtkPartitionedDataSetCollection.
   */
  vtkSetMacro(MaintainStructure, vtkTypeBool);
  vtkGetMacro(MaintainStructure, vtkTypeBool);
  vtkBooleanMacro(MaintainStructure, vtkTypeBool);
  ///@}

protected:
  vtkExtractBlock();
  ~vtkExtractBlock() override;

  /**
   * Internal key, used to avoid pruning of a branch.
   */
  static vtkInformationIntegerKey* DONT_PRUNE();

  /// Implementation of the algorithm.
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /// Extract subtree
  void CopySubTree(vtkDataObjectTreeIterator* loc, vtkDataObjectTree* output,
    vtkDataObjectTree* input, vtkSet& activeIndices);
  bool Prune(vtkMultiBlockDataSet* mblock);
  bool Prune(vtkPartitionedDataSet* mpiece);
  bool Prune(vtkDataObject* branch);

  vtkTypeBool PruneOutput;
  vtkTypeBool MaintainStructure;

private:
  vtkExtractBlock(const vtkExtractBlock&) = delete;
  void operator=(const vtkExtractBlock&) = delete;
  vtkSet* Indices;
};

VTK_ABI_NAMESPACE_END
#endif
