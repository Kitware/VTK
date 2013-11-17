/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorNorm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVectorNorm - generate scalars from Euclidean norm of vectors
// .SECTION Description
// vtkVectorNorm is a filter that generates scalar values by computing
// Euclidean norm of vector triplets. Scalars can be normalized
// 0<=s<=1 if desired.
//
// Note that this filter operates on point or cell attribute data, or
// both.  By default, the filter operates on both point and cell data
// if vector point and cell data, respectively, are available from the
// input. Alternatively, you can choose to generate scalar norm values
// for just cell or point data.

#ifndef __vtkVectorNorm_h
#define __vtkVectorNorm_h

#define VTK_ATTRIBUTE_MODE_DEFAULT 0
#define VTK_ATTRIBUTE_MODE_USE_POINT_DATA 1
#define VTK_ATTRIBUTE_MODE_USE_CELL_DATA 2

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkVectorNorm : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkVectorNorm,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with normalize flag off.
  static vtkVectorNorm *New();

  // Description:
  // Specify whether to normalize scalar values.
  vtkSetMacro(Normalize,int);
  vtkGetMacro(Normalize,int);
  vtkBooleanMacro(Normalize,int);

  // Description:
  // Control how the filter works to generate scalar data from the
  // input vector data. By default, (AttributeModeToDefault) the
  // filter will generate the scalar norm for point and cell data (if
  // vector data present in the input). Alternatively, you can
  // explicitly set the filter to generate point data
  // (AttributeModeToUsePointData) or cell data
  // (AttributeModeToUseCellData).
  vtkSetMacro(AttributeMode,int);
  vtkGetMacro(AttributeMode,int);
  void SetAttributeModeToDefault()
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_DEFAULT);};
  void SetAttributeModeToUsePointData()
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_POINT_DATA);};
  void SetAttributeModeToUseCellData()
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_CELL_DATA);};
  const char *GetAttributeModeAsString();

protected:
  vtkVectorNorm();
  ~vtkVectorNorm() {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int Normalize;  // normalize 0<=n<=1 if true.
  int AttributeMode; //control whether to use point or cell data, or both
private:
  vtkVectorNorm(const vtkVectorNorm&);  // Not implemented.
  void operator=(const vtkVectorNorm&);  // Not implemented.
};

#endif
