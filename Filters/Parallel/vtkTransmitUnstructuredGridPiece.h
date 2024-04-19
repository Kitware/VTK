// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTransmitRectilinearGridPiece
 * @brief   Redistributes data produced
 * by serial readers
 *
 *
 * This filter can be used to redistribute data from producers that can't
 * produce data in parallel. All data is produced on first process and
 * the distributed to others using the multiprocess controller.
 */

#ifndef vtkTransmitUnstructuredGridPiece_h
#define vtkTransmitUnstructuredGridPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkTransmitUnstructuredGridPiece
  : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkTransmitUnstructuredGridPiece* New();
  vtkTypeMacro(vtkTransmitUnstructuredGridPiece, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Turn on/off creating ghost cells (on by default).
   */
  vtkSetMacro(CreateGhostCells, vtkTypeBool);
  vtkGetMacro(CreateGhostCells, vtkTypeBool);
  vtkBooleanMacro(CreateGhostCells, vtkTypeBool);
  ///@}

protected:
  vtkTransmitUnstructuredGridPiece();
  ~vtkTransmitUnstructuredGridPiece() override;

  // Data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void RootExecute(
    vtkUnstructuredGrid* input, vtkUnstructuredGrid* output, vtkInformation* outInfo);
  void SatelliteExecute(int procId, vtkUnstructuredGrid* output, vtkInformation* outInfo);

  vtkTypeBool CreateGhostCells;
  vtkMultiProcessController* Controller;

private:
  vtkTransmitUnstructuredGridPiece(const vtkTransmitUnstructuredGridPiece&) = delete;
  void operator=(const vtkTransmitUnstructuredGridPiece&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
