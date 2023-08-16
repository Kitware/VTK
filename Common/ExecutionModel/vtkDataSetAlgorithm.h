// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataSetAlgorithm
 * @brief   Superclass for algorithms that produce output of the same type as input
 *
 * vtkDataSetAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline architecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this classes
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be DataSet. If that isn't
 * the case then please override this method in your subclass. This class
 * breaks out the downstream requests into separate functions such as
 * RequestDataObject RequestData and RequestInformation. The default
 * implementation of RequestDataObject will create an output data of the
 * same type as the input.
 */

#ifndef vtkDataSetAlgorithm_h
#define vtkDataSetAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkImageData;
class vtkPolyData;
class vtkStructuredPoints;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkRectilinearGrid;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkDataSetAlgorithm : public vtkAlgorithm
{
public:
  static vtkDataSetAlgorithm* New();
  vtkTypeMacro(vtkDataSetAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkDataSet* GetOutput();
  vtkDataSet* GetOutput(int);
  ///@}

  /**
   * Get the input data object. This method is not recommended for use, but
   * lots of old style filters use it.
   */
  vtkDataObject* GetInput();

  /**
   * Get the output as vtkPolyData.
   */
  vtkPolyData* GetPolyDataOutput();

  /**
   * Get the output as vtkStructuredPoints.
   */
  vtkStructuredPoints* GetStructuredPointsOutput();

  /**
   * Get the output as vtkStructuredPoints.
   */
  vtkImageData* GetImageDataOutput();

  /**
   * Get the output as vtkStructuredGrid.
   */
  vtkStructuredGrid* GetStructuredGridOutput();

  /**
   * Get the output as vtkUnstructuredGrid.
   */
  vtkUnstructuredGrid* GetUnstructuredGridOutput();

  /**
   * Get the output as vtkRectilinearGrid.
   */
  vtkRectilinearGrid* GetRectilinearGridOutput();

  ///@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);
  void SetInputData(vtkDataSet*);
  void SetInputData(int, vtkDataSet*);
  ///@}

  ///@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(vtkDataObject*);
  void AddInputData(vtkDataSet*);
  void AddInputData(int, vtkDataSet*);
  void AddInputData(int, vtkDataObject*);
  ///@}

  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

protected:
  vtkDataSetAlgorithm();
  ~vtkDataSetAlgorithm() override = default;

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
   * This is called within ProcessRequest when each filter in the pipeline
   * decides what portion of its input is needed to create the portion of its
   * output that the downstream filter asks for. This happens after the
   * RequestInformation pass of the pipeline execution process.
   */
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
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
  vtkDataSetAlgorithm(const vtkDataSetAlgorithm&) = delete;
  void operator=(const vtkDataSetAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
