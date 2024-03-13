// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAlgorithmOutput
 * @brief   Proxy object to connect input/output ports.
 *
 * vtkAlgorithmOutput is a proxy object returned by the GetOutputPort
 * method of vtkAlgorithm.  It may be passed to the
 * SetInputConnection, AddInputConnection, or RemoveInputConnection
 * methods of another vtkAlgorithm to establish a connection between
 * an output and input port.  The connection is not stored in the
 * proxy object: it is simply a convenience for creating or removing
 * connections.
 */

#ifndef vtkAlgorithmOutput_h
#define vtkAlgorithmOutput_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkAlgorithm;

class VTKCOMMONEXECUTIONMODEL_EXPORT VTK_MARSHALAUTO vtkAlgorithmOutput : public vtkObject
{
public:
  static vtkAlgorithmOutput* New();
  vtkTypeMacro(vtkAlgorithmOutput, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetIndex(int index);
  int GetIndex() const;

  vtkAlgorithm* GetProducer() const;
  void SetProducer(vtkAlgorithm* producer);

protected:
  vtkAlgorithmOutput();
  ~vtkAlgorithmOutput() override;

  int Index;
  vtkAlgorithm* Producer;

private:
  vtkAlgorithmOutput(const vtkAlgorithmOutput&) = delete;
  void operator=(const vtkAlgorithmOutput&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
