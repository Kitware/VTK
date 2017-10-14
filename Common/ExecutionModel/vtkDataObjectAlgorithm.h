/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataObjectAlgorithm
 * @brief   Superclass for algorithms that produce only data object as output
 *
 *
 * vtkDataObjectAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline architecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this classes
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be DataObject. If that
 * isn't the case then please override this method in your subclass.
 * You should implement the subclass's algorithm into
 * RequestData( request, inputVec, outputVec).
*/

#ifndef vtkDataObjectAlgorithm_h
#define vtkDataObjectAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkDataObject.h" // makes things a bit easier

class vtkDataSet;
class vtkDataObject;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkDataObjectAlgorithm : public vtkAlgorithm
{
public:
  static vtkDataObjectAlgorithm *New();
  vtkTypeMacro(vtkDataObjectAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkDataObject* GetOutput();
  vtkDataObject* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);
  //@}

  /**
   * see vtkAlgorithm for details
   */
  int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*) override;

  // this method is not recommended for use, but lots of old style filters
  // use it
  vtkDataObject* GetInput();
  vtkDataObject *GetInput(int port);

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
  vtkDataObjectAlgorithm();
  ~vtkDataObjectAlgorithm() override;

  // convenience method
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector**,
                                vtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*)
  {
    return 1;
  }

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*)
  {
    return 1;
  }

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkDataObjectAlgorithm(const vtkDataObjectAlgorithm&) = delete;
  void operator=(const vtkDataObjectAlgorithm&) = delete;
};

#endif
