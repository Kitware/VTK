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
/**
 * @class   vtkNonOverlappingAMRAlgorithm
 *  produce vtkNonOverlappingAMR as output.
 *
 *
 *
*/

#ifndef vtkNonOverlappingAMRAlgorithm_h
#define vtkNonOverlappingAMRAlgorithm_h

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
    void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

    //@{
    /**
     * Get the output data object for a port on this algorithm
     */
    vtkNonOverlappingAMR* GetOutput();
    vtkNonOverlappingAMR* GetOutput(int);
    //@}

  protected:
    vtkNonOverlappingAMRAlgorithm();
    ~vtkNonOverlappingAMRAlgorithm() VTK_OVERRIDE;

    //@{
    /**
     * See algorithm for more info.
     */
    int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
    int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
    //@}

  private:
    vtkNonOverlappingAMRAlgorithm(const vtkNonOverlappingAMRAlgorithm&) VTK_DELETE_FUNCTION;
    void operator=(const vtkNonOverlappingAMRAlgorithm&) VTK_DELETE_FUNCTION;
};

#endif /* VTKNONOVERLAPPINGAMRALGORITHM_H_ */
