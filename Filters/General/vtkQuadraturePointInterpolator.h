/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraturePointInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadraturePointInterpolator
// .SECTION Description
//
// Interpolates each scalar/vector field in a vtkUnstrcturedGrid
// on its input to a specific set of quadrature points. The
// set of quadrature points is specified per array via a
// dictionary (ie an instance of vtkInformationQuadratureSchemeDefinitionVectorKey).
// contained in the array. The interpolated fields are placed
// in FieldData along with a set of per cell indexes, that allow
// random access to a given cells quadrature points.
//
// .SECTION See Also
// vtkQuadratureSchemeDefinition, vtkQuadraturePointsGenerator, vtkInformationQuadratureSchemeDefinitionVectorKey

#ifndef vtkQuadraturePointInterpolator_h
#define vtkQuadraturePointInterpolator_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkUnstructuredGrid;
class vtkInformation;
class vtkInformationVector;

class VTKFILTERSGENERAL_EXPORT vtkQuadraturePointInterpolator : public vtkDataSetAlgorithm
{
public:
  static vtkQuadraturePointInterpolator *New();
  vtkTypeMacro(vtkQuadraturePointInterpolator,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  int FillInputPortInformation(int port, vtkInformation *info);
  int FillOutputPortInformation(int port, vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkQuadraturePointInterpolator();
  ~vtkQuadraturePointInterpolator();

private:
  vtkQuadraturePointInterpolator(const vtkQuadraturePointInterpolator &); // Not implemented
  void operator=(const vtkQuadraturePointInterpolator &); // Not implemented
  //
  void Clear();
  // Description:
  // Generate field data arrays that have all scalar/vector
  // fields interpolated to the quadrature points. The type
  // of quadrature used is found in the dictionary stored as
  // meta data in each data array.
  int InterpolateFields(vtkUnstructuredGrid *usgOut);
};

#endif
