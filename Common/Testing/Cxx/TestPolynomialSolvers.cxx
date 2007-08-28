/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
#include "vtkPolynomialSolvers.h"

//=============================================================================
int TestPolynomialSolvers(int,char *[])
{
  int testIntValue;
  
  double P5[] = { 1., -10., 35., -50., 24., 0. } ;
  double roots[5];
  double tol = 1.e-12;
  testIntValue = vtkPolynomialSolvers::LinBairstowSolve( P5, 5, roots, tol );
  if ( testIntValue != 5 )
    {
    vtkGenericWarningMacro("LinBairstowSolve( x**5 -10*x**4 +35*x**3 -50*x**2 +24*x ) = "<<testIntValue<<" != 5");
    return 1;
    }

  double P22[] = {
    -0.0005, -0.001, 0.05, 0.1, -0.2,
    1., 0., -5.1, 0., 4., 
    -1., .2, 3., 2.2, 2.,
    -7., -.3, 3.8, 14., -16.,
    80., -97.9, 5. };
  double rootInt[] = { -3., 3. };

  testIntValue = vtkPolynomialSolvers::SturmRootCount( P22, 22, rootInt );
  if ( testIntValue != 5 )
    {
    vtkGenericWarningMacro("SturmRootCount( -0.0005 x^22 -0.001 x^21 +0.05 x^20 +0.1 x^19 -0.2 x^18 +1 x^17 -5.1 x^15 +4 x^13 -1 x^12 +0.2 x^11 +3 x^10 +2.2 x^9 +2 x^8 -7 x^7 -0.3 x^6 +3.8 x^5 +14 x^4 -16 x^3 +80 x^2 -97.9 x +5, ] -3 ; 3 ] ) = "<<testIntValue<<" != 5");
    return 1;
    }

  double lowerBnds[22];
  tol = 1.e-5;
  testIntValue = vtkPolynomialSolvers::SturmBisectionSolve( P22, 22, rootInt, lowerBnds, tol );
  if ( testIntValue != 5 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( -0.0005 x^22 -0.001 x^21 +0.05 x^20 +0.1 x^19 -0.2 x^18 +1 x^17 -5.1 x^15 +4 x^13 -1 x^12 +0.2 x^11 +3 x^10 +2.2 x^9 +2 x^8 -7 x^7 -0.3 x^6 +3.8 x^5 +14 x^4 -16 x^3 +80 x^2 -97.9 x +5, ] -3 ; 3 ] ) = "<<testIntValue<<" != 5");
    return 1;
    }

  return 0;
}
