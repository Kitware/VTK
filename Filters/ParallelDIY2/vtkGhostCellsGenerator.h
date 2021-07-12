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
 * If `BuildIfRequired` is set to true (which is by default), then the filter will compute ghost
 * based on the value being returned by
 * `vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()` in the downstream streaming
 * pipeline. If not (i.e. `BuildIfRequired` is off), then the max between this latter value and
 * `NumberOfGhostLayers` is being used.
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

  ///@{
  /**
   * Resets parameter.
   */
  ///@}
  virtual void Initialize();

  ///@{
  /**
   * Specify if the filter must generate the ghost cells only if required by
   * the pipeline.
   * If false, ghost cells are computed even if they are not required.
   * Default is TRUE.
   */
  vtkSetMacro(BuildIfRequired, bool);
  vtkGetMacro(BuildIfRequired, bool);
  vtkBooleanMacro(BuildIfRequired, bool);
  ///@}

  ///@{
  /**
   * When BuildIfRequired is `false`, this can be used to set the number
   * of ghost layers to generate. Note, if the downstream pipeline requests more
   * ghost levels than the number specified here, then the filter will generate
   * those extra ghost levels as needed. Accepted values are in the interval
   * [1, VTK_INT_MAX].
   */
  vtkGetMacro(NumberOfGhostLayers, int);
  vtkSetClampMacro(NumberOfGhostLayers, int, 1, VTK_INT_MAX);
  ///@}

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
  bool BuildIfRequired;

private:
  vtkGhostCellsGenerator(const vtkGhostCellsGenerator&) = delete;
  void operator=(const vtkGhostCellsGenerator&) = delete;
};

#endif
