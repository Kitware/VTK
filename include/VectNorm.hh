/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VectNorm.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkVectorNorm - generate scalars from euclidean norm of vectors
// .SECTION Description
// vtkVectorNorm is a filter that generates scalar values by computing
// euclidean norm of vector triplets. Scalars can be normalized 
// 0<=s<=1 if desired.

#ifndef __vtkVectorNorm_h
#define __vtkVectorNorm_h

#include "DS2DSF.hh"

class vtkVectorNorm : public vtkDataSetToDataSetFilter 
{
public:
  vtkVectorNorm();
  char *GetClassName() {return "vtkVectorNorm";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify whether to normalize scalar values.
  vtkSetMacro(Normalize,int);
  vtkGetMacro(Normalize,int);
  vtkBooleanMacro(Normalize,int);

protected:
  void Execute();
  int Normalize;  // normalize 0<=n<=1 if true.
};

#endif


