/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVOI.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkmExtractVOI_h
#define vtkmExtractVOI_h

#include "vtkExtractVOI.h"
#include "vtkAcceleratorsVTKmModule.h" // for export macro


class VTKACCELERATORSVTKM_EXPORT vtkmExtractVOI : public vtkExtractVOI
{
public:
  vtkTypeMacro(vtkmExtractVOI, vtkExtractVOI)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkmExtractVOI* New();

protected:
  vtkmExtractVOI();
  ~vtkmExtractVOI();

  int RequestData(vtkInformation*, vtkInformationVector**,
                  vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkmExtractVOI(const vtkmExtractVOI&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmExtractVOI&) VTK_DELETE_FUNCTION;
};

#endif // vtkmExtractVOI_h
// VTK-HeaderTest-Exclude: vtkmExtractVOI.h
