/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkLevelIdScalars.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkLevelIdScalars.h -- Empty class for backwards compatibility
//
// .SECTION Description
//  Empty class for backwards compatibility.
#ifndef VTKLEVELIDSCALARS_H_
#define VTKLEVELIDSCALARS_H_

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkOverlappingAMRLevelIdScalars.h"

class VTKFILTERSGENERAL_EXPORT vtkLevelIdScalars :
  public vtkOverlappingAMRLevelIdScalars
{
  public:
    static vtkLevelIdScalars* New();
    vtkTypeMacro(vtkLevelIdScalars,vtkOverlappingAMRLevelIdScalars);
    void PrintSelf(ostream& os, vtkIndent indent);

  protected:
    vtkLevelIdScalars();
    virtual ~vtkLevelIdScalars();

  private:
    vtkLevelIdScalars(const vtkLevelIdScalars&); // Not implemented
    void operator=(const vtkLevelIdScalars&); // Not implemented
};

#endif /* VTKLEVELIDSCALARS_H_ */
