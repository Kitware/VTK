/*=========================================================================

  Program:   Visualization Library
  Module:    VectNorm.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlVectorNorm - generate scalars from euclidean norm of vectors
// .SECTION Description
// vlVectorNorm is a filter that generates scalar values by computing
// euclidean norm of vector triplets. Scalars can be normalized 
// 0<=s<=1 if desired.

#ifndef __vlVectorNorm_h
#define __vlVectorNorm_h

#include "DS2DSF.hh"

class vlVectorNorm : public vlDataSetToDataSetFilter 
{
public:
  vlVectorNorm();
  char *GetClassName() {return "vlVectorNorm";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify whether to normalize scalar values.
  vlSetMacro(Normalize,int);
  vlGetMacro(Normalize,int);
  vlBooleanMacro(Normalize,int);

protected:
  void Execute();
  int Normalize;  // normalize 0<=n<=1 if true.
};

#endif


