/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSphereSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPSphereSource
 * @brief   sphere source that supports pieces
*/

#ifndef vtkPSphereSource_h
#define vtkPSphereSource_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkSphereSource.h"

class VTKFILTERSPARALLEL_EXPORT vtkPSphereSource : public vtkSphereSource
{
public:
  vtkTypeMacro(vtkPSphereSource,vtkSphereSource);

  //@{
  /**
   * Construct sphere with radius=0.5 and default resolution 8 in both Phi
   * and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
   */
  static vtkPSphereSource *New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Get the estimated memory size in kibibytes (1024 bytes).
   */
  unsigned long GetEstimatedMemorySize();

protected:
  vtkPSphereSource() {}
  ~vtkPSphereSource() {}

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkPSphereSource(const vtkPSphereSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPSphereSource&) VTK_DELETE_FUNCTION;
};

#endif
