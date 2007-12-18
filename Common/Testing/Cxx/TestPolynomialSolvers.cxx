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
    if ( P[i] > 0 ) cout << " +" << P[i] << "*x**" << degP - i;
    else if ( P[i] < 0 ) cout << " " << P[i] << "*x**" << degP - i;
    }   

  if ( degP > 0 )
    {
    if ( P[degPm1] > 0 ) cout << " +" << P[degPm1] << "*x";
    else if ( P[degPm1] < 0 ) cout << " " << P[degPm1] << "*x";
    }

  if ( P[degP] > 0 ) cout << " +" << P[degP];
  else if ( P[degP] < 0 ) cout << " " << P[degP];

  cout << "\n";
}

//=============================================================================
int TestPolynomialSolvers( int, char *[] )
{
  int testIntValue;
  double tolLinBairstow = 1.e-12;
  double tolSturm = 1.e-5;
  double roots[5];
  int mult[4];
  double rootInt[] = { -4., 4. };
  double lowerBnds[22];
  vtkTimerLog *timer = vtkTimerLog::New();

  // 1. find the roots of a degree 4 polynomial with a 1 double root (1) and 2
  // simple roots (2 and 3) using:
  // 1.a FerrariSolve
  // 1.b SturmBissectionSolve
  double P4[] = { 1., -7., 17., -17., 6. } ;
  PrintPolynomial( P4, 4 );

  // 1.a FerrariSolve
  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::FerrariSolve( P4 + 1, roots, mult, 0. );
  timer->StopTimer();

  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("LinBairstowSolve( x^4 -7 x^3 +17 x^2 -17 x +6 ) = "<<testIntValue<<" != 3");
    return 1;
    }
  cout << "-> FerrariSolve found ( numerical noise filtering = " << 0
               << " ) " << testIntValue << " roots in " 
               << timer->GetElapsedTime() << " sec.:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << " " << roots[i] 
                                                  << ", with multiplicity " << mult[i] 
                                                  << "\n";
  double actualRoots[] = { 1., 2., 3. };
  int actualMult[] = { 2, 1, 1 };
  for ( int i = 0; i < testIntValue ; ++ i ) 
    {
    if ( roots[i] != actualRoots[i] )
      {
      vtkGenericWarningMacro("FerrariSolve( x^4 -7 x^3 +17 x^2 -17 x +6, ] -4 ; 4 ] ) found root "<<roots[i]<<" != "<<actualRoots[i]);
      return 1;  
      }
    if ( mult[i] != actualMult[i] )
      {
      vtkGenericWarningMacro("FerrariSolve( x^4 -7 x^3 +17 x^2 -17 x +6, ] -4 ; 4 ] ) found multiplicity "<<mult[i]<<" != "<<actualMult[i]);
      return 1;  
      }
    }

  // 1.b SturmBissectionSolve
  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::SturmBisectionSolve( P4, 4, rootInt, lowerBnds, tolSturm );
  timer->StopTimer();

  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( x^4 -7 x^3 +17 x^2 -17 x +6, ] -4 ; 4 ] ) found "<<testIntValue<<" root(s) instead of 3.");
    return 1;
    }
  cout << "-> SturmBisectionSolve bracketed " << testIntValue << " roots in ] " 
               << rootInt[0] << " ; "
               << rootInt[1] << " ] within tolerance "
               << tolSturm << " in "
               << timer->GetElapsedTime() << " sec:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << " ] " << lowerBnds[i] -tolSturm
                                                          << " ; " << lowerBnds[i]
                                                          << " ]\n";

  // 2. find the roots of a degree 5 polynomial with LinBairstowSolve
  double P5[] = { 1., -10., 35., -50., 24., 0. } ;
  PrintPolynomial( P5, 5 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::LinBairstowSolve( P5, 5, roots, tolLinBairstow );
  timer->StopTimer();

  if ( testIntValue != 5 )
    {
    vtkGenericWarningMacro("LinBairstowSolve( x^5 -10 x^4 +35 x^3 -50 x^2 +24 x ) = "<<testIntValue<<" != 5");
    return 1;
    }
  cout << "-> LinBairstowSolve found ( tolerance = " << tolLinBairstow
               << " ) " << testIntValue << " roots in " 
               << timer->GetElapsedTime() << " sec.:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << " " << roots[i] << "\n";

  // 3. find the roots of a quadratic trinomial with SturmBisectionSolve
  double P2[] = { 1., -2., 1. };
  PrintPolynomial( P2, 2 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::SturmBisectionSolve( P2, 2, rootInt, lowerBnds, tolSturm );
  timer->StopTimer();

  if ( testIntValue != 1 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( x^2 -  2 x + 1, ] -4 ; 4 ] ) found "<<testIntValue<<" root(s) instead of 1.");
    return 1;
    }
  if ( fabs( lowerBnds[0] - 1. ) > tolSturm )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( x^2 -  2 x + 1, ] -4 ; 4 ] ) found root "<<lowerBnds[0]<<" instead of 1 (within tolSturmerance of "<<tolSturm<<").");
    return 1;
    }
  cout << "-> SturmBisectionSolve bracketed " << testIntValue << " roots in ] " 
               << rootInt[0] << " ; "
               << rootInt[1] << " ] within tolerance "
               << tolSturm << " in "
               << timer->GetElapsedTime() << " sec:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << " ] " << lowerBnds[i] - tolSturm
                                                          << " ; " << lowerBnds[i]
                                                          << " ]\n";
  

  // 4. count, then find the roots of a degree 22 polynomial with SturmRootCount and SturmBisectionSolve
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
    vtkGenericWarningMacro("SturmRootCount( -0.0005 x^22 -0.001 x^21 +0.05 x^20 +0.1 x^19 -0.2 x^18 +1 x^17 -5.1 x^15 +4 x^13 -1 x^12 +0.2 x^11 +3 x^10 +2.2 x^9 +2 x^8 -7 x^7 -0.3 x^6 +3.8 x^5 +14 x^4 -16 x^3 +80 x^2 -97.9 x +5, ] -4 ; 4 ] ) = "<<testIntValue<<" != 5");
    return 1;
    }
  cout << "-> SturmRootCount counted " << testIntValue << " roots in ] " 
               << rootInt[0] << " ; "
               << rootInt[1] << " ] in "
               << timer->GetElapsedTime() << " sec.\n";

  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::SturmBisectionSolve( P22, 22, rootInt, lowerBnds, tolSturm );
  timer->StopTimer();

  if ( testIntValue != 5 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( -0.0005 x^22 -0.001 x^21 +0.05 x^20 +0.1 x^19 -0.2 x^18 +1 x^17 -5.1 x^15 +4 x^13 -1 x^12 +0.2 x^11 +3 x^10 +2.2 x^9 +2 x^8 -7 x^7 -0.3 x^6 +3.8 x^5 +14 x^4 -16 x^3 +80 x^2 -97.9 x +5, ] -4 ; 4 ] ) found "<<testIntValue<<" root(s) instead of 5");
    return 1;
    }
  cout << "-> SturmBisectionSolve bracketed " << testIntValue << " roots in ] " 
               << rootInt[0] << " ; "
               << rootInt[1] << " ] within tolerance "
               << tolSturm << " in "
               << timer->GetElapsedTime() << " sec:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << " ] " << lowerBnds[i] - tolSturm
                                                          << " ; " << lowerBnds[i]
                                                          << " ]\n";

  timer->Delete();
  return 0;
}
