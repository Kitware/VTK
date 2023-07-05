// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPassInputTypeAlgorithm
 * @brief   Superclass for algorithms that produce output of the same type as input
 *
 * vtkPassInputTypeAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline architecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this classes
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be DataObject. If that isn't
 * the case then please override this method in your subclass. This class
 * breaks out the downstream requests into separate functions such as
 * RequestDataObject RequestData and RequestInformation. The default
 * implementation of RequestDataObject will create an output data of the
 * same type as the input.
 */

#ifndef vtkPassInputTypeAlgorithm_h
#define vtkPassInputTypeAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkGraph;
class vtkHyperTreeGrid;
class vtkImageData;
class vtkMolecule;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkStructuredPoints;
class vtkTable;
class vtkUnstructuredGrid;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkPassInputTypeAlgorithm : public vtkAlgorithm
{
public:
  static vtkPassInputTypeAlgorithm* New();
  vtkTypeMacro(vtkPassInputTypeAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkDataObject* GetOutput();
  vtkDataObject* GetOutput(int);
  ///@}

  ///@{
  /**
   * Get the output as a concrete type.
   */
  vtkPolyData* GetPolyDataOutput();
  vtkStructuredPoints* GetStructuredPointsOutput();
  vtkImageData* GetImageDataOutput();
  vtkStructuredGrid* GetStructuredGridOutput();
  vtkUnstructuredGrid* GetUnstructuredGridOutput();
  vtkRectilinearGrid* GetRectilinearGridOutput();
  vtkGraph* GetGraphOutput();
  vtkMolecule* GetMoleculeOutput();
  vtkTable* GetTableOutput();
  vtkHyperTreeGrid* GetHyperTreeGridOutput();
  ///@}

  /**
   * Get the input data object. This method is not recommended for use, but
   * lots of old style filters use it.
   */
  vtkDataObject* GetInput();

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
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(vtkDataObject*);
  void AddInputData(int, vtkDataObject*);
  ///@}

  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

protected:
  vtkPassInputTypeAlgorithm();
  ~vtkPassInputTypeAlgorithm() override = default;

  /**
   * This is called within ProcessRequest when a request asks the
   * algorithm to create empty output data objects. This typically happens
   * early on in the execution of the pipeline. The default behavior is to
   * create an output DataSet of the same type as the input for each
   * output port. This method can be overridden to change the output
   * data type of an algorithm. This happens in the first pass of the pipeline
   * execution process.
   */
  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  /**
   * This is called within ProcessRequest when a request asks for
   * Information. Typically an algorithm provides whatever lightweight
   * information about its output that it can here without doing any
   * lengthy computations. This happens after the RequestDataObject pass of
   * the pipeline execution process.
   */
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called within ProcessRequest when a request ask for temporal
   * information to be updated. This special pass is only used for
   * temporal data, this happened after the RequestInformation pass of the
   * pipeline execution process.
   */
  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called within ProcessRequest when a request ask for meta
   * information to be updated. This special pass is only used for
   * temporal data, this happened after the RequestUpdateTime pass of the
   * pipeline execution process.
   */
  virtual int RequestUpdateTimeDependentInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called within ProcessRequest when each filter in the pipeline
   * decides what portion of its input is needed to create the portion of its
   * output that the downstream filter asks for. This happens after the
   * RequestInformation / RequestUpdateTimeDependentInformation pass of the
   * pipeline execution process.
   */
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called within ProcessRequest when a request asks the algorithm
   * to do its work. This is the method you should override to do whatever the
   * algorithm is designed to do. This happens during the final pass in the
   * pipeline execution process.
   */
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkDataObject* GetInput(int port);

private:
  vtkPassInputTypeAlgorithm(const vtkPassInputTypeAlgorithm&) = delete;
  void operator=(const vtkPassInputTypeAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
