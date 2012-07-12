/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkUniformGridAMRAlgorithm.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkUniformGridAMRAlgorithm.h -- Superclass for algorithms that produce
//  vtkUniformGridAMR as output.
//
// .SECTION Description
//  A base class for all algorithms that take as input any type of data object
//  including composite datasets and produce vtkUniformGridAMR in the output.

#ifndef VTKUNIFORMGRIDAMRALGORITHM_H_
#define VTKUNIFORMGRIDAMRALGORITHM_H_

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkUniformGridAMR;
class vtkInformation;
class vtkInformationVector;
class vtkExecutive;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkUniformGridAMRAlgorithm : public vtkAlgorithm
{
  public:
    static vtkUniformGridAMRAlgorithm* New();
    vtkTypeMacro(vtkUniformGridAMRAlgorithm, vtkAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
    // Get the output data object for a port on this algorithm
    vtkUniformGridAMR* GetOutput();
    vtkUniformGridAMR* GetOutput(int);

    // Description:
    // Set an input of this algorithm.
    void SetInputData(vtkDataObject*);
    void SetInputData(int,vtkDataObject*);

    // Description:
    // See vtkAlgorithm for details
    virtual int ProcessRequest(vtkInformation* request,
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector );

  protected:
    vtkUniformGridAMRAlgorithm();
    virtual ~vtkUniformGridAMRAlgorithm();

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
                                    vtkInformationVector*) { return 1; };

    // Description:
    // Create a default executive
    virtual vtkExecutive* CreateDefaultExecutive();

    // Description:
    // See algorithm for more info.
    virtual int FillOutputPortInformation(int port, vtkInformation* info);
    virtual int FillInputPortInformation(int port, vtkInformation* info);

    vtkDataObject *GetInput(int port);

  private:
    vtkUniformGridAMRAlgorithm(const vtkUniformGridAMRAlgorithm&); // Not implemented
    void operator=(const vtkUniformGridAMRAlgorithm&); // Not implemented
};

#endif /* VTKUNIFORMGRIDAMRALGORITHM_H_ */
