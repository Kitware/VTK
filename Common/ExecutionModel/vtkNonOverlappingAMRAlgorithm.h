/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkNonOverlappingAMRAlgorithm.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkNonOverlappingAMRAlgorithm.h -- Superclass for algorithms that
//  produce vtkNonOverlappingAMR as output.
//
// .SECTION Description
//
#ifndef VTKNONOVERLAPPINGAMRALGORITHM_H_
#define VTKNONOVERLAPPINGAMRALGORITHM_H_

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkUniformGridAMRAlgorithm.h"

class vtkNonOverlappingAMR;
class vtkInformation;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkNonOverlappingAMRAlgorithm :
  public vtkUniformGridAMRAlgorithm
{
  public:
    static vtkNonOverlappingAMRAlgorithm* New();
    vtkTypeMacro(vtkNonOverlappingAMRAlgorithm,vtkUniformGridAMRAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
    // Get the output data object for a port on this algorithm
    vtkNonOverlappingAMR* GetOutput();
    vtkNonOverlappingAMR* GetOutput(int);

  protected:
    vtkNonOverlappingAMRAlgorithm();
    virtual ~vtkNonOverlappingAMRAlgorithm();

    // Description:
    // See algorithm for more info.
    virtual int FillOutputPortInformation(int port, vtkInformation* info);
    virtual int FillInputPortInformation(int port, vtkInformation* info);

  private:
    vtkNonOverlappingAMRAlgorithm(const vtkNonOverlappingAMRAlgorithm&); // Not implemented
    void operator=(const vtkNonOverlappingAMRAlgorithm&); // Not implemented
};

#endif /* VTKNONOVERLAPPINGAMRALGORITHM_H_ */
