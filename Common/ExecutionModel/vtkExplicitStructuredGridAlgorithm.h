/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitStructuredGridAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExplicitStructuredGridAlgorithm
 * @brief   Superclass for algorithms that produce only
 * explicit structured grid as output.
 */

#ifndef vtkExplicitStructuredGridAlgorithm_h
#define vtkExplicitStructuredGridAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro

class vtkDataSet;
class vtkExplicitStructuredGrid;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkExplicitStructuredGridAlgorithm : public vtkAlgorithm
{
public:
  static vtkExplicitStructuredGridAlgorithm* New();
  vtkTypeMacro(vtkExplicitStructuredGridAlgorithm, vtkAlgorithm);

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkExplicitStructuredGrid* GetOutput();
  vtkExplicitStructuredGrid* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);
  //@}

  /**
   * see vtkAlgorithm for details
   */
  virtual vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // this method is not recommended for use, but lots of old style filters
  // use it
  vtkDataObject* GetInput();
  vtkDataObject* GetInput(int port);
  vtkExplicitStructuredGrid* GetExplicitStructuredGridInput(int port);

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(vtkDataObject*);
  void AddInputData(int, vtkDataObject*);
  //@}

protected:
  vtkExplicitStructuredGridAlgorithm();
  ~vtkExplicitStructuredGridAlgorithm() override = default;

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

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info) override;
  virtual int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkExplicitStructuredGridAlgorithm(const vtkExplicitStructuredGridAlgorithm&) = delete;
  void operator=(const vtkExplicitStructuredGridAlgorithm&) = delete;
};

#endif
