// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMRInterfaceFilter
 * @brief   A contour filter for vtkOverlappingAMR data
 *
 * This filters convert an overlapping AMR into a composite data of structured grid and
 * proper unstructrured interfaces between grids. This interface perfectly pave the space
 * between the different level of resolution of the AMR grids.
 * It also interpolates data on the interface wherever needed
 *
 * Assumption:
 *  - A single voxel in a non-refined grid is not supposed to have points in common with grid of two
 * other refinements levels.
 *  - A single edge should not be shared between grids of more than two different refinement level.
 *
 * Implementation details of this filter:
 *
 * - Iterate over each grid.
 * - If grid is of the highest refinement, just run a put it into the output.
 * - If not, identify the "interface", which are cells that are neighbors with a more refined cell.
 * - Blank interface cells and keep a note of them, then put the result into the output.
 * - Split each interface cell into multiple unstructured cells to create an actual interface
 * between high resolution grid and low resolution grid.
 * - Interpolate data on the interface wherever needed.
 * - Put the interface into the output
 *
 * The complex part is obviously the splitting of low resolution grid cells into interface cells,
 * The idea is to create pyramids and tetrahedrons out of the cells, by adding points on the edges
 * (from the refined grid) as well as in the center of the faces and center of the voxel.
 *
 * - Iterate over each cells of the interface.
 * - Check each edges of the cell and create the "most refined edges" from it, which correspond to a
 * list of ordered points from refined grids
 * - Check each face of the cell.
 * -- If no edges are split, the face is facing a non-refined grid, then create a simple pyramid
 * using the 4 face point and the voxel center.
 * -- If all edges are split, the face is facing a refined grid, recover the refined point
 * of the refined grids by walking on the edges and create many pyramids between these quads and the
 * voxel center.
 * -- Else, its an "interface" face, create tetrahedrons beween each "split" edges using refined
 * edges, the face center and the voxel center.
 *
 * This creates a perfect paving of the interface, connecting the refined grid with the non refined
 * grid.
 *
 * The data on the interface points also matters. We obviously first take data from the refined grid
 * when available, then data from the non-refined grid. If no data is available (interface face
 * centers and voxel center), then a simple Shepard interpolation is used using all the points on
 * the face or the voxel respectively.
 *
 * @sa vtkOverlappingAMR vtkAMRContourFilter
 */
#ifndef vtkAMRInterfaceFilter_h
#define vtkAMRInterfaceFilter_h

#include "vtkFiltersAMRModule.h" // For export macro
#include "vtkPartitionedDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCallbackCommand;
class vtkCartesianGrid;
class vtkCellArray;
class vtkCellData;
class vtkContourFilter;
class vtkDataSet;
class vtkMergePoints;
class vtkOverlappingAMR;
class vtkPointData;
class vtkUnsignedCharArray;
class VTKFILTERSAMR_EXPORT vtkAMRInterfaceFilter : public vtkPartitionedDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkAMRInterfaceFilter, vtkPartitionedDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAMRInterfaceFilter* New();

protected:
  vtkAMRInterfaceFilter();
  ~vtkAMRInterfaceFilter() override;

  /**
   * Set input to vtkOverlappingAMR
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Implement the AMR contouring logic, see class documentation for details
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkAMRInterfaceFilter(const vtkAMRInterfaceFilter&) = delete;
  void operator=(const vtkAMRInterfaceFilter&) = delete;

  /**
   * Remove all empty partitions from the provided output
   */
  static void CleanupOutput(vtkPartitionedDataSet* output);

  ///@{
  /**
   * Progress Handling
   */
  static void InternalProgressCallbackFunction(
    vtkObject* arg, unsigned long, void* clientdata, void*);
  void InternalProgressCallback(vtkAlgorithm* algorithm);
  ///@}

  // Progress handling
  vtkNew<vtkCallbackCommand> InternalProgressObserver;
  double ProgressFloor = 0.;
  double ProgressCeiling = 1.;
};

VTK_ABI_NAMESPACE_END
#endif // vtkAMRInterfaceFilter_h
