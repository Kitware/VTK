// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkGenerateGlobalIds
 * @brief generates global point and cell ids.
 *
 * vtkGenerateGlobalIds generates global point and cell ids. This filter also
 * generated ghost-point information, flagging duplicate points appropriately.
 * vtkGenerateGlobalIds works across all blocks in the input datasets and across
 * all ranks.
 */

#ifndef vtkGenerateGlobalIds_h
#define vtkGenerateGlobalIds_h

#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLELDIY2_EXPORT vtkGenerateGlobalIds : public vtkPassInputTypeAlgorithm
{
public:
  static vtkGenerateGlobalIds* New();
  vtkTypeMacro(vtkGenerateGlobalIds, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the tolerance to use to identify coincident points. 0 means the
   * points should be exactly identical.
   *
   * Default is 0.
   */
  vtkSetClampMacro(Tolerance, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkGenerateGlobalIds();
  ~vtkGenerateGlobalIds() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkGenerateGlobalIds(const vtkGenerateGlobalIds&) = delete;
  void operator=(const vtkGenerateGlobalIds&) = delete;

  vtkMultiProcessController* Controller;
  double Tolerance;
};

VTK_ABI_NAMESPACE_END
#endif
