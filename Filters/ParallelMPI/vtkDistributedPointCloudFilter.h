/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedPointCloudFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class vtkDistributedPointCloudFilter
 * @brief Distributes points among MPI processors.
 *
 * This filter distributes points among processors into spatially
 * contiguous vtkPointSet, containing an equivalent number of points.
 * Algorithm : Point cloud is recursively splitted in two, among MPI groups.
 * Note Input cells are ignored.
 *
 * @par Thanks:
 * This class has been written by Kitware SAS from an initial work made by
 * Aymeric Pelle from Universite de Technologie de Compiegne, France,
 * and Laurent Colombet and Thierry Carrard from Commissariat a l'Energie
 * Atomique (CEA/DIF).
 */

#ifndef vtkDistributedPointCloudFilter_h
#define vtkDistributedPointCloudFilter_h

#include "vtkFiltersParallelMPIModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

#include <vector> // for vector

class vtkMultiProcessController;
class vtkPointSet;

struct KdTreeBuildRound
{
  vtkMultiProcessController* controller;
  int np;
  int rank;
};

class VTKFILTERSPARALLELMPI_EXPORT vtkDistributedPointCloudFilter : public vtkPointSetAlgorithm
{
public:
  static vtkDistributedPointCloudFilter* New();
  vtkTypeMacro(vtkDistributedPointCloudFilter, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkDistributedPointCloudFilter();
  ~vtkDistributedPointCloudFilter() = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Optimize bounding box following this rules:
   * - no intersection of bounding box of different MPI nodes
   * - same amount of point inside bounding box of each MPI nodes.
   *
   * Return false if input pointSet is nullptr or if no communicator was found.
   * Return true otherwise.
   */
  bool OptimizeBoundingBox(vtkPointSet* poly, double bounds[6]);

  /**
   * Initialize KdTreeRound: creates subControllers from Controller.
   * Delete old values if any.
   * Return false if KdTree cannot be initialized.
   */
  bool InitializeKdTree();

private:
  vtkDistributedPointCloudFilter(const vtkDistributedPointCloudFilter&) = delete;
  void operator=(const vtkDistributedPointCloudFilter&) = delete;

  vtkMultiProcessController* Controller;

  std::vector<KdTreeBuildRound> KdTreeRound;
};

#endif
