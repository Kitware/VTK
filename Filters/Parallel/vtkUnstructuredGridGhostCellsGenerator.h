/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridGhostCellsGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkUnstructuredGridGhostCellsGenerator
 * @brief   Builds ghost cells for a distributed unstructured grid dataset.
 *
 * This filter is a serial implementation of the vtkPUnstructuredGridGhostCellsGenerator
 * filter with the intent that it can be used in non-MPI builds. Both the serial and
 * parallel version act as a "pass-through" filter when run in serial. The combination
 * of these filters serves to unify the API for serial and parallel builds.
 *
 * @sa
 * vtkDistributedDataFilter
 * vtkPUnstructuredGridGhostCellsGenerator
 *
 *
*/

#ifndef vtkUnstructuredGridGhostCellsGenerator_h
#define vtkUnstructuredGridGhostCellsGenerator_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkMultiProcessController;
class vtkUnstructuredGrid;
class vtkUnstructuredGridBase;

class VTKFILTERSPARALLEL_EXPORT vtkUnstructuredGridGhostCellsGenerator:
  public vtkUnstructuredGridAlgorithm
{
  vtkTypeMacro(vtkUnstructuredGridGhostCellsGenerator, vtkUnstructuredGridAlgorithm);

public:
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkUnstructuredGridGhostCellsGenerator *New();

  //@{
  /**
   * Specify if the filter must take benefit of global point ids if they exist.
   * If false, point coordinates are used. Default is TRUE.
   */
  vtkSetMacro(UseGlobalPointIds, bool);
  vtkGetMacro(UseGlobalPointIds, bool);
  vtkBooleanMacro(UseGlobalPointIds, bool);
  //@}

  //@{
  /**
   * Specify the name of the global point ids data array if the GlobalIds
   * attribute array is not set. Default is "GlobalNodeIds".
   */
  vtkSetStringMacro(GlobalPointIdsArrayName);
  vtkGetStringMacro(GlobalPointIdsArrayName);
  //@}

  //@{
  /**
   * Specify if the data has global cell ids.
   * If more than one layer of ghost cells is needed, global cell ids are
   * necessary. If global cell ids are not provided, they will be computed
   * internally.
   * If false, global cell ids will be computed, then deleted afterwards.
   * Default is FALSE.
   */
  vtkSetMacro(HasGlobalCellIds, bool);
  vtkGetMacro(HasGlobalCellIds, bool);
  vtkBooleanMacro(HasGlobalCellIds, bool);
  //@}

  //@{
  /**
   * Specify the name of the global cell ids data array if the GlobalIds
   * attribute array is not set. Default is "GlobalNodeIds".
   */
  vtkSetStringMacro(GlobalCellIdsArrayName);
  vtkGetStringMacro(GlobalCellIdsArrayName);
  //@}

  //@{
  /**
   * Specify if the filter must generate the ghost cells only if required by
   * the pipeline.
   * If false, ghost cells are computed even if they are not required.
   * Default is TRUE.
   */
  vtkSetMacro(BuildIfRequired, bool);
  vtkGetMacro(BuildIfRequired, bool);
  vtkBooleanMacro(BuildIfRequired, bool);
  //@}

  //@{
  /**
   * When BuildIfRequired is `false`, this can be used to set the minimum number
   * of ghost levels to generate. Note, if the downstream pipeline requests more
   * ghost levels than the number specified here, then the filter will generate
   * those extra ghost levels as needed. Accepted values are in the interval
   * [1, VTK_INT_MAX].
   */
  vtkSetClampMacro(MinimumNumberOfGhostLevels, int, 1, VTK_INT_MAX);
  vtkGetMacro(MinimumNumberOfGhostLevels, int);
  //@}

protected:
  vtkUnstructuredGridGhostCellsGenerator();
  ~vtkUnstructuredGridGhostCellsGenerator() override;

  int RequestUpdateExtent(
    vtkInformation*,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) override;

  char *GlobalPointIdsArrayName;
  bool UseGlobalPointIds;
  char *GlobalCellIdsArrayName;
  bool HasGlobalCellIds;
  bool BuildIfRequired;
  int MinimumNumberOfGhostLevels;

private:
  vtkUnstructuredGridGhostCellsGenerator(const vtkUnstructuredGridGhostCellsGenerator&) = delete;
  void operator=(const vtkUnstructuredGridGhostCellsGenerator&) = delete;
};

#endif
