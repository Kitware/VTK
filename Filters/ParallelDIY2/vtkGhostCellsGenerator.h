/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGhostCellsGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkGhostCellsGenerator
 * @brief Computes ghost cells on vtkCompositeDataSet inputs
 *
 * This filter computes ghost cells between data sets of same types in a `vtkCompositeDataSet`.
 * For example, a `vtkImageData` inside a `vtkCompositeDataSet` will send and receive ghosts only to
 * and from other `vtkImageData`.
 *
 * If the input is a `vtkPartitionedDataSetCollection`, then ghosts are computed per partitioned
 * data set. In other words, ghost are not computed between 2 `vtkDataSet` belonging to 2 different
 * `vtkPartitionedDataSet`, even if they are adjacent.
 *
 * If the input is composed of some data sets already owning ghosts, those ghosts are removed from
 * the output and are recomputed. Ghosts in the input are as if they didn't exist.
 * A ghost cell is to be peeled off if it holds the `CELLDUPLICATE` flag in its ghost bit mask.
 * Similarly, each generated ghost cells from this filter is tagged with `CELLDUPLICATE`, in
 * addition of other tags that could be set (`HIDDENCELL` for instance).
 *
 * If the input is a `vtkUnstructuredGrid`, if the input `vtkPointData` has global ids, then the
 * values of those global ids are used instead of point position in 3D to connect 2 partitions.
 * If not, point position of the outer surface are used to connect them. The precision of such
 * connection is done using numeric precision of the input coordinates.
 *
 * @warning If an input already holds ghosts, the input ghost cells should be tagged as
 * `CELLDUPLICATE` in order for this filter to work properly.
 *
 * @note Currently,`vtkImageData`, `vtkRectilinearGrid`, `vtkStructuredGrid` and
 * `vtkUnstructuredGrid` are implemented.
 */

#ifndef vtkGhostCellsGenerator_h
#define vtkGhostCellsGenerator_h

#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkPassInputTypeAlgorithm.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLELDIY2_EXPORT vtkGhostCellsGenerator : public vtkPassInputTypeAlgorithm
{
public:
  static vtkGhostCellsGenerator* New();
  vtkTypeMacro(vtkGhostCellsGenerator, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  vtkGetMacro(NumberOfGhostLayers, int);
  vtkSetMacro(NumberOfGhostLayers, int);

protected:
  vtkGhostCellsGenerator();
  ~vtkGhostCellsGenerator() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Local controller.
   */
  vtkMultiProcessController* Controller;

  int NumberOfGhostLayers;

private:
  vtkGhostCellsGenerator(const vtkGhostCellsGenerator&) = delete;
  void operator=(const vtkGhostCellsGenerator&) = delete;
};

#endif
