// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRectilinearGridPartitioner
 *
 *
 *  A concrete implementation of vtkMultiBlockDataSetAlgorithm that provides
 *  functionality for partitioning a VTK rectilinear dataset. The partitioning
 *  method used is Recursive Coordinate Bisection (RCB) where each time the
 *  longest dimension is split.
 *
 * @sa
 *  vtkUniformGridPartitioner vtkStructuredGridPartitioner
 */

#ifndef vtkRectilinearGridPartitioner_h
#define vtkRectilinearGridPartitioner_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;
class vtkInformationVector;
class vtkIndent;
class vtkDoubleArray;
class vtkRectilinearGrid;

class VTKFILTERSGEOMETRY_EXPORT vtkRectilinearGridPartitioner : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkRectilinearGridPartitioner* New();
  vtkTypeMacro(vtkRectilinearGridPartitioner, vtkMultiBlockDataSetAlgorithm);
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
  vtkRectilinearGridPartitioner();
  ~vtkRectilinearGridPartitioner() override;

  /**
   * Extracts the coordinates
   */
  void ExtractGridCoordinates(vtkRectilinearGrid* grd, int subext[6], vtkDoubleArray* xcoords,
    vtkDoubleArray* ycoords, vtkDoubleArray* zcoords);

  // Standard Pipeline methods
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int NumberOfPartitions;
  int NumberOfGhostLayers;
  vtkTypeBool DuplicateNodes;

private:
  vtkRectilinearGridPartitioner(const vtkRectilinearGridPartitioner&) = delete;
  void operator=(const vtkRectilinearGridPartitioner&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkRectilinearGridPartitioner_h */
