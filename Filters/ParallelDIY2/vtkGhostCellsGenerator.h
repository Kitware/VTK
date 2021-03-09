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
 * If the input is composed of some data sets already owning ghosts, those ghosts are removed from
 * the output and are recomputed. Ghosts in the input are as if they didn't exist.
 *
 * @note Currently, only `vtkImageData`, `vtkRectilinearGrid` and `vtkStructuredGrid` are
 * implemented.
 */

#ifndef vtkGhostCellsGenerator_h
#define vtkGhostCellsGenerator_h

#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkSmartPointer.h"

#include <vector>

class vtkDataSet;
class vtkImageData;
class vtkMultiProcessController;
class vtkPointSet;

class VTKFILTERSPARALLELDIY2_EXPORT vtkGhostCellsGenerator : public vtkPassInputTypeAlgorithm
{
public:
  static vtkGhostCellsGenerator* New();
  vtkTypeMacro(vtkGhostCellsGenerator, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

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
