// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointSetAlgorithm
 * @brief   Superclass for algorithms that process vtkPointSet input
 *
 * vtkPointSetAlgorithm is a convenience class to make writing algorithms
 * easier. Filter subclasses of vtkPointSetAlgorithm take vtkPointSet (and
 * derived classes) as input to the filter, and produce vtkPointSet as
 * output. (Note that overriding FillInputPortInformation() and
 * FillOutputPortInformation() can be used to change this behavior.)
 *
 * There are some assumptions and defaults made by this class you should be
 * aware of. This class defaults such that your filter will have one input
 * port and one output port. If that is not the case simply change it with
 * SetNumberOfInputPorts() etc. See this classes constructor for the
 * default. To implement a filter, developers should implement the subclass's
 * algorithm in the RequestData(request,inputVec,outputVec) method.
 */

#ifndef vtkPointSetAlgorithm_h
#define vtkPointSetAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkWrappingHints.h"              // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkPointSet;
class vtkPolyData;
class vtkStructuredGrid;
class vtkUnstructuredGrid;

class VTKCOMMONEXECUTIONMODEL_EXPORT VTK_MARSHALAUTO vtkPointSetAlgorithm : public vtkAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing instances of the class.
   */
  static vtkPointSetAlgorithm* New();
  vtkTypeMacro(vtkPointSetAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkPointSet* GetOutput();
  vtkPointSet* GetOutput(int);
  ///@}

  /**
   * Get the output as vtkPolyData.
   */
  vtkPolyData* GetPolyDataOutput();

  /**
   * Get the output as vtkStructuredGrid.
   */
  vtkStructuredGrid* GetStructuredGridOutput();

  /**
   * Get the output as vtkUnstructuredGrid.
   */
  vtkUnstructuredGrid* GetUnstructuredGridOutput();

  ///@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);
  void SetInputData(vtkPointSet*);
  void SetInputData(int, vtkPointSet*);
  ///@}

  ///@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(vtkDataObject*);
  void AddInputData(vtkPointSet*);
  void AddInputData(int, vtkPointSet*);
  void AddInputData(int, vtkDataObject*);
  ///@}

  // This method is not recommended for use, but lots of old style filters
  // use it
  vtkDataObject* GetInput();

  /**
   * See vtkAlgorithm for details.
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

protected:
  vtkPointSetAlgorithm();
  ~vtkPointSetAlgorithm() override = default;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int ExecuteInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  ///@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int ComputeInputUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  ///@}

  virtual int ComputeInputUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkPointSetAlgorithm(const vtkPointSetAlgorithm&) = delete;
  void operator=(const vtkPointSetAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
