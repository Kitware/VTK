/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridAlgorithm
 * @brief   Superclass for algorithms that produce
 * a hyper tree grid as output
 *
 *
 * vtkHyperTreeGridAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline architecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this classes
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be HyperTreeGrid. If that
 * isn't the case then please override this method in your subclass.
 *
 * @par Thanks:
 * This test was written by Philippe Pebay and Charles Law, Kitware 2012
 * This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridAlgorithm_h
#define vtkHyperTreeGridAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkHyperTreeGrid.h" // makes things a bit easier

class vtkDataSet;
class vtkHyperTreeGrid;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkHyperTreeGridAlgorithm : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkHyperTreeGridAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkHyperTreeGrid *GetOutput();
  vtkHyperTreeGrid *GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);
  //@}

  /**
   * see vtkAlgorithm for details
   */
  int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*) VTK_OVERRIDE;

  // this method is not recommended for use, but lots of old style filters
  // use it
  vtkDataObject* GetInput();
  vtkDataObject *GetInput(int port);
  vtkHyperTreeGrid *GetHyperTreeGridInput(int port);

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(vtkDataObject *);
  void SetInputData(int, vtkDataObject*);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(vtkDataObject *);
  void AddInputData(int, vtkDataObject*);
  //@}

protected:
  vtkHyperTreeGridAlgorithm();
  ~vtkHyperTreeGridAlgorithm() VTK_OVERRIDE;

  // convenience method
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

private:
  vtkHyperTreeGridAlgorithm(const vtkHyperTreeGridAlgorithm&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHyperTreeGridAlgorithm&) VTK_DELETE_FUNCTION;
};

#endif
