/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransposeMatrix.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkTransposeMatrix
 * @brief   Computes the transpose of an input matrix.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkTransposeMatrix_h
#define vtkTransposeMatrix_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkArrayDataAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkTransposeMatrix : public vtkArrayDataAlgorithm
{
public:
  static vtkTransposeMatrix* New();
  vtkTypeMacro(vtkTransposeMatrix, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTransposeMatrix();
  ~vtkTransposeMatrix();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkTransposeMatrix(const vtkTransposeMatrix&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTransposeMatrix&) VTK_DELETE_FUNCTION;
};

#endif

