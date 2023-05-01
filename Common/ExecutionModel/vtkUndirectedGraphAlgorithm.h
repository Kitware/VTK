// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkUndirectedGraphAlgorithm
 * @brief   Superclass for algorithms that produce undirected graph as output
 *
 *
 * vtkUndirectedGraphAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline edgehitecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this class
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be Graph. If that
 * isn't the case then please override this method in your subclass.
 *
 * @par Thanks:
 * Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
 * Sandia National Laboratories for their help in developing this class.
 */

#ifndef vtkUndirectedGraphAlgorithm_h
#define vtkUndirectedGraphAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkUndirectedGraph.h"            // makes things a bit easier

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkUndirectedGraphAlgorithm : public vtkAlgorithm
{
public:
  static vtkUndirectedGraphAlgorithm* New();
  vtkTypeMacro(vtkUndirectedGraphAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkUndirectedGraph* GetOutput() { return this->GetOutput(0); }
  vtkUndirectedGraph* GetOutput(int index);

  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(vtkDataObject* obj) { this->SetInputData(0, obj); }
  void SetInputData(int index, vtkDataObject* obj);

protected:
  vtkUndirectedGraphAlgorithm();
  ~vtkUndirectedGraphAlgorithm() override;

  // convenience method
  virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkUndirectedGraphAlgorithm(const vtkUndirectedGraphAlgorithm&) = delete;
  void operator=(const vtkUndirectedGraphAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
