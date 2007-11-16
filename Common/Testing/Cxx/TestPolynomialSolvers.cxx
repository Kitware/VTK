/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
#include "vtkPolynomialSolvers.h"
#include "vtkTimerLog.h"

//=============================================================================
void PrintPolynomial( double* P, unsigned int degP )
{
  cout << "\n## P = ";

  unsigned int degPm1 = degP - 1;
  for ( unsigned int i = 0; i < degPm1; ++ i ) 
    {
    if ( P[i] > 0 ) cout << " +" << P[i] << " x^" << degP - i;
    else if ( P[i] < 0 ) cout << " " << P[i] << " x^" << degP - i;
    }   

  if ( degP > 0 )
    {
    if ( P[degPm1] > 0 ) cout << " +" << P[degPm1] << " x";
    else if ( P[degPm1] < 0 ) cout << " " << P[degPm1] << " x";
    }

  if ( P[degP] > 0 ) cout << " +" << P[degP];
  else if ( P[degP] < 0 ) cout << " " << P[degP];

  cout << "\n";
}

//=============================================================================
int TestPolynomialSolvers( int, char *[] )
{
  int testIntValue;

  // 1. find the roots of a degree 5 polynomial with LinBairstowSolve
  double P5[] = { 1., -10., 35., -50., 24., 0. } ;
  PrintPolynomial( P5, 5 );

  double roots[5];
  double tol = 1.e-12;
  vtkTimerLog *timer = vtkTimerLog::New();
  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::LinBairstowSolve( P5, 5, roots, tol );
  timer->StopTimer();

  if ( testIntValue != 5 )
    {
    vtkGenericWarningMacro("LinBairstowSolve( x**5 -10*x**4 +35*x**3 -50*x**2 +24*x ) = "<<testIntValue<<" != 5");
    return 1;
    }
  cout << "-> LinBairstowSolve found ( tolerance = " << tol
               << " ) " << testIntValue << " roots in " 
               << timer->GetElapsedTime() << " sec.:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << " " << roots[i] << "\n";

  // 2. find the roots of a quadratic trinomial with SturmBisectionSolve
  double P2[] = { 1., -2., 1. };
  PrintPolynomial( P2, 2 );

  double rootInt[] = { -3., 3. };
  tol = 1.e-5;
  double lowerBnds[22];
  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::SturmBisectionSolve( P2, 2, rootInt, lowerBnds, tol );
  timer->StopTimer();

  if ( testIntValue != 1 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( x^2 -  2 x + 1, ] -3 ; 3 ] ) found "<<testIntValue<<" root(s) instead of 1.");
    return 1;
    }
  if ( fabs( lowerBnds[0] - 1. ) > tol )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( x^2 -  2 x + 1, ] -3 ; 3 ] ) found root "<<lowerBnds[0]<<" instead of 1 (within tolerance of "<<tol<<").");
    return 1;
    }
  cout << "-> SturmBisectionSolve bracketed " << testIntValue << " roots in ] " 
               << rootInt[0] << " ; "
               << rootInt[1] << " ] within tolerance "
               << tol << " in "
               << timer->GetElapsedTime() << " sec:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << " ] " << lowerBnds[i]
                                                          << " ; " << lowerBnds[i] + tol
                                                          << " ]\n";
  

  // 3. count, then find the roots of a degree 22 polynomial with SturmRootCount and SturmBisectionSolve
  double P22[] = {
    -0.0005, -0.001, 0.05, 0.1, -0.2,
    1., 0., -5.1, 0., 4., 
    -1., .2, 3., 2.2, 2.,
    -7., -.3, 3.8, 14., -16.,
    80., -97.9, 5. };
  PrintPolynomial( P22, 22 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::SturmRootCount( P22, 22, rootInt );
  timer->StopTimer();
  if ( testIntValue != 5 )
    {
    vtkGenericWarningMacro("SturmRootCount( -0.0005 x^22 -0.001 x^21 +0.05 x^20 +0.1 x^19 -0.2 x^18 +1 x^17 -5.1 x^15 +4 x^13 -1 x^12 +0.2 x^11 +3 x^10 +2.2 x^9 +2 x^8 -7 x^7 -0.3 x^6 +3.8 x^5 +14 x^4 -16 x^3 +80 x^2 -97.9 x +5, ] -3 ; 3 ] ) = "<<testIntValue<<" != 5");
    return 1;
    }
  cout << "-> SturmRootCount counted " << testIntValue << " roots in ] " 
               << rootInt[0] << " ; "
               << rootInt[1] << " ] in "
               << timer->GetElapsedTime() << " sec.\n";

  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::SturmBisectionSolve( P22, 22, rootInt, lowerBnds, tol );
  timer->StopTimer();

  if ( testIntValue != 5 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( -0.0005 x^22 -0.001 x^21 +0.05 x^20 +0.1 x^19 -0.2 x^18 +1 x^17 -5.1 x^15 +4 x^13 -1 x^12 +0.2 x^11 +3 x^10 +2.2 x^9 +2 x^8 -7 x^7 -0.3 x^6 +3.8 x^5 +14 x^4 -16 x^3 +80 x^2 -97.9 x +5, ] -3 ; 3 ] ) found "<<testIntValue<<" root(s) instead of 5");
    return 1;
    }
  cout << "-> SturmBisectionSolve bracketed " << testIntValue << " roots in ] " 
               << rootInt[0] << " ; "
               << rootInt[1] << " ] within tolerance "
               << tol << " in "
               << timer->GetElapsedTime() << " sec:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << " ] " << lowerBnds[i]
                                                          << " ; " << lowerBnds[i] + tol
                                                          << " ]\n";

  timer->Delete();
  return 0;
}
