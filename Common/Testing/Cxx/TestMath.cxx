/*
 * Copyright 2005 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
#include "vtkMath.h"

int TestMath(int,char *[])
{
  int testIntValue;
  
  testIntValue = vtkMath::Factorial(5);
  if ( testIntValue != 120 )
    {
    vtkGenericWarningMacro("Factorial(5) = "<<testIntValue<<" != 120");
    return 1;
    }

  testIntValue = vtkMath::Binomial(8,3);
  if ( testIntValue != 56 )
    {
    vtkGenericWarningMacro("Binomial(8,3) = "<<testIntValue<<" != 56");
    return 1;
    }

  testIntValue = vtkMath::Binomial(5,3);
  if ( testIntValue != 10 )
    {
    vtkGenericWarningMacro("Binomial(5,3) = "<<testIntValue<<" != 10");
    return 1;
    }

  // Solving x(x - 10^-4)^2 = 0 illustrates how the Tartaglia-Cardan solver
  // filters some numerical noise by noticing there is a double root (that
  // SolveCubic does not notice).
  double c[] = { 1., -2.e-4, 1.e-8, 0.};
#if 0
  double r1, r2, r3;
  int nr;
  testIntValue = vtkMath::SolveCubic( c[0], c[1], c[2], c[3], &r1, &r2, &r3, &nr );
  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("SolveCubic returned "<<testIntValue<<" != 3");
    return 1;
    }
#endif // 0  
  double r[3];
  int m[3];
  testIntValue = vtkMath::TartagliaCardanSolve( c, r, m );
  if ( testIntValue != 2 )
    {
    vtkGenericWarningMacro("TartagliaCardanSolve returned "<<testIntValue<<" != 2");
    return 1;
    }

  return 0;
}
