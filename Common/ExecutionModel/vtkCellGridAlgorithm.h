// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridAlgorithm
 * @brief   Superclass for algorithms that produce only polydata as output
 *
 *
 * vtkCellGridAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline architecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this class
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be PolyData. If that
 * isn't the case then please override this method in your subclass.
 */

#ifndef vtkCellGridAlgorithm_h
#define vtkCellGridAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCellGrid.h"                   // makes things a bit easier
#include "vtkCommonExecutionModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkCellAttribute;
class vtkCellGrid;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkCellGridAlgorithm : public vtkAlgorithm
{
public:
  static vtkCellGridAlgorithm* New();
  vtkTypeMacro(vtkCellGridAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkCellGrid* GetOutput();
  vtkCellGrid* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);
  ///@}

  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // this method is not recommended for use, but lots of old style filters
  // use it
  vtkDataObject* GetInput();
  vtkDataObject* GetInput(int port);
  vtkCellGrid* GetPolyDataInput(int port);

  ///@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);
  ///@}

  ///@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(vtkDataObject*);
  void AddInputData(int, vtkDataObject*);
  ///@}

  // clang-format off
  ///@{
  /**
   * Set an input cell-attribute that this algorithm will process.
   * Specifically the \a idx-th cell-attribute for this algorithm
   * (starting from 0) will be taken from the cell-grid at the given
   * \a port and \a connection and must have the given \a name.
   *
   * Note that SetInputAttributeToProcess() simply invokes
   * SetInputArrayToProcess() with a cell-centered array-name;
   * the same information keys are used to mark input attributes
   * as input arrays. This means that you may use ParaView's
   * existing ArrayListDomain to call SetInputArrayToProcess
   * with cell-grid algorithms to indicate cell-attributes rather
   * than vtkAbstractArrays on vtkCellData.
   */
  virtual void SetInputAttributeToProcess(int idx, int port, int connection, const char* name);

  /**
   * Fetch a vtkCellAttribute that matches a cell-centered array
   * specified by calling SetInputAttributeToProcess().
   * (Note that SetInputAttributeToProcess() simply invokes
   * SetInputArrayToProcess() with a cell-centered array-name).
   *
   * If you call a variant that accepts an \a association, it
   * will always be set to FIELD_ASSOCIATION_CELLS upon success.
   */
  vtkCellAttribute* GetInputCellAttributeToProcess(int idx, int connection, vtkInformationVector** inputVector);
  vtkCellAttribute* GetInputCellAttributeToProcess(int idx, int connection, vtkInformationVector** inputVector, int& association);
  vtkCellAttribute* GetInputCellAttributeToProcess(int idx, vtkCellGrid* input);
  vtkCellAttribute* GetInputCellAttributeToProcess(int idx, vtkCellGrid* input, int& association);
  ///@}
  // clang-format on

protected:
  vtkCellGridAlgorithm();
  ~vtkCellGridAlgorithm() override;

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
  vtkCellGridAlgorithm(const vtkCellGridAlgorithm&) = delete;
  void operator=(const vtkCellGridAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
