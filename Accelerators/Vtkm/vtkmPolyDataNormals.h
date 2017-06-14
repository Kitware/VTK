/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmPolyDataNormals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkmPolyDataNormals_h
#define vtkmPolyDataNormals_h

#include "vtkPolyDataNormals.h"
#include "vtkAcceleratorsVTKmModule.h" // for export macro

class VTKACCELERATORSVTKM_EXPORT vtkmPolyDataNormals : public vtkPolyDataNormals
{
public:
  vtkTypeMacro(vtkmPolyDataNormals, vtkPolyDataNormals)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkmPolyDataNormals* New();

protected:
  vtkmPolyDataNormals();
  ~vtkmPolyDataNormals();

  int RequestData(vtkInformation*, vtkInformationVector**,
                  vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkmPolyDataNormals(const vtkmPolyDataNormals&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmPolyDataNormals&) VTK_DELETE_FUNCTION;
};

#endif // vtkmPolyDataNormals_h
// VTK-HeaderTest-Exclude: vtkmPolyDataNormals.h
