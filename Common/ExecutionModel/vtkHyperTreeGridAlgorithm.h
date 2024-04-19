// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridAlgorithm
 * @brief   Superclass for algorithms that produce
 * a hyper tree grid as output
 *
 *
 * vtkHyperTreeGridAlgorithm is a base class for hyper tree grid algorithms.
 * This class defaults with one input port and one output port; it must be
 * modified by the concrete derived class if a different behavior is sought.
 * In addition, this class provides a FillOutputPortInfo() method that, by
 * default, specifies that the output is a data object; this
 * must also be modified in concrete subclasses if needed.
 *
 * @par Thanks:
 * This test was written by Philippe Pebay and Charles Law, Kitware 2012
 * This test was rewritten by Philippe Pebay, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridAlgorithm_h
#define vtkHyperTreeGridAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkDataSetAttributes;
class vtkHyperTreeGrid;
class vtkPolyData;
class vtkUnstructuredGrid;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkHyperTreeGridAlgorithm : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkHyperTreeGridAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkDataObject* GetOutput();
  vtkDataObject* GetOutput(int);
  virtual void SetOutput(vtkDataObject*);
  ///@}

  ///@{
  /**
   * Get the output as a hyper tree grid.
   */
  vtkHyperTreeGrid* GetHyperTreeGridOutput();
  vtkHyperTreeGrid* GetHyperTreeGridOutput(int);
  ///@}

  ///@{
  /**
   * Get the output as a polygonal dataset.
   */
  vtkPolyData* GetPolyDataOutput();
  vtkPolyData* GetPolyDataOutput(int);
  ///@}

  ///@{
  /**
   * Get the output as an unstructured grid.
   */
  vtkUnstructuredGrid* GetUnstructuredGridOutput();
  vtkUnstructuredGrid* GetUnstructuredGridOutput(int);
  ///@}

  /**
   * See vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

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

protected:
  vtkHyperTreeGridAlgorithm();
  ~vtkHyperTreeGridAlgorithm() override;

  /**
   * see vtkAlgorithm for details
   */
  int RequestDataObject(
    vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  // convenience method
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  /**
   * Main routine to process individual trees in the grid
   * This is pure virtual method to be implemented by concrete algorithms
   */
  virtual int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) = 0;

  ///@{
  /**
   * Define default input and output port types
   */
  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;
  ///@}

  ///@{
  /**
   * Reference to input and output data
   */
  vtkDataSetAttributes* InData;
  vtkDataSetAttributes* OutData;
  ///@}

  ///@{
  /**
   * If set, the output object will have the same type as the input object.
   */
  bool AppropriateOutput;
  ///@}

private:
  vtkHyperTreeGridAlgorithm(const vtkHyperTreeGridAlgorithm&) = delete;
  void operator=(const vtkHyperTreeGridAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
