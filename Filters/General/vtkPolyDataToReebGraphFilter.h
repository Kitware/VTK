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
// .NAME vtkPolyDataToReebGraphFilter - generate a Reeb graph from a scalar
// field defined on a vtkPolyData.
// .SECTION Description
// The filter will first try to pull as a scalar field the vtkDataArray with
// Id 'fieldId' of the mesh's vtkPointData.
// If this field does not exist, the filter will use the vtkElevationFilter to
// generate a default scalar field.

#ifndef vtkPolyDataToReebGraphFilter_h
#define vtkPolyDataToReebGraphFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkDirectedGraphAlgorithm.h"

class vtkReebGraph;

class VTKFILTERSGENERAL_EXPORT vtkPolyDataToReebGraphFilter :
  public vtkDirectedGraphAlgorithm
{
public:
  static vtkPolyDataToReebGraphFilter* New();
  vtkTypeMacro(vtkPolyDataToReebGraphFilter, vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the scalar field id (default = 0).
  vtkSetMacro(FieldId, int);
  vtkGetMacro(FieldId, int);

  vtkReebGraph* GetOutput();

protected:
  vtkPolyDataToReebGraphFilter();
  ~vtkPolyDataToReebGraphFilter();

  int FieldId;

  int FillInputPortInformation(int portNumber, vtkInformation *);
  int FillOutputPortInformation(int, vtkInformation *);

  int RequestData(vtkInformation*,
                  vtkInformationVector**,
                  vtkInformationVector*);

private:
  vtkPolyDataToReebGraphFilter(const vtkPolyDataToReebGraphFilter&);
  // Not implemented.
  void operator=(const vtkPolyDataToReebGraphFilter&);  // Not implemented.
};

#endif
