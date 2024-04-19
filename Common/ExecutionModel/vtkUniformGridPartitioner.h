// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformGridPartitioner
 *
 *
 *  A concrete implementation of vtkMultiBlockDataSetAlgorithm that provides
 *  functionality for partitioning a uniform grid. The partitioning method
 *  that is used is Recursive Coordinate Bisection (RCB) where each time
 *  the longest dimension is split.
 *
 * @sa
 * vtkStructuredGridPartitioner vtkRectilinearGridPartitioner
 */

#ifndef vtkUniformGridPartitioner_h
#define vtkUniformGridPartitioner_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;
class vtkInformationVector;
class vtkIndent;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkUniformGridPartitioner
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkUniformGridPartitioner* New();
  vtkTypeMacro(vtkUniformGridPartitioner, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& oss, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get macro for the number of subdivisions.
   */
  vtkGetMacro(NumberOfPartitions, int);
  vtkSetMacro(NumberOfPartitions, int);
  ///@}

  ///@{
  /**
   * Set/Get macro for the number of ghost layers.
   */
  vtkGetMacro(NumberOfGhostLayers, int);
  vtkSetMacro(NumberOfGhostLayers, int);
  ///@}

  ///@{
  vtkGetMacro(DuplicateNodes, vtkTypeBool);
  vtkSetMacro(DuplicateNodes, vtkTypeBool);
  vtkBooleanMacro(DuplicateNodes, vtkTypeBool);
  ///@}

protected:
  vtkUniformGridPartitioner();
  ~vtkUniformGridPartitioner() override;

  // Standard Pipeline methods
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int NumberOfPartitions;
  int NumberOfGhostLayers;
  vtkTypeBool DuplicateNodes;

private:
  vtkUniformGridPartitioner(const vtkUniformGridPartitioner&) = delete;
  void operator=(const vtkUniformGridPartitioner&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* VTKUNIFORMGRIDPARTITIONER_H_ */
