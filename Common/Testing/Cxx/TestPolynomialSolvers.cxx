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
  cout << "\nP = ";

  unsigned int degPm1 = degP - 1;
  for ( unsigned int i = 0; i < degPm1; ++ i ) 
    {
    if ( P[i] > 0 ) cout << "+" << P[i] << "*x**" << degP - i;
    else if ( P[i] < 0 ) cout << P[i] << "*x**" << degP - i;
    }   

  if ( degP > 0 )
    {
    if ( P[degPm1] > 0 ) cout << "+" << P[degPm1] << "*x";
    else if ( P[degPm1] < 0 ) cout << P[degPm1] << "*x";
    }

  if ( P[degP] > 0 ) cout << "+" << P[degP];
  else if ( P[degP] < 0 ) cout << P[degP];

  cout << "\n";
}

//=============================================================================
int TestPolynomialSolvers( int, char *[] )
{
  int testIntValue;
  double tolLinBairstow = 1.e-12;
  double tolSturm = 1.e-6;
  double tolRoots = 1.e-15;
  double roots[5];
  int mult[4];
  double rootInt[] = { -4., 4. };
  double upperBnds[22];
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
    vtkGenericWarningMacro("FerrariSolve(x^4 -7x^3 +17x^2 -17 x +6 ) = "<<testIntValue<<" != 3");
    return 1;
    }
  cout << "FerrariSolve found (tol= " << 0
               << ") " << testIntValue << " roots in " 
               << timer->GetElapsedTime() << " sec.:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << roots[i] 
                                                  << ", mult. " << mult[i] 
                                                  << "\n";
  double actualRoots[] = { 1., 2., 3. };
  int actualMult[] = { 2, 1, 1 };
  for ( int i = 0; i < testIntValue ; ++ i ) 
    {
    if ( fabs ( roots[i] - actualRoots[i] ) > tolRoots )
      {
      vtkGenericWarningMacro("FerrariSolve(x^4 -7x^3 +17x^2 -17 x +6, ]-4;4] ) found root "<<roots[i]<<" != "<<actualRoots[i]);
      return 1;  
      }
    if ( mult[i] != actualMult[i] )
      {
      vtkGenericWarningMacro("FerrariSolve(x^4 -7x^3 +17x^2 -17 x +6, ]-4;4] ) found multiplicity "<<mult[i]<<" != "<<actualMult[i]);
      return 1;  
      }
    }

  // 1.b SturmBissectionSolve
  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::SturmBisectionSolve( P4, 4, rootInt, upperBnds, tolSturm );
  timer->StopTimer();

  cout << "SturmBisectionSolve bracketed " << testIntValue << " roots in ]" 
               << rootInt[0] << ";"
               << rootInt[1] << "] within "
               << tolSturm << " in "
               << timer->GetElapsedTime() << " sec:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << upperBnds[i] - tolSturm * .5 << "\n";
  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve(x^4 -7x^3 +17x^2 -17 x +6, ]-4;4] ) found "<<testIntValue<<" root(s) instead of 3.");
    return 1;  
    }

  // 2. find the roots of a degree 5 polynomial with LinBairstowSolve
  double P5[] = { 1., -10., 35., -50., 24., 0. } ;
  PrintPolynomial( P5, 5 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::LinBairstowSolve( P5, 5, roots, tolLinBairstow );
  timer->StopTimer();

  if ( testIntValue != 5 )
    {
    vtkGenericWarningMacro("LinBairstowSolve(x^5 -10x^4 +35x^3 -50x^2 +24x ) = "<<testIntValue<<" != 5");
    return 1;
    }
  cout << "LinBairstowSolve found (tol= " << tolLinBairstow
               << ") " << testIntValue << " roots in " 
               << timer->GetElapsedTime() << " sec.:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << roots[i] << "\n";

  // 3. find the roots of a quadratic trinomial with SturmBisectionSolve
  double P2[] = { 1., -2., 1. };
  PrintPolynomial( P2, 2 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::SturmBisectionSolve( P2, 2, rootInt, upperBnds, tolSturm );
  timer->StopTimer();

  if ( testIntValue != 1 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve(x^2 -  2x + 1, ]-4;4] ) found "<<testIntValue<<" root(s) instead of 1.");
    return 1;
    }
  if ( fabs( upperBnds[0] - 1. ) > tolSturm )
    {
    vtkGenericWarningMacro("SturmBisectionSolve(x^2 -  2x + 1, ]-4;4] ) found root "<<upperBnds[0]<<" instead of 1 (within tolSturmerance of "<<tolSturm<<").");
    return 1;
    }
  cout << "SturmBisectionSolve bracketed " << testIntValue << " roots in ]" 
               << rootInt[0] << ";"
               << rootInt[1] << "] within "
               << tolSturm << " in "
               << timer->GetElapsedTime() << " sec:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << upperBnds[i] - tolSturm * .5 << "\n";

  // 4. find the roots of a biquadratic trinomial with SturmBisectionSolve,
  // whose 2 double roots (-4 and 4) are also the bounds of the interval, thus
  // being a limiting case of Sturm's theorem, using:
  // 4.a FerrariSolve
  // 4.b SturmBissectionSolve

  double P4_2[] = { 1., 0., -32., 0., 256. };
  PrintPolynomial( P4_2, 4 );

  // 4.a FerrariSolve
  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::FerrariSolve( P4_2 + 1, roots, mult, 0. );
  timer->StopTimer();

  if ( testIntValue != 2 )
    {
    vtkGenericWarningMacro("FerrariSolve(x^4 -32x^2 +256 ) = "<<testIntValue<<" != 2");
    return 1;
    }
  cout << "FerrariSolve found (tol= " << 0
               << ") " << testIntValue << " roots in " 
               << timer->GetElapsedTime() << " sec.:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << roots[i] 
                                                  << ", mult. " << mult[i] 
                                                  << "\n";
  for ( int i = 0; i < testIntValue ; ++ i ) 
    {
    if ( fabs ( roots[i] ) - 4. > tolRoots )
      {
      vtkGenericWarningMacro("FerrariSolve(, ]-4;4] ) found root "<<roots[i]<<" != +/-4");
      return 1;  
      }
    if ( mult[i] != 2 )
      {
      vtkGenericWarningMacro("FerrariSolve(, ]-4;4] ) found multiplicity "<<mult[i]<<" != 2");
      return 1;  
      }
    }

  // 4.b SturmBissectionSolve
  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::SturmBisectionSolve( P4_2, 4, rootInt, upperBnds, tolSturm );
  timer->StopTimer();

  if ( testIntValue != 2 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve(x^2 -  2x + 1, ]-4;4] ) found "<<testIntValue<<" root(s) instead of 2.");
    return 1;
    }
  if ( fabs( upperBnds[0] - 4. ) > tolSturm )
    {
    vtkGenericWarningMacro("SturmBisectionSolve(x^2 -  2x + 1, ]-4;4] ) found root "<<upperBnds[0]<<" instead of 1 (within tolSturmerance of "<<tolSturm<<").");
    return 1;
    }
  cout << "SturmBisectionSolve bracketed " << testIntValue << " roots in ]" 
               << rootInt[0] << ";"
               << rootInt[1] << "] within "
               << tolSturm << " in "
               << timer->GetElapsedTime() << " sec:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << upperBnds[i] - tolSturm * .5 << "\n";
  

  // 5. Find the roots of a degree 22 polynomial with SturmBisectionSolve
  double P22[] = {
    -0.0005, -0.001, 0.05, 0.1, -0.2,
    1., 0., -5.1, 0., 4., 
    -1., .2, 3., 2.2, 2.,
    -7., -.3, 3.8, 14., -16.,
    80., -97.9, 5. };
  PrintPolynomial( P22, 22 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::SturmBisectionSolve( P22, 22, rootInt, upperBnds, tolSturm );
  timer->StopTimer();

  if ( testIntValue != 5 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( -0.0005x^22 -0.001x^21 +0.05x^20 +0.1x^19 -0.2x^18 +1x^17 -5.1x^15 +4x^13 -1x^12 +0.2x^11 +3x^10 +2.2x^9 +2x^8 -7x^7 -0.3x^6 +3.8x^5 +14x^4 -16x^3 +80x^2 -97.9x +5, ]-4;4] ) found "<<testIntValue<<" root(s) instead of 5");
    return 1;
    }
  cout << "SturmBisectionSolve bracketed " << testIntValue << " roots in ]" 
               << rootInt[0] << ";"
               << rootInt[1] << "] within tol. "
               << tolSturm << " in "
               << timer->GetElapsedTime() << " sec:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << upperBnds[i] - tolSturm * .5 << "\n";

  // 6. Solving x(x - 10^-4)^2 = 0 illustrates how the Tartaglia-Cardan solver
  // filters some numerical noise by noticing there is a double root (that
  // SolveCubic does not notice).
  double P3[] = { 1., -2.e-4, 1.e-8, 0.};
  PrintPolynomial( P3, 3 );

#if 0
  double r1, r2, r3;
  int nr;
  testIntValue = vtkMath::SolveCubic( P3[0], P3[1], P3[2], P3[3], &r1, &r2, &r3, &nr );
  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("SolveCubic returned "<<testIntValue<<" != 3");
    return 1;
    }
#endif // 0  

  timer->StartTimer();
  testIntValue = vtkPolynomialSolvers::TartagliaCardanSolve( P3, roots, mult );
  timer->StopTimer();

  if ( testIntValue != 2 )
    {
    vtkGenericWarningMacro("TartagliaCardanSolve returned "<<testIntValue<<" != 2");
    return 1;
    }
  cout << "TartagliaCardanSolve found (tol= " << 0
               << ") " << testIntValue << " roots in " 
               << timer->GetElapsedTime() << " sec.:\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << roots[i] 
                                                  << ", mult. " << mult[i] 
                                                  << "\n";


  timer->Delete();
  return 0;
}
