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
/**
 * @class   vtkOverlappingAMRAlgorithm
 *
 *
 *  A base class for all algorithms that take as input vtkOverlappingAMR and
 *  produce vtkOverlappingAMR.
*/

#ifndef vtkOverlappingAMRAlgorithm_h
#define vtkOverlappingAMRAlgorithm_h

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
    void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

    //@{
    /**
     * Get the output data object for a port on this algorithm
     */
    vtkOverlappingAMR* GetOutput();
    vtkOverlappingAMR* GetOutput(int);
    //@}

  protected:
    vtkOverlappingAMRAlgorithm();
    ~vtkOverlappingAMRAlgorithm() VTK_OVERRIDE;

    //@{
    /**
     * See algorithm for more info.
     */
    int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
    int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
    //@}

  private:
    vtkOverlappingAMRAlgorithm(const vtkOverlappingAMRAlgorithm&) VTK_DELETE_FUNCTION;
    void operator=(const vtkOverlappingAMRAlgorithm&) VTK_DELETE_FUNCTION;
};

#endif /* VTKOVERLAPPINGAMRALGORITHM_H_ */
