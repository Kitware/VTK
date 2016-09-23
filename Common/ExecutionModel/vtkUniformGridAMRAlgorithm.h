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
/**
 * @class   vtkUniformGridAMRAlgorithm
 *  vtkUniformGridAMR as output.
 *
 *
 *  A base class for all algorithms that take as input any type of data object
 *  including composite datasets and produce vtkUniformGridAMR in the output.
*/

#ifndef vtkUniformGridAMRAlgorithm_h
#define vtkUniformGridAMRAlgorithm_h

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
    void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

    //@{
    /**
     * Get the output data object for a port on this algorithm
     */
    vtkUniformGridAMR* GetOutput();
    vtkUniformGridAMR* GetOutput(int);
    //@}

    //@{
    /**
     * Set an input of this algorithm.
     */
    void SetInputData(vtkDataObject*);
    void SetInputData(int,vtkDataObject*);
    //@}

    /**
     * See vtkAlgorithm for details
     */
    int ProcessRequest(vtkInformation* request,
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector ) VTK_OVERRIDE;

  protected:
    vtkUniformGridAMRAlgorithm();
    ~vtkUniformGridAMRAlgorithm() VTK_OVERRIDE;

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

    /**
     * This is called by the superclass.
     * This is the method you should override.
     */
    virtual int RequestUpdateExtent(vtkInformation*,
                                    vtkInformationVector**,
                                    vtkInformationVector*) { return 1; };

    /**
     * Create a default executive
     */
    vtkExecutive* CreateDefaultExecutive() VTK_OVERRIDE;

    //@{
    /**
     * See algorithm for more info.
     */
    int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
    int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
    //@}

    vtkDataObject *GetInput(int port);

  private:
    vtkUniformGridAMRAlgorithm(const vtkUniformGridAMRAlgorithm&) VTK_DELETE_FUNCTION;
    void operator=(const vtkUniformGridAMRAlgorithm&) VTK_DELETE_FUNCTION;
};

#endif /* VTKUNIFORMGRIDAMRALGORITHM_H_ */
