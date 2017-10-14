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
/**
 * @class   vtkLevelIdScalars
 *
 *
 *  Empty class for backwards compatibility.
*/

#ifndef vtkLevelIdScalars_h
#define vtkLevelIdScalars_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkOverlappingAMRLevelIdScalars.h"

class VTKFILTERSGENERAL_EXPORT vtkLevelIdScalars :
  public vtkOverlappingAMRLevelIdScalars
{
  public:
    static vtkLevelIdScalars* New();
    vtkTypeMacro(vtkLevelIdScalars,vtkOverlappingAMRLevelIdScalars);
    void PrintSelf(ostream& os, vtkIndent indent) override;

  protected:
    vtkLevelIdScalars();
    ~vtkLevelIdScalars() override;

  private:
    vtkLevelIdScalars(const vtkLevelIdScalars&) = delete;
    void operator=(const vtkLevelIdScalars&) = delete;
};

#endif /* VTKLEVELIDSCALARS_H_ */
