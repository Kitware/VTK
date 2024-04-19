// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPipelineSize
 * @brief   compute the memory required by a pipeline
 */

#ifndef vtkPipelineSize_h
#define vtkPipelineSize_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkObject.h"
VTK_ABI_NAMESPACE_BEGIN
class vtkAlgorithm;

class VTKFILTERSPARALLEL_EXPORT vtkPipelineSize : public vtkObject
{
public:
  static vtkPipelineSize* New();
  vtkTypeMacro(vtkPipelineSize, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Compute an estimate of how much memory a pipeline will require in
   * kibibytes (1024 bytes) This is only an estimate and the
   * calculations in here do not take into account the specifics of many
   * sources and filters.
   */
  unsigned long GetEstimatedSize(vtkAlgorithm* input, int inputPort, int connection);

  /**
   * Determine how many subpieces a mapper should use to fit a target memory
   * limit. The piece and numPieces can be queried from the mapper using
   * `vtkPolyDataMapper::GetPiece` and
   * `vtkPolyDataMapper::GetNumberOfSubPieces`.
   */
  unsigned long GetNumberOfSubPieces(
    unsigned long memoryLimit, vtkAlgorithm* mapper, int piece, int numPieces);

protected:
  vtkPipelineSize() = default;
  void GenericComputeSourcePipelineSize(vtkAlgorithm* src, int outputPort, unsigned long size[3]);
  void ComputeSourcePipelineSize(vtkAlgorithm* src, int outputPort, unsigned long size[3]);
  void ComputeOutputMemorySize(
    vtkAlgorithm* src, int outputPort, unsigned long* inputSize, unsigned long size[2]);
  void GenericComputeOutputMemorySize(
    vtkAlgorithm* src, int outputPort, unsigned long* inputSize, unsigned long size[2]);

private:
  vtkPipelineSize(const vtkPipelineSize&) = delete;
  void operator=(const vtkPipelineSize&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
