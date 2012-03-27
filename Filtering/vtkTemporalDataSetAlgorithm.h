/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalDataSetAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalDataSetAlgorithm - Superclass for algorithms that produce only vtkTemporalDataSet as output
// .SECTION Description
// Algorithms that take any type of data object (including composite dataset)
// and produce a vtkTemporalDataSet in the output can subclass from this
// class.


#ifndef __vtkTemporalDataSetAlgorithm_h
#define __vtkTemporalDataSetAlgorithm_h

#include "vtkAlgorithm.h"

class vtkTemporalDataSet;

class VTK_FILTERING_EXPORT vtkTemporalDataSetAlgorithm : public vtkAlgorithm
{
public:
  static vtkTemporalDataSetAlgorithm *New();
  vtkTypeMacro(vtkTemporalDataSetAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkTemporalDataSet* GetOutput();
  vtkTemporalDataSet* GetOutput(int);

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
  vtkTemporalDataSetAlgorithm();
  ~vtkTemporalDataSetAlgorithm() {};

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
  vtkTemporalDataSetAlgorithm(const vtkTemporalDataSetAlgorithm&);  // Not implemented.
  void operator=(const vtkTemporalDataSetAlgorithm&);  // Not implemented.
};

#endif


