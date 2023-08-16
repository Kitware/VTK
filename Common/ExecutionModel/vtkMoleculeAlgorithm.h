// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMoleculeAlgorithm
 * @brief   Superclass for algorithms that operate on
 * vtkMolecules
 *
 *
 *
 * vtkMoleculeAlgorithm is a convenience class to make writing algorithms
 * easier. There are some assumptions and defaults made by this class you
 * should be aware of. This class defaults such that your filter will have
 * one input port and one output port. If that is not the case simply change
 * it with SetNumberOfInputPorts etc. See this class constructor for the
 * default. This class also provides a FillInputPortInfo method that by
 * default says that all inputs will be vtkMolecules. If that isn't the case
 * then please override this method in your subclass. You should implement
 * the subclass's algorithm into RequestData( request, inputVec, outputVec).
 */

#ifndef vtkMoleculeAlgorithm_h
#define vtkMoleculeAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkMolecule;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkMoleculeAlgorithm : public vtkAlgorithm
{
public:
  static vtkMoleculeAlgorithm* New();
  vtkTypeMacro(vtkMoleculeAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkMolecule* GetOutput();
  vtkMolecule* GetOutput(int);
  virtual void SetOutput(vtkMolecule* d);
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
  vtkMolecule* GetMoleculeInput(int port);

  ///@{
  /**
   * Set an input of this algorithm. You should not override these
   * methods because they are not the only way to connect a pipeline.
   * Note that these methods support old-style pipeline connections.
   * When writing new code you should use the more general
   * vtkAlgorithm::SetInputConnection().  These methods transform the
   * input index to the input port index, not an index of a connection
   * within a single port.
   */
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);
  ///@}

  ///@{
  /**
   * Add an input of this algorithm.  Note that these methods support
   * old-style pipeline connections.  When writing new code you should
   * use the more general vtkAlgorithm::AddInputConnection().  See
   * SetInputData() for details.
   */
  void AddInputData(vtkDataObject*);
  void AddInputData(int, vtkDataObject*);
  ///@}

protected:
  vtkMoleculeAlgorithm();
  ~vtkMoleculeAlgorithm() override;

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
  vtkMoleculeAlgorithm(const vtkMoleculeAlgorithm&) = delete;
  void operator=(const vtkMoleculeAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
