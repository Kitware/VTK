/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnstructuredGridToReebGraphFilter - generate a Reeb graph from a
// scalar field defined on a vtkUnstructuredGrid.
// .SECTION Description
// The filter will first try to pull as a scalar field the vtkDataArray with
// Id 'fieldId' of the mesh's vtkPointData.
// If this field does not exist, the filter will use the vtkElevationFilter to
// generate a default scalar field.

#ifndef __vtkUnstructuredGridToReebGraphFilter_h
#define __vtkUnstructuredGridToReebGraphFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkReebGraph.h"

class VTK_GRAPHICS_EXPORT vtkUnstructuredGridToReebGraphFilter :
  public vtkDataObjectAlgorithm
{
public:
  static vtkUnstructuredGridToReebGraphFilter* New();
  vtkTypeRevisionMacro(vtkUnstructuredGridToReebGraphFilter,
    vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the scalar field id (default = 0).
  vtkSetMacro(FieldId, int);
  vtkGetMacro(FieldId, int);

  vtkReebGraph* GetOutput();


protected:
  vtkUnstructuredGridToReebGraphFilter();
  ~vtkUnstructuredGridToReebGraphFilter();

  int FieldId;

  int FillInputPortInformation(int portNumber, vtkInformation *);
  int FillOutputPortInformation(int, vtkInformation *);

  int RequestData(vtkInformation*,
                  vtkInformationVector**,
                  vtkInformationVector*);

private:
  vtkUnstructuredGridToReebGraphFilter(
    const vtkUnstructuredGridToReebGraphFilter&);
  // Not implemented.
  void operator=(const vtkUnstructuredGridToReebGraphFilter&);
  // Not implemented.
};

#endif
