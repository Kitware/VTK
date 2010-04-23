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
// .NAME vtkLevelIdScalars - generate scalars from levels.
// .SECTION Description
// vtkLevelIdScalars is a filter that generates scalars using the level number
// for each level. Note that all datasets within a level get the same scalar.
// The new scalars array is named \c LevelIdScalars.

#ifndef __vtkLevelIdScalars_h
#define __vtkLevelIdScalars_h

#include "vtkHierarchicalBoxDataSetAlgorithm.h"

class vtkUniformGrid;
class VTK_GRAPHICS_EXPORT vtkLevelIdScalars : public vtkHierarchicalBoxDataSetAlgorithm
{
public:
  static vtkLevelIdScalars* New();
  vtkTypeMacro(vtkLevelIdScalars, vtkHierarchicalBoxDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkLevelIdScalars();
  ~vtkLevelIdScalars();

  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);

  vtkUniformGrid* ColorLevel(vtkUniformGrid* input, int group);

private:
  vtkLevelIdScalars(const vtkLevelIdScalars&); // Not implemented.
  void operator=(const vtkLevelIdScalars&); // Not implemented.
//ETX
};

#endif


