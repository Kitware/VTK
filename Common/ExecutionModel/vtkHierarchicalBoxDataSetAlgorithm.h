/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxDataSetAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHierarchicalBoxDataSetAlgorithm
 * @brief   superclass for algorithms that
 * produce vtkHierarchicalBoxDataSet as output.
 *
 * Algorithms that take any type of data object (including composite dataset)
 * and produce a vtkHierarchicalBoxDataSet in the output can subclass from this
 * class.
*/

#ifndef vtkHierarchicalBoxDataSetAlgorithm_h
#define vtkHierarchicalBoxDataSetAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkHierarchicalBoxDataSet;
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkHierarchicalBoxDataSetAlgorithm : public vtkAlgorithm
{
public:
  static vtkHierarchicalBoxDataSetAlgorithm* New();
  vtkTypeMacro(vtkHierarchicalBoxDataSetAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkHierarchicalBoxDataSet* GetOutput();
  vtkHierarchicalBoxDataSet* GetOutput(int);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);
  //@}

  /**
   * see vtkAlgorithm for details
   */
  int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector) VTK_OVERRIDE;

protected:
  vtkHierarchicalBoxDataSetAlgorithm();
  ~vtkHierarchicalBoxDataSetAlgorithm() VTK_OVERRIDE;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector**,
                                vtkInformationVector*) {return 1;};

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*) {return 1;};

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*) {return 1;};

  //@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*)
  {
      return 1;
  };
  //@}

  // Create a default executive.
  vtkExecutive* CreateDefaultExecutive() VTK_OVERRIDE;

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  vtkDataObject *GetInput(int port);

private:
  vtkHierarchicalBoxDataSetAlgorithm(const vtkHierarchicalBoxDataSetAlgorithm&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHierarchicalBoxDataSetAlgorithm&) VTK_DELETE_FUNCTION;

};

#endif


