/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverlappingCellsDetector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkOverlappingCellsDetector
 * @brief Exposes how many cells each cell of the input collide.
 *
 * This filter performs a cell collision detection between the cells of the input.
 * This detection takes the form of a cell array of double. Its name can be
 * reached from the static string attribute
 * vtkOverlappingCellsDetector::NumberOfOverlapsPerCell.
 *
 * To detect collisions, coarse bounding spheres are estimated for each cell of the input.
 * The center of those spheres is stored in a point cloud which is used to find potential
 * colliding cells candidates, querying with twice the bounding sphere radius to ensure
 * we do not miss other bounding sphere centers. Duplicate intersections might appear during
 * this process, so a sphere id map is stored to avoid adding already added overlapping cell ids.
 *
 * This filter works in a multi-process environment. When so, each cell of the input
 * whose bounding sphere and bounding box intersects another process is added in
 * a temporary `vtkUnstructuredGrid` being sent to this process. Cell collision
 * is then performed, and the collision id map is sent back. This map is then
 * read to look if any of those cells were not already counted (local process
 * could have spotted the same collision from
 * the cells sent by the other process indeed). One cell id collision map is stored
 * per neighbor process to avoid cell id collision.
 *
 * The user can set a `Tolerance` parameter. It is set by default to zero. When
 * it is equal to zero or is lower to floating point precision,
 * then floating point precision is used to compute cell
 * overlaps. If it is not set to zero, then each cells are deflated by `0.5 *
 * Tolerance` before the overlaps are computed. The deflation is computed using
 * `vtkCell::Inflate` with a negative parameter.
 */

#ifndef vtkOverlappingCellsDetector_h
#define vtkOverlappingCellsDetector_h

#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkPassInputTypeAlgorithm.h"

#include "vtkBoundingBox.h" // For DetectOverlappingCells

#include <set>           // For DetectOverlappingCells
#include <unordered_map> // For DetectOverlappingCells
#include <vector>        // For DetectOverlappingCells

class vtkDataSet;
class vtkMultiProcessController;
class vtkPointSet;

class VTKFILTERSPARALLELDIY2_EXPORT vtkOverlappingCellsDetector : public vtkPassInputTypeAlgorithm
{
public:
  static vtkOverlappingCellsDetector* New();
  vtkTypeMacro(vtkOverlappingCellsDetector, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Getter / Setter for the Tolerance parameter.
   */
  vtkGetMacro(Tolerance, double);
  vtkSetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Getter / Setter for the name of the output array counting cell collisions.
   * This array is a cell array.
   */
  vtkGetStringMacro(NumberOfOverlapsPerCellArrayName);
  vtkSetStringMacro(NumberOfOverlapsPerCellArrayName);
  ///@}

protected:
  vtkOverlappingCellsDetector();
  ~vtkOverlappingCellsDetector() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Main pipeline. Performs cell collision detection in a MPI aware environment.
   */
  int ExposeOverlappingCellsAmongBlocks(std::vector<vtkPointSet*>& outputs);

  /**
   * Method performing the cell detection.
   * There are 2 main types of inputs: query inputs, as well as inputs where to search.
   * Points in point clouds represent bounding spheres of corresponding cell data set.
   * Each point is associated with a radius.
   * Bounding boxes must match the bounding boxes of corresponding cells in data sets.
   *
   * The algorithm goes as follows:
   * - For each query point in queryPointCloud, a neighborhood is searched in pointCloud
   * - For each neighbor found, a collision test is performed between query cell associated
   *   to query point and each cells associated with points found in the neighborhood.
   * - If the test is positive, arrays of double associated with both datasets are
   *   incremented
   *
   * Last input CollisionListMaps is here to store the list of intersected cell ids from the query,
   * mapped to the id of input cellDataSet. This object is used to avoid double counting
   * collisions when sending back collision information to every block.
   *
   * This function can be called with queryCellDataSet and queryDataSet pointing to the same
   * object in memory.
   *
   * Precondition: cellDataSet MUST have the cell array named NumberOfOverlapsPerCellArrayName()
   */
  bool DetectOverlappingCells(vtkDataSet* queryCellDataSet, vtkPointSet* queryPointCloud,
    const std::vector<vtkBoundingBox>& queryCellBoundingBoxes, vtkDataSet* cellDataSet,
    vtkPointSet* pointCloud, const std::vector<vtkBoundingBox>& cellBoundingBoxes,
    std::unordered_map<vtkIdType, std::set<vtkIdType>>& collisionListMap,
    bool updateProgress = false);

  /**
   * Local controller.
   */
  vtkMultiProcessController* Controller;

  /**
   * Output cell scalar field counting the number of cells that each cell was found to collide.
   */
  char* NumberOfOverlapsPerCellArrayName;

  /**
   * Tolerance for overlap detections. If its value is lower than floating point
   * precision, then floating point
   * precision is used as bound error for overlaps. If not, then cells are
   * deflated by 0.5 * Tolerance before checking the overlaps. Deflating a cell
   * by `x` means translating inward its edges / faces by a distance `x` following
   * the edge's / face's normal direction. `vtkCell::Inflate` is used with a
   * negative parameter.
   */
  double Tolerance;

private:
  vtkOverlappingCellsDetector(const vtkOverlappingCellsDetector&) = delete;
  void operator=(const vtkOverlappingCellsDetector&) = delete;
};

#endif
