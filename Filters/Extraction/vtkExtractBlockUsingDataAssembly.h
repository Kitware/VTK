// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkExtractBlockUsingDataAssembly
 * @brief extract blocks from certain composite datasets
 *
 * vtkExtractBlockUsingDataAssembly is intended to extract selected blocks
 * from certain composite datasets. Blocks to extract are selected using
 * selectors. For supported selectors see `vtkDataAssembly::SelectNodes`.
 *
 * The specific data-assembly to use to apply the selectors to determine the
 * blocks to extract is chosen using `vtkExtractBlockUsingDataAssembly::SetAssemblyName`.
 *
 * @section vtkExtractBlockUsingDataAssembly-SupportedDataTypes Supported Data Types
 *
 * This filter accepts `vtkUniformGridAMR`,
 * `vtkMultiBlockDataSet`, and `vtkPartitionedDataSetCollection` (and
 * subclasses). vtkMultiPieceDataSet and vtkPartitionedDataSet are not accepted
 * as inputs since those composite datasets are not comprised of "blocks".
 *
 * For vtkOverlappingAMR, since extracting blocks cannot always guarantee a valid
 * overlapping AMR, this filter generates a `vtkPartitionedDataSetCollection`
 * instead. Any blanking information present in the input vtkOverlappingAMR is
 * also discarded for the same reason.
 *
 * For all other supported input data types, the type is preserved.
 */

#ifndef vtkExtractBlockUsingDataAssembly_h
#define vtkExtractBlockUsingDataAssembly_h

#include "vtkCompositeDataSetAlgorithm.h"
#include "vtkFiltersExtractionModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataAssembly;
class vtkPartitionedDataSetCollection;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractBlockUsingDataAssembly
  : public vtkCompositeDataSetAlgorithm
{
public:
  static vtkExtractBlockUsingDataAssembly* New();
  vtkTypeMacro(vtkExtractBlockUsingDataAssembly, vtkCompositeDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * API to set selectors. Multiple selectors can be added using `AddSelector`.
   * The order in which selectors are specified is not preserved and has no
   * impact on the result.
   *
   * `AddSelector` returns true if the selector was added, false if the selector
   * was already specified and hence not added.
   *
   * @sa vtkDataAssembly::SelectNodes
   */
  bool AddSelector(const char* selector);
  void ClearSelectors();
  ///@}

  /**
   * Convenience method to set a single selector.
   * This clears any other existing selectors.
   */
  void SetSelector(const char* selector);

  ///@{
  /**
   * API to access selectors.
   */
  int GetNumberOfSelectors() const;
  const char* GetSelector(int index) const;
  ///@}

  ///@{
  /**
   * Get/Set the active assembly to use. The chosen assembly is used
   * in combination with the selectors specified to determine which blocks
   * are to be extracted.
   *
   * By default, this is set to
   * vtkDataAssemblyUtilities::HierarchyName().
   */
  vtkSetStringMacro(AssemblyName);
  vtkGetStringMacro(AssemblyName);
  ///@}

  ///@{
  /**
   * When set to true (default) subtrees for chosen paths are treated as
   * selected.
   */
  vtkSetMacro(SelectSubtrees, bool);
  vtkGetMacro(SelectSubtrees, bool);
  vtkBooleanMacro(SelectSubtrees, bool);
  ///@}

  ///@{
  /**
   * When set to true (default), the data assembly is pruned to remove
   * branches that were not selected.
   */
  vtkSetMacro(PruneDataAssembly, bool);
  vtkGetMacro(PruneDataAssembly, bool);
  vtkBooleanMacro(PruneDataAssembly, bool);
  ///@}

protected:
  vtkExtractBlockUsingDataAssembly();
  ~vtkExtractBlockUsingDataAssembly() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkExtractBlockUsingDataAssembly(const vtkExtractBlockUsingDataAssembly&) = delete;
  void operator=(const vtkExtractBlockUsingDataAssembly&) = delete;

  class vtkInternals;
  vtkInternals* Internals;

  bool SelectSubtrees;
  bool PruneDataAssembly;
  char* AssemblyName;
};

VTK_ABI_NAMESPACE_END
#endif
