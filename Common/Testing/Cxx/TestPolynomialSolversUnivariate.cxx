/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
#include "vtkPolynomialSolversUnivariate.h"
#include "vtkTimerLog.h"

//=============================================================================
void PrintPolynomial( double* P, unsigned int degP )
{
  cout << "\n";

  unsigned int degPm1 = degP - 1;
  for ( unsigned int i = 0; i < degPm1; ++ i ) 
    {
    if ( P[i] > 0 ) 
      {
      if ( i ) cout << "+" << P[i] << "*x**" << degP - i;
      else cout << P[i] << "*x**" << degP - i;
      }
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
int vtkRunPolynomial(
  double* poly, int degree, double* rootInt, double* upperBnds, 
  double tolSturm, double divtol, double* expected, int expectedLength, 
  double expectedTol, const char* name, bool divideGCD, bool useHabichtSolver = false )
{
  int rootcount;
  const char* sname = useHabichtSolver ? "Habicht" : "Sturm";

  vtkPolynomialSolversUnivariate::SetDivisionTolerance( divtol );
  if ( useHabichtSolver )
    {
    rootcount = vtkPolynomialSolversUnivariate::HabichtBisectionSolve(
      poly, degree, rootInt, upperBnds, tolSturm, 0, divideGCD );
    }
  else
    {
    rootcount = vtkPolynomialSolversUnivariate::SturmBisectionSolve(
      poly, degree, rootInt, upperBnds, tolSturm, 0, divideGCD );
    }

  if ( rootcount != expectedLength )
    {
    vtkGenericWarningMacro( << sname << "BisectionSolve( " << name << ", ]"
      << rootInt[0] << ", " << rootInt[1] << " [ ), found: "
      << rootcount << " roots, expected " << expectedLength << " roots.");
    return 1;
    }
  cout << "With DivisionTolerance: " << divtol << ", roots are:\n";
  for ( int i = 0; i < rootcount; ++ i )
    {
    cout << upperBnds[i] << endl;
    if ( fabs( upperBnds[i] - expected[i] ) > expectedTol )
      {
      vtkGenericWarningMacro( << sname << "BisectionSolve( " << name << ", ]"
        << rootInt[0] << ", " << rootInt[1] << " [ ), found: "
        << upperBnds[i] << ", expected " << expected[i] << ".");
      return 1;
      }
    }
  return 0;
}

//=============================================================================
int vtkTestPolynomials(
  double * poly, int degree, double* rootInt, double* upperBnds, 
  double tolSturm, double* divtols, int len, double* expectd, int expectedLength,
  double expectTol, const char* name, bool divideGCD )
{
  int rval;
  cout << endl << name << " (Sturm)" << endl;
  for ( int i = 0; i < len; ++ i )
    {
    rval = vtkRunPolynomial(poly, degree, rootInt, upperBnds, tolSturm, divtols[i], 
      expectd, expectedLength, expectTol, name, divideGCD, false );
    if ( rval == 1 )
      {
      return 1;
      }
    }

  cout << endl << name << " (Habicht)" << endl;
  for ( int i = 0; i < len; ++ i )
    {
    rval = vtkRunPolynomial(poly, degree, rootInt, upperBnds, tolSturm, divtols[i], 
      expectd, expectedLength, expectTol, name, divideGCD, true );
    if ( rval == 1 )
      {
      return 1;
      }
    }
  return 0;
}

//=============================================================================
// These polynomials are from the paper
// Jenkins, M. A. and J. F. Traub, "Principles for Testing Polynomial Zerofinding
//   Programs" in ACM Transactions on Mathematical Software, pp. 26--34, 1(1), 1975.
//
int vtkSolveJenkinsTraubPolynomials()
{
  double tolSturm = 1.e-7;
  //double tolRoots = 1.e-15;
  double roots[3000];
  double rootInt[] = { -14., 28.1 };
  //double otol = 1e-5;
  double divtol[] = {1e-5,1e-6, 1e-7, 1e-8, 1e-9, 1e-10};
  int limit = 6;
  //int rootcount;
  int stat = 0;

  // prod_{i=1}^{19}(x-i)
  double poly2[] = {
    1., -190., 16815., -920550., 34916946., -973941900., 20692933630.,
    -342252511900., 4465226757381., -46280647751910., 381922055502195.,
    -2.50385875546755e+015, 1.29536369899439e+016, -5.22609033625127e+016,
    1.61429736530119e+017, -3.71384787345228e+017, 6.10116075740492e+017,
    -6.68609730341153e+017, 4.31565146817638e+017, -1.21645100408832e+017
  };
  int degree = 19;
  double expected2[] = {
    1.,2.,3.,4.,5.,6.,7.,8.,9.,10.,11., 12., 13.,14.,15.,16.,17.,18.,19.
  };

  stat |= vtkTestPolynomials(
    poly2, degree, rootInt, roots, tolSturm, divtol, limit, expected2, 19, 1.9e-3, 
    "prod_{i=1}^{19}(x-i)", false ) << 1;

  // (x-.1)^3(x-.5)(x-.6)(x-.7)
  double poly4[] = { 1., -2.1, 1.64, -0.586, 0.0969, -0.00737, 0.00021 };
  double expected4[] = { .1, .5, .6, .7 };
  degree = 6;

  stat |= vtkTestPolynomials(
    poly4, degree, rootInt, roots, tolSturm, divtol, limit, expected4, 4, 1e-6, 
    "(x-.1)^3(x-.5)(x-.6)(x-.7)", false ) << 2;

  // (x-.1)^4(x-.2)^3(x-.3)^2(x-.4)
  double poly5[] = {
    1., -2., 1.75, -0.882, 0.2835, -0.06072, 0.008777,
    -0.0008458, 5.204e-005, -1.848e-006, 2.88e-008
  };
  double expected5[] = { .1, .2, .3, .4 };
  degree = 10;

  stat |= vtkTestPolynomials(
    poly5, degree, rootInt, roots, tolSturm, divtol, limit, expected5, 4, 1e-6,
    "(x-.1)^4(x-.2)^3(x-.3)^2(x-.4)", true ) << 3;

  // (x-.1)*(x-1.001)*(x-.998)*(x-1.00002)*(x-.99999)
  double poly6[] = {
    1., -4.0990099999999998, 6.3969289898000001, -4.5967287785602, 1.39871058773822, -0.099900798978019997,
    1., -4.09811, 6.3941407808, -4.59376253967838, 1.39754273695822, -0.099810978079838,
    1., -13.09901, 34.2790189898, -34.2591175967602, 13.0772183877618, -0.99810978079838,
    1., -4.09901, 6.39692899, -4.596728779, 1.398710588, -0.09990079898};
#if 0 
  // Works with Habicht but not Sturm.
  double expected6[] = { .1, 0.998, 0.99999, 1.00002, 1.001 };
  degree = 5;

  vtkTestPolynomials(
    poly6, degree, rootInt, roots, 1e-10, divtol, limit, expected6, 4, 1e-5, 
    "(x-.1)(x-1.001)(x-.998)(x-1.00002)(x-.99999)", false );
#endif // 0

  // (x+1)^6
  double poly8[] = { 1.0, 6.0, 15.0, 20.0, 15.0, 6.0, 1.0 };//{1.0, 5.0, 10.0, 10.0, 5.0, 1.0};
  double expected8[] = { -1.0 };
  degree = 6;

  stat |= vtkTestPolynomials(
    poly8, degree, rootInt, roots, tolSturm, divtol, limit, expected8, 1, 1e-7,
    "(x+1)^6", true ) << 4;
  // 
  double expected62[] = { -1 * poly6[1] };
  stat |= vtkTestPolynomials(
    poly6, 1, rootInt, roots, tolSturm, divtol, limit, expected62, 1, 1e-7,
    "poly6b", false ) << 5;

  // High multiplicities
  // (x-1)^6(x-2)^6(x-3)^6
  double polymult[] = {
    1., -36., 606., -6336., 46095., -247716., 1018816., -3278016., 8361951.,
    -17033580., 27767046., -36128736., 37235521., -29981196., 18442620.,
    -8362656., 2632176., -513216., 46656.
  };
  double expectedmult[] = { 1., 2., 3. };
  stat |= vtkTestPolynomials(
    polymult, 18, rootInt, roots, tolSturm, divtol, limit-1, expectedmult, 3, 1e-7, 
    "(x-1)^6(x-2)^6(x-3)^6", true ) << 6;

  return stat;
}

//=============================================================================
int TestPolynomialSolversUnivariate( int, char *[] )
{
  int testIntValue;
  double tolLinBairstow = 1.e-12;
  double tolSturm = 1.e-6;
  double tolRoots = 1.e-15;
  double tolDirectSolvers = VTK_DBL_EPSILON;
  double roots[22];
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
  testIntValue = vtkPolynomialSolversUnivariate::FerrariSolve( P4 + 1, roots, mult, tolDirectSolvers );
  timer->StopTimer();

  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("FerrariSolve(x^4 -7x^3 +17x^2 -17 x +6 ) = "<<testIntValue<<" != 3");
    timer->Delete();
    return 1;
    }
  cout << "Ferrari tol=" << tolDirectSolvers
               << ", " << testIntValue << " " 
               << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) 
    {
    cout << roots[i]; 
    if ( mult[i] > 1 ) cout << "(" << mult[i] << ")";
    cout << "\n";
    }
  double actualRoots[] = { 1., 2., 3. };
  int actualMult[] = { 2, 1, 1 };
  for ( int i = 0; i < testIntValue ; ++ i ) 
    {
    if ( fabs ( roots[i] - actualRoots[i] ) > tolRoots )
      {
      vtkGenericWarningMacro("FerrariSolve(x^4 -7x^3 +17x^2 -17 x +6, ]-4;4] ): root "<<roots[i]<<" != "<<actualRoots[i]);
      timer->Delete();
      return 1;  
      }
    if ( mult[i] != actualMult[i] )
      {
      vtkGenericWarningMacro("FerrariSolve(x^4 -7x^3 +17x^2 -17 x +6, ]-4;4] ): multiplicity "<<mult[i]<<" != "<<actualMult[i]);
      timer->Delete();
      return 1;  
      }
    }

  // 1.b SturmBisectionSolve
  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::SturmBisectionSolve( P4, 4, rootInt, upperBnds, tolSturm, 2, true );
  timer->StopTimer();

  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve(x^4 -7x^3 +17x^2 -17 x +6, ]-4;4] ): "<<testIntValue<<" root(s) instead of 3.");
    timer->Delete();
    return 1;  
    }
  cout << "SturmBisection +/-" << tolSturm << " ]" 
       << rootInt[0] << ";"
       << rootInt[1] << "], "
       << testIntValue << " "
       << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << upperBnds[i] - tolSturm * .5 << "\n";

  // 2. find the roots of a degree 5 polynomial with LinBairstowSolve
  double P5[] = { 1., -10., 35., -50., 24., 0. } ;
  PrintPolynomial( P5, 5 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::LinBairstowSolve( P5, 5, roots, tolLinBairstow );
  timer->StopTimer();

  if ( testIntValue != 5 )
    {
    vtkGenericWarningMacro("LinBairstowSolve(x^5 -10x^4 +35x^3 -50x^2 +24x ) = "<<testIntValue<<" != 5");
    timer->Delete();
    return 1;
    }
  cout << "LinBairstow tol=" << tolLinBairstow << ", " 
       << testIntValue << " " 
       << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << roots[i] << "\n";

  // 3. find the roots of a quadratic trinomial with SturmBisectionSolve
  double P2[] = { 1., -2., 1. };
  PrintPolynomial( P2, 2 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::SturmBisectionSolve( P2, 2, rootInt, upperBnds, tolSturm );
  timer->StopTimer();

  if ( testIntValue != 1 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve(x^2 -  2x + 1, ]-4;4] ): "<<testIntValue<<" root(s) instead of 1.");
    timer->Delete();
    return 1;
    }
  if ( fabs( upperBnds[0] - 1. ) > tolSturm )
    {
    vtkGenericWarningMacro("SturmBisectionSolve(x^2 -  2x + 1, ]-4;4] ): root "<<upperBnds[0]<<" instead of 1 (within tolSturmerance of "<<tolSturm<<").");
    timer->Delete();
    return 1;
    }
  cout << "SturmBisection +/-" << tolSturm << " ]" 
       << rootInt[0] << ";"
       << rootInt[1] << "], "
       << testIntValue << " "
       << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << upperBnds[i] - tolSturm * .5 << "\n";

  // 4. find the roots of a biquadratic trinomial with SturmBisectionSolve,
  // whose 2 double roots (-4 and 4) are also the bounds of the interval, thus
  // being a limiting case of Sturm's theorem, using:
  // 4.a FerrariSolve
  // 4.b SturmBisectionSolve

  double P4_2[] = { 1., 0., -32., 0., 256. };
  PrintPolynomial( P4_2, 4 );

  // 4.a FerrariSolve
  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::FerrariSolve( P4_2 + 1, roots, mult, tolDirectSolvers );
  timer->StopTimer();

  if ( testIntValue != 2 )
    {
    vtkGenericWarningMacro("FerrariSolve(x^4 -32x^2 +256 ) = "<<testIntValue<<" != 2");
    timer->Delete();
    return 1;
    }
  cout << "Ferrari tol=" << tolDirectSolvers
               << ", " << testIntValue << " " 
               << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) 
    {
    cout << roots[i]; 
    if ( mult[i] > 1 ) cout << "(" << mult[i] << ")";
    cout << "\n";
    }
  for ( int i = 0; i < testIntValue ; ++ i ) 
    {
    if ( fabs ( roots[i] ) - 4. > tolRoots )
      {
      vtkGenericWarningMacro("FerrariSolve(1*x**4-32*x**2+256, ]-4;4] ): root "<<roots[i]<<" != +/-4");
      timer->Delete();
      return 1;  
      }
    if ( mult[i] != 2 )
      {
      vtkGenericWarningMacro("FerrariSolve(1*x**4-32*x**2+256, ]-4;4] ): multiplicity "<<mult[i]<<" != 2");
      timer->Delete();
      return 1;  
      }
    }

  // 4.b SturmBisectionSolve
  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::SturmBisectionSolve( P4_2, 4, rootInt, upperBnds, tolSturm, 3 );
  timer->StopTimer();

  if ( testIntValue != 2 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve(1*x**4-32*x**2+256, [-4;4] ): "<<testIntValue<<" root(s) instead of 2.");
    timer->Delete();
    return 1;
    }
  if ( fabs( upperBnds[0] + 4. ) > tolSturm )
    {
    vtkGenericWarningMacro("SturmBisectionSolve(1*x**4-32*x**2+256, [-4;4] ): root "<<upperBnds[0]<<" instead of -4 (within tolerance of "<<tolSturm<<").");
    timer->Delete();
    return 1;
    }
  cout << "SturmBisection +/-" << tolSturm << " ]" 
       << rootInt[0] << ";"
       << rootInt[1] << "], "
       << testIntValue << " "
       << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << upperBnds[i] - tolSturm * .5 << "\n";
  

  // 5. find the quadruple roots of the degree 12 polynomial (x-1)^4 (x-2)^4 (x-3)^4
  // All roots are quadruple roots, making it challenging for solvers using floating
  // point arithmetic.
  rootInt[0] = 0.;
  rootInt[1] = 20.;
  double P12[] = {
    1,
    -24,
    260,
    -1680,
    7206,
    -21600,
    46364,
    -71760,
    79441,
    -61320,
    31320,
    -9504,
    1296
  };
  PrintPolynomial( P12, 12 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::SturmBisectionSolve( P12, 12, rootInt, upperBnds, tolSturm );
  timer->StopTimer();

  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( (x-1)^4 (x-2)^4 (x-3)^4, ]0;20] ): "<<testIntValue<<" root(s) instead of 3");
    timer->Delete();
    return 1;
    }
  cout << "SturmBisection +/-" << tolSturm << " ]" 
       << rootInt[0] << ";"
       << rootInt[1] << "], "
       << testIntValue << " "
       << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << upperBnds[i] - tolSturm * .5 << "\n";

  // 6. Find the roots of a degree 22 polynomial with SturmBisectionSolve
  rootInt[0] = -10.;
  rootInt[1] = 10.;
  double P22[] = {
    -0.0005, -0.001, 0.05, 0.1, -0.2,
    1., 0., -5.1, 0., 4., 
    -1., .2, 3., 2.2, 2.,
    -7., -.3, 3.8, 14., -16.,
    80., -97.9, 5. };
  PrintPolynomial( P22, 22 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::SturmBisectionSolve( P22, 22, rootInt, upperBnds, tolSturm );
  timer->StopTimer();

  if ( testIntValue != 8 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( -0.0005x^22 -0.001x^21 +0.05x^20 +0.1x^19 -0.2x^18 +1x^17 -5.1x^15 +4x^13 -1x^12 +0.2x^11 +3x^10 +2.2x^9 +2x^8 -7x^7 -0.3x^6 +3.8x^5 +14x^4 -16x^3 +80x^2 -97.9x +5, ]-10;10] ): "<<testIntValue<<" root(s) instead of 8");
    timer->Delete();
    return 1;
    }
  cout << "SturmBisection +/-" << tolSturm << " ]" 
       << rootInt[0] << ";"
       << rootInt[1] << "], "
       << testIntValue << " "
       << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << upperBnds[i] - tolSturm * .5 << "\n";

  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::LinBairstowSolve( P22, 22, roots, tolLinBairstow );
  timer->StopTimer();

  if ( testIntValue != 8 )
    {
    vtkGenericWarningMacro("LinBairstowSolve( -0.0005x^22 -0.001x^21 +0.05x^20 +0.1x^19 -0.2x^18 +1x^17 -5.1x^15 +4x^13 -1x^12 +0.2x^11 +3x^10 +2.2x^9 +2x^8 -7x^7 -0.3x^6 +3.8x^5 +14x^4 -16x^3 +80x^2 -97.9x +5, ]-10;10] ): "<<testIntValue<<" root(s) instead of 8");
    timer->Delete();
    return 1;
    }
  cout << "LinBairstow tol=" << tolLinBairstow << ", " 
       << testIntValue << " " 
       << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << roots[i] << "\n";

  // 7. Solving x^4 + 3x^3 - 4x + 1e-18 = 0 illustrates how the Ferrari solver
  // filters some numerical noise by noticing there is a double root.
  // This also exercises a case not otherwise tested.
  double P4_3[] = { 1., 3., -4., 0., 1.e-18 };
  PrintPolynomial( P4_3, 4 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::FerrariSolve( P4_3 + 1, roots, mult, tolDirectSolvers );
  timer->StopTimer();

  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("FerrariSolve(x^4 +3x^3 -3x^2 +1e-18 ) = "<<testIntValue<<" != 3");
    timer->Delete();
    return 1;
    }
  cout << "Ferrari tol=" << tolDirectSolvers
               << ", " << testIntValue << " " 
               << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) 
    {
    cout << roots[i]; 
    if ( mult[i] > 1 ) cout << "(" << mult[i] << ")";
    cout << "\n";
    }

  // 8. Solving x(x - 10^-4)^2 = 0 illustrates how the Tartaglia-Cardan solver
  // filters some numerical noise by noticing there is a double root (that
  // SolveCubic does not notice).
  double P3[] = { 1., -2.e-4, 1.e-8, 0.};
  PrintPolynomial( P3, 3 );

#if 0
  double r1, r2, r3;
  int nr;
  testIntValue = vtkMath::SolveCubic( P3[0], P3[1], P3[2], P3[3], &r1, &r2, &r3, &nr );
  if ( testIntValue != 2 )
    {
    vtkGenericWarningMacro("SolveCubic returned "<<testIntValue<<" != 3");
    timer->Delete();
    return 1;
    }
#endif // 0  

  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::TartagliaCardanSolve( P3 + 1, roots, mult, tolDirectSolvers );
  timer->StopTimer();

  if ( testIntValue != 2 )
    {
    vtkGenericWarningMacro("TartagliaCardanSolve returned "<<testIntValue<<" != 2");
    timer->Delete();
    return 1;
    }
  cout << "TartagliaCardan tol=" << tolDirectSolvers
               << ", " << testIntValue << " " 
               << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) 
    {
    cout << roots[i]; 
    if ( mult[i] > 1 ) cout << "(" << mult[i] << ")";
    cout << "\n";
    }

  // 9. Solving x^3+x^2+x+1 = 0 to exercise a case not otherwise tested.
  double P3_2[] = { 1., 1., 1., 1.};
  PrintPolynomial( P3_2, 3 );

#if 0
  double r1, r2, r3;
  int nr;
  testIntValue = vtkMath::SolveCubic( P3_2[0], P3_2[1], P3_2[2], P3_2[3], &r1, &r2, &r3, &nr );
  if ( testIntValue != 1 )
    {
    vtkGenericWarningMacro("SolveCubic returned "<<testIntValue<<" != 1");
    timer->Delete();
    return 1;
    }
#endif // 0  

  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::TartagliaCardanSolve( P3_2 + 1, roots, mult, tolDirectSolvers );
  timer->StopTimer();

  if ( testIntValue != 1 )
    {
    vtkGenericWarningMacro("TartagliaCardanSolve returned "<<testIntValue<<" != 1");
    timer->Delete();
    return 1;
    }
  cout << "TartagliaCardan tol=" << tolDirectSolvers
               << ", " << testIntValue << " " 
               << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) 
    {
    cout << roots[i]; 
    if ( mult[i] > 1 ) cout << "(" << mult[i] << ")";
    cout << "\n";
    }

  // 10. Solving x^3 - 2e-6 x^2 + 0.999999999999999e-12 x = 0 to test a nearly degenerate case.
  double P3_3[] = { 1., -2.e-6 , .999999999999999e-12, 0.};
  PrintPolynomial( P3_3, 3 );

#if 0
  double r1, r2, r3;
  int nr;
  testIntValue = vtkMath::SolveCubic( P3_3[0], P3_3[1], P3_3[2], P3_3[3], &r1, &r2, &r3, &nr );
  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("SolveCubic returned "<<testIntValue<<" != 3");
    timer->Delete();
    return 1;
    }
#endif // 0  

  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::TartagliaCardanSolve( P3_3 + 1, roots, mult, tolDirectSolvers );
  timer->StopTimer();

  if ( testIntValue != 3 )
    {
    vtkGenericWarningMacro("TartagliaCardanSolve returned "<<testIntValue<<" != 3");
    timer->Delete();
    return 1;
    }
  cout << "TartagliaCardan tol=" << tolDirectSolvers
               << ", " << testIntValue << " " 
               << timer->GetElapsedTime() << "s\n";
  cout.precision( 9 );
  for ( int i = 0; i < testIntValue ; ++ i ) 
    {
    cout << roots[i]; 
    if ( mult[i] > 1 ) cout << "(" << mult[i] << ")";
    cout << "\n";
    }

  // 11. Find the roots of a sparse degree 10 polynomial with SturmBisectionSolve to exercise a particular
  // case of the Euclidean division routine, where the remainder does not have maximal degree. 
  rootInt[0] = -10.;
  rootInt[1] = 10.;
  double P10[] = { 76., 0., 0., 0., 0., 0., 0., 0., 95., 0., -14. };
  PrintPolynomial( P10, 10 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::SturmBisectionSolve( P10, 10, rootInt, upperBnds, tolSturm );
  timer->StopTimer();

  if ( testIntValue != 2 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( 76x^10 +95x^2 -14, ]-10;10] ): "<<testIntValue<<" root(s) instead of 2");
    timer->Delete();
    return 1;
    }
  cout << "SturmBisection +/-" << tolSturm << " ]" 
       << rootInt[0] << ";"
       << rootInt[1] << "], "
       << testIntValue << " "
       << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i ) cout << upperBnds[i] - tolSturm * .5 << "\n";

  // 11. Find the roots of a sparse degree 10 polynomial with SturmBisectionSolve to exercise a particular
  // case of the Euclidean division routine, where the remainder does not have maximal degree. 
  rootInt[0] = -10.;
  rootInt[1] = 10.;
  double P84[] = 
    {
      55.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      -79.000000,
      90.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      37.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      49.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      0.000000,
      -12.000000
    };
  PrintPolynomial( P84, 84 );

  timer->StartTimer();
  testIntValue = vtkPolynomialSolversUnivariate::SturmBisectionSolve( P84, 84, rootInt, upperBnds, tolSturm );
  timer->StopTimer();

  if ( testIntValue != 2 )
    {
    vtkGenericWarningMacro("SturmBisectionSolve( 55x^84-79x^72+90x^71+37x^44+49x^21-12, ]-10;10] ): "<<testIntValue<<" root(s) instead of 2");
    timer->Delete();
    return 1;
    }
  cout << "SturmBisection +/-" << tolSturm << " ]" 
       << rootInt[0] << ";"
       << rootInt[1] << "], "
       << testIntValue << " "
       << timer->GetElapsedTime() << "s\n";
  for ( int i = 0; i < testIntValue ; ++ i )
    {
    cout << upperBnds[i] - tolSturm * .5 << "\n";
    }

  // Now try the Sturm solver on some benchmark polynomials from a paper by Jenkins and Traub.
  int status = vtkSolveJenkinsTraubPolynomials(); 

  timer->Delete();
  return status;
}
