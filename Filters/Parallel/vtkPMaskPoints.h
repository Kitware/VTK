/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPMaskPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPMaskPoints
 * @brief   parallel Mask Points
 *
 * The difference between this implementation and vtkMaskPoints is
 * the use of the vtkMultiProcessController and that
 * ProportionalMaximumNumberOfPoints is obeyed.
*/

#ifndef vtkPMaskPoints_h
#define vtkPMaskPoints_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkMaskPoints.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPMaskPoints : public vtkMaskPoints
{
public:
  static vtkPMaskPoints *New();
  vtkTypeMacro(vtkPMaskPoints,vtkMaskPoints);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the communicator object for interprocess communication
   */
  virtual vtkMultiProcessController* GetController();
  virtual void SetController(vtkMultiProcessController*);
  //@}

protected:
  vtkPMaskPoints();
  ~vtkPMaskPoints() VTK_OVERRIDE;

  void InternalScatter(unsigned long*, unsigned long *, int, int) VTK_OVERRIDE;
  void InternalGather(unsigned long*, unsigned long*, int, int) VTK_OVERRIDE;
  int InternalGetNumberOfProcesses() VTK_OVERRIDE;
  int InternalGetLocalProcessId() VTK_OVERRIDE;
  void InternalBarrier() VTK_OVERRIDE;

  vtkMultiProcessController* Controller;
private:
  vtkPMaskPoints(const vtkPMaskPoints&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPMaskPoints&) VTK_DELETE_FUNCTION;
};

#endif
