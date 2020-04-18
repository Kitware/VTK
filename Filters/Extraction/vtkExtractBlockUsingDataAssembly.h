/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractBlockUsingDataAssembly.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkExtractBlockUsingDataAssembly
 * @brief pass through only selected datasets in a vtkPartitionedDataSetCollection.
 *
 * vtkExtractBlockUsingDataAssembly is similar to vtkExtractBlock filter for
 * vtkPartitionedDataSetCollection that uses the `vtkDataAssembly` provided by
 * the vtkPartitionedDataSetCollection to determine which datasets to pass
 * through.
 */

#ifndef vtkExtractBlockUsingDataAssembly_h
#define vtkExtractBlockUsingDataAssembly_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersExtractionModule.h" // For export macro

class VTKFILTERSEXTRACTION_EXPORT vtkExtractBlockUsingDataAssembly : public vtkDataObjectAlgorithm
{
public:
  static vtkExtractBlockUsingDataAssembly* New();
  vtkTypeMacro(vtkExtractBlockUsingDataAssembly, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Choose nodes to pass through.
   */
  bool AddNodePath(const char* path);
  void ClearNodePaths();
  //@}

  /**
   * Convenience method to set a single path.
   * This clears any other existing paths.
   */
  void SetNodePath(const char* path);

  //@{
  /**
   * Get currently chosen paths.
   */
  int GetNumberOfPaths() const;
  const char* GetNodePath(int index) const;
  //@}

  //@{
  /**
   * When set to true (default) subtrees for chosen paths are treated as
   * selected.
   */
  vtkSetMacro(SelectSubtrees, bool);
  vtkGetMacro(SelectSubtrees, bool);
  vtkBooleanMacro(SelectSubtrees, bool);
  //@}

  //@{
  /**
   * When set to true (default), the data assembly is pruned to remove
   * branches that were not selected.
   */
  vtkSetMacro(PruneDataAssembly, bool);
  vtkGetMacro(PruneDataAssembly, bool);
  vtkBooleanMacro(PruneDataAssembly, bool);
  //@}
protected:
  vtkExtractBlockUsingDataAssembly();
  ~vtkExtractBlockUsingDataAssembly() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkExtractBlockUsingDataAssembly(const vtkExtractBlockUsingDataAssembly&) = delete;
  void operator=(const vtkExtractBlockUsingDataAssembly&) = delete;

  class vtkInternals;
  vtkInternals* Internals;

  bool SelectSubtrees;
  bool PruneDataAssembly;
};

#endif
