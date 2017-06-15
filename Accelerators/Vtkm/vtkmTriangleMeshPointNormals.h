/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmTriangleMeshPointNormals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkmTriangleMeshPointNormals_h
#define vtkmTriangleMeshPointNormals_h

#include "vtkTriangleMeshPointNormals.h"
#include "vtkAcceleratorsVTKmModule.h" // for export macro

class VTKACCELERATORSVTKM_EXPORT vtkmTriangleMeshPointNormals
  : public vtkTriangleMeshPointNormals
{
public:
  vtkTypeMacro(vtkmTriangleMeshPointNormals, vtkTriangleMeshPointNormals)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkmTriangleMeshPointNormals* New();

protected:
  vtkmTriangleMeshPointNormals();
  ~vtkmTriangleMeshPointNormals();

  int RequestData(vtkInformation*, vtkInformationVector**,
                  vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkmTriangleMeshPointNormals(const vtkmTriangleMeshPointNormals&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmTriangleMeshPointNormals&) VTK_DELETE_FUNCTION;
};

#endif // vtkmTriangleMeshPointNormals_h
// VTK-HeaderTest-Exclude: vtkmTriangleMeshPointNormals.h
