/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraturePointStatistics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadraturePointStatistics
// .SECTION Description
//
// For all interpolated fields (see vtkQuadraturePointInterpolator) in
// this data set, generate descriptive statisics. The results are 
// placed in a vtkTable withone column for each interpolarted array
// found in the input.
//
// .SECTION See Also
// vtkQuadratureSchemeDefinition, vtkQuadraturePointInterpolator, vtkTable

#ifndef vtkQuadraturePointStatistics_h
#define vtkQuadraturePointStatistics_h

#include "vtkDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;
class vtkTable;

class VTK_GRAPHICS_EXPORT vtkQuadraturePointStatistics : public vtkDataSetAlgorithm
{
public:
  static vtkQuadraturePointStatistics *New();
  vtkTypeRevisionMacro(vtkQuadraturePointStatistics,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  int FillInputPortInformation(int port, vtkInformation *info);
  int FillOutputPortInformation(int port, vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkQuadraturePointStatistics();
  ~vtkQuadraturePointStatistics();

private:
  vtkQuadraturePointStatistics(const vtkQuadraturePointStatistics &); // Not implemented
  void operator=(const vtkQuadraturePointStatistics &); // Not implemented
  //
  void Clear();
  // Description:
  // Compute statistics, placing the results in a column for 
  // each interpolated array found in the input.
  int ComputeStatistics(vtkUnstructuredGrid *usgIn, vtkTable *results);
};

#endif
