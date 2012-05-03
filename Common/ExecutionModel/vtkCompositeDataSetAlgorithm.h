/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSetAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataSetAlgorithm - Superclass for algorithms that produce only vtkCompositeDataSet as output
// .SECTION Description
// Algorithms that take any type of data object (including composite dataset)
// and produce a vtkCompositeDataSet in the output can subclass from this
// class.


#ifndef __vtkCompositeDataSetAlgorithm_h
#define __vtkCompositeDataSetAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkCompositeDataSet;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkCompositeDataSetAlgorithm : public vtkAlgorithm
{
public:
  static vtkCompositeDataSetAlgorithm *New();
  vtkTypeMacro(vtkCompositeDataSetAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkCompositeDataSet* GetOutput();
  vtkCompositeDataSet* GetOutput(int);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:
  vtkCompositeDataSetAlgorithm();
  ~vtkCompositeDataSetAlgorithm() {};

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector**,
                                vtkInformationVector*) {return 1;};

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*) {return 1;};

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*) {return 1;};

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*)
    {
      return 1;
    };

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkDataObject *GetInput(int port);

private:
  vtkCompositeDataSetAlgorithm(const vtkCompositeDataSetAlgorithm&);  // Not implemented.
  void operator=(const vtkCompositeDataSetAlgorithm&);  // Not implemented.
};

#endif


