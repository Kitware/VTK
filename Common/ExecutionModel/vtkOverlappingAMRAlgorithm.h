/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkOverlappingAMRAlgorithm.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkOverlappingAMRAlgorithm.h -- Superclass for overlapping AMR classes
//
// .SECTION Description
//  A base class for all algorithms that take as input vtkOverlappingAMR and
//  produce vtkOverlappingAMR.

#ifndef VTKOVERLAPPINGAMRALGORITHM_H_
#define VTKOVERLAPPINGAMRALGORITHM_H_

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkUniformGridAMRAlgorithm.h"

class vtkOverlappingAMR;
class vtkInformation;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkOverlappingAMRAlgorithm :
  public vtkUniformGridAMRAlgorithm
{
  public:
    static vtkOverlappingAMRAlgorithm* New();
    vtkTypeMacro(vtkOverlappingAMRAlgorithm,vtkUniformGridAMRAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
    // Get the output data object for a port on this algorithm
    vtkOverlappingAMR* GetOutput();
    vtkOverlappingAMR* GetOutput(int);

  protected:
    vtkOverlappingAMRAlgorithm();
    virtual ~vtkOverlappingAMRAlgorithm();

    // Description:
    // See algorithm for more info.
    virtual int FillOutputPortInformation(int port, vtkInformation* info);
    virtual int FillInputPortInformation(int port, vtkInformation* info);

  private:
    vtkOverlappingAMRAlgorithm(const vtkOverlappingAMRAlgorithm&); // Not implemented
    void operator=(const vtkOverlappingAMRAlgorithm&); // Not implemented
};

#endif /* VTKOVERLAPPINGAMRALGORITHM_H_ */
