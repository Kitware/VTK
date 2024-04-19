// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredGridPartitioner
 *
 *
 *  A concrete implementation of vtkMultiBlockDataSetAlgorithm that provides
 *  functionality for partitioning a VTK structured grid dataset. The partition-
 *  ing method used is Recursive Coordinate Bisection (RCB) where each time the
 *  longest dimension is split.
 *
 * @sa
 *  vtkUniformGridPartitioner vtkRectilinearGridPartitioner
 */

#ifndef vtkStructuredGridPartitioner_h
#define vtkStructuredGridPartitioner_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;
class vtkInformationVector;
class vtkIndent;
class vtkStructuredGrid;
class vtkPoints;

class VTKFILTERSGEOMETRY_EXPORT vtkStructuredGridPartitioner : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkStructuredGridPartitioner* New();
  vtkTypeMacro(vtkStructuredGridPartitioner, vtkMultiBlockDataSetAlgorithm);
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
  /**
   * Set/Get & boolean macro for the DuplicateNodes property.
   */
  vtkGetMacro(DuplicateNodes, vtkTypeBool);
  vtkSetMacro(DuplicateNodes, vtkTypeBool);
  vtkBooleanMacro(DuplicateNodes, vtkTypeBool);
  ///@}

protected:
  vtkStructuredGridPartitioner();
  ~vtkStructuredGridPartitioner() override;

  /**
   * Extracts the coordinates of the sub-grid from the whole grid.
   */
  vtkPoints* ExtractSubGridPoints(vtkStructuredGrid* wholeGrid, int subext[6]);

  // Standard Pipeline methods
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int NumberOfPartitions;
  int NumberOfGhostLayers;
  vtkTypeBool DuplicateNodes;

private:
  vtkStructuredGridPartitioner(const vtkStructuredGridPartitioner&) = delete;
  void operator=(const vtkStructuredGridPartitioner&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkStructuredGridPartitioner_h */
