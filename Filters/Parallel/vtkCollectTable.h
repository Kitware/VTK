// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCollectTable
 * @brief   Collect distributed table.
 *
 * This filter has code to collect a table from across processes onto node 0.
 * Collection can be turned on or off using the "PassThrough" flag.
 */

#ifndef vtkCollectTable_h
#define vtkCollectTable_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkTableAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;
class vtkSocketController;

class VTKFILTERSPARALLEL_EXPORT vtkCollectTable : public vtkTableAlgorithm
{
public:
  static vtkCollectTable* New();
  vtkTypeMacro(vtkCollectTable, vtkTableAlgorithm);
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
   * When this filter is being used in client-server mode,
   * this is the controller used to communicate between
   * client and server.  Client should not set the other controller.
   */
  virtual void SetSocketController(vtkSocketController*);
  vtkGetObjectMacro(SocketController, vtkSocketController);
  ///@}

  ///@{
  /**
   * To collect or just copy input to output. Off (collect) by default.
   */
  vtkSetMacro(PassThrough, vtkTypeBool);
  vtkGetMacro(PassThrough, vtkTypeBool);
  vtkBooleanMacro(PassThrough, vtkTypeBool);
  ///@}

protected:
  vtkCollectTable();
  ~vtkCollectTable() override;

  vtkTypeBool PassThrough;

  // Data generation method
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkMultiProcessController* Controller;
  vtkSocketController* SocketController;

private:
  vtkCollectTable(const vtkCollectTable&) = delete;
  void operator=(const vtkCollectTable&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
