/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetGradientPrecompute.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// .NAME vtkDataSetGradientPrecompute 
//
// .SECTION Description
// Computes a geometry based vector field that the DataSetGradient filter uses to accelerate
// gradient computation. This vector field is added to FieldData since it has a different
// value for each vertex of each cell (a vertex shared by two cell has two differents values).
//
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm contributed by
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// Implementation by Thierry Carrard (CEA)

#ifndef __vtkDataSetGradientPrecompute_h
#define __vtkDataSetGradientPrecompute_h

#include "vtkDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkDataSetGradientPrecompute : public vtkDataSetAlgorithm
{
 public:
  static vtkDataSetGradientPrecompute* New();
  vtkTypeMacro(vtkDataSetGradientPrecompute,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static int GradientPrecompute(vtkDataSet* ds);

 protected:
  vtkDataSetGradientPrecompute ();
  ~vtkDataSetGradientPrecompute ();
  
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

 private:
  vtkDataSetGradientPrecompute(const vtkDataSetGradientPrecompute&); // Not implemented
  void operator=(const vtkDataSetGradientPrecompute&); // Not implemented
};

#endif /* VTK_DATA_SET_GRADIENT_PRECOMPUTE_H */

