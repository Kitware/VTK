/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverlappingAMRLevelIdScalars.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOverlappingAMRLevelIdScalars - generate scalars from levels.
// .SECTION Description
// vtkOverlappingAMRLevelIdScalars is a filter that generates scalars using
// the level number for each level. Note that all datasets within a level get
// the same scalar. The new scalars array is named \c LevelIdScalars.

#ifndef vtkOverlappingAMRLevelIdScalars_h
#define vtkOverlappingAMRLevelIdScalars_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkOverlappingAMRAlgorithm.h"

class vtkUniformGrid;
class vtkUniformGridAMR;

class VTKFILTERSGENERAL_EXPORT vtkOverlappingAMRLevelIdScalars :
  public vtkOverlappingAMRAlgorithm
{
public:
  static vtkOverlappingAMRLevelIdScalars* New();
  vtkTypeMacro(vtkOverlappingAMRLevelIdScalars,vtkOverlappingAMRAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkOverlappingAMRLevelIdScalars();
  ~vtkOverlappingAMRLevelIdScalars();

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *);

  void AddColorLevels(vtkUniformGridAMR *input, vtkUniformGridAMR *output);
  vtkUniformGrid* ColorLevel(vtkUniformGrid* input, int group);

private:
  vtkOverlappingAMRLevelIdScalars(const vtkOverlappingAMRLevelIdScalars&); // Not implemented.
  void operator=(const vtkOverlappingAMRLevelIdScalars&); // Not implemented.
//ETX
};

#endif
