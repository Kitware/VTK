/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPReflectionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPReflectionFilter
 * @brief   parallel version of vtkReflectionFilter
 *
 * vtkPReflectionFilter is a parallel version of vtkReflectionFilter which takes
 * into consideration the full dataset bounds for performing the reflection.
*/

#ifndef vtkPReflectionFilter_h
#define vtkPReflectionFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkReflectionFilter.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPReflectionFilter : public vtkReflectionFilter
{
public:
  static vtkPReflectionFilter* New();
  vtkTypeMacro(vtkPReflectionFilter, vtkReflectionFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get/Set the parallel controller.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro (Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPReflectionFilter();
  ~vtkPReflectionFilter();

  /**
   * Internal method to compute bounds.
   */
  virtual int ComputeBounds(vtkDataObject* input, double bounds[6]);

  vtkMultiProcessController* Controller;
private:
  vtkPReflectionFilter(const vtkPReflectionFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPReflectionFilter&) VTK_DELETE_FUNCTION;

};

#endif


