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

static int vtkRunPolynomial(
  double* poly, int degree, double* rootInt, double* upperBnds, 
  double tolSturm, double divtol, double* expected, int expectedLength, 
  double expectedTol, const char* name, bool divideGCD, bool useHabichtSolver, int intType)
{
  int rootcount;
  const char* sname = useHabichtSolver ? "Habicht" : "Sturm";
  vtkTimerLog* timer = vtkTimerLog::New();

  vtkPolynomialSolversUnivariate::SetDivisionTolerance( divtol );
  if ( useHabichtSolver )
    {
    timer->StartTimer();
    rootcount = vtkPolynomialSolversUnivariate::HabichtBisectionSolve( 
      poly, degree, rootInt, upperBnds, tolSturm, intType, divideGCD );
    timer->StopTimer();
    }
  else
    {
    timer->StartTimer();
    rootcount = vtkPolynomialSolversUnivariate::SturmBisectionSolve( 
      poly, degree, rootInt, upperBnds, tolSturm, intType, divideGCD );
    timer->StopTimer();
    }

  int trueexp = expectedLength;
  cout << "divtol is: " << divtol << ", " << timer->GetElapsedTime() << "s" << endl;
  timer->Delete();

  int j = 0;
  for(int i = 0; i < rootcount; i++, j++)
    {
    cout << upperBnds[i] << endl;
    while(j < expectedLength && 
        (
          ( ((intType & 1) == 0) && expected[j] == rootInt[0] ) ||
          ( ((intType & 2) == 0) && expected[j] == rootInt[1] ) ||
          expected[j] < rootInt[0] ||
          expected[j] > rootInt[1]
        ))
      {
      j++;
      trueexp--;
      }
    if ( j >= expectedLength )
      {
      vtkGenericWarningMacro( <<  sname << "BisectionSolve( " << name << ", " << ((intType&1)?"[":"]") << rootInt[0] << ", " << rootInt[1] << " " << ((intType&2)?"]":"[") << " )," 
        << upperBnds[i] << " found but not expected." << "." );
      return 1;
      }
    else if ( fabs( upperBnds[i] - expected[j] ) > expectedTol)
      {
      vtkGenericWarningMacro( << sname << "BisectionSolve( " << name << ", " << ((intType&1)?"[":"]") << rootInt[0] << ", " << rootInt[1] << " " << ((intType&2)?"]":"[") << " ),"
        << " found: "
        << upperBnds[i] << ", expected " << expected[j] << ".");
      return 1;
      }
    }
  while(j < expectedLength)
    {
    if ( expected[j] <= rootInt[0] || expected[j] >= rootInt[1] )
      {
      trueexp--;
      }
    j++;
    }
  if( rootcount != trueexp )
    {
    vtkGenericWarningMacro( << sname << "BisectionSolve( " << name << ", " << ((intType&1)?"[":"]") << rootInt[0] << ", " << rootInt[1] << " " << ((intType&2)?"]":"[") << " ), found: "
      << rootcount << " roots, expected " << trueexp << " roots.");
    return 1;
    }

  return 0;
}

//=============================================================================
static int vtkTestPolynomials(
  double * poly, int degree, double* rootInt, double* upperBnds, 
  double tolSturm, double* divtols, int len, double* expectd, int expectedLength,
  double expectTol, const char* name, bool divideGCD, int methods = 3, int intType = 0 )
{
  int rval = 0;
  if( methods & 1 )
    {
    cout << endl << name << " (Sturm)" << endl;
    for ( int i = 0; i < len; ++ i )
      {
      rval |= vtkRunPolynomial(poly, degree, rootInt, upperBnds, tolSturm, divtols[i], 
        expectd, expectedLength, expectTol, name, divideGCD, false, intType );
      }
    }

  if( methods & 2 )
    {
    cout << endl << name << " (Habicht)" << endl;
    for ( int i = 0; i < len; ++ i )
      {
      rval |= vtkRunPolynomial(poly, degree, rootInt, upperBnds, tolSturm, divtols[i], 
        expectd, expectedLength, expectTol, name, divideGCD, true, intType );
      }
    }
  return rval;
}

//=============================================================================
int TestPolynomialSolversUnivariate( int len, char * c[] )
{
  (void) len;
  (void) c;
  double tolSturm = 1.e-18;
  double tolRoots = 1.e-15;
  double roots[3000];
  double rootInt[] = { -14., 28.1 };
  double divtol[] = { 1e-5,1e-6, 1e-7, 1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13 };
  int limit = 6;
  int stat = 0;
  vtkTimerLog* timer = vtkTimerLog::New();
  timer->AllocateLog();

  cout << "------Testing set 'Jenkins Traub'------" << endl;
  cout << 13 << " polynomials in the set." << endl;
  // Testing mul(x-i, i=1..19)
  double JT2[] = {
    1.000000000000000e+00, -1.900000000000000e+02, 1.681500000000000e+04, 
    -9.205500000000000e+05, 3.491694600000000e+07, -9.739419000000000e+08, 
    2.069293363000000e+10, -3.422525119000000e+11, 4.465226757381000e+12, 
    -4.628064775191000e+13, 3.819220555021950e+14, -2.503858755467550e+15, 
    1.295363698994390e+16, -5.226090336251272e+16, 1.614297365301190e+17, 
    -3.713847873452280e+17, 6.101160757404918e+17, -6.686097303411533e+17, 
    4.315651468176384e+17, -1.216451004088320e+17
  };
  double JT2RTS[] = {
    1.000000000000000e+00, 2.000000000000000e+00, 3.000000000000000e+00, 
    4.000000000000000e+00, 5.000000000000000e+00, 6.000000000000000e+00, 
    7.000000000000000e+00, 8.000000000000000e+00, 9.000000000000000e+00, 
    1.000000000000000e+01, 1.100000000000000e+01, 1.200000000000000e+01, 
    1.300000000000000e+01, 1.400000000000000e+01, 1.500000000000000e+01, 
    1.600000000000000e+01, 1.700000000000000e+01, 1.800000000000000e+01, 
    1.900000000000000e+01
  };
  stat |= vtkTestPolynomials(JT2, 19, rootInt, roots,
                tolSturm, divtol, limit, 
                JT2RTS, 19, 2.500000e-03, "mul(x-i, i=1..19)", false, 3, 0);

  double JT22[] = {
    1., -171., 13566., -662796., 22323822., -549789282., 10246937272., 
    -147560703732., 1661573386473., -14710753408923., 102417740732658., 
    -557921681547048., 2.35312504054998e+015, -7.55152759206302e+015, 
    1.79507122809215e+016, -3.03212540077194e+016, 3.40122495938227e+016, 
    -2.23769880585216e+016, 6.402373705728e+015
  };
  stat |= vtkTestPolynomials(JT22, 18, rootInt, roots,
                tolSturm, divtol, limit, 
                JT2RTS, 18, 0.500000e-03, "mul(x-i, i=1..19)", false, 3, 0);

  double JT23[] = {
   1., -153., 10812., -468180., 13896582., -299650806., 4853222764., 
   -60202693980., 577924894833., -4308105301929., 24871845297936., 
   -110228466184200., 369012649234384., -909299905844112., 1.58331397572749e+015, 
   -1.82160244462464e+015, 1.2234055905792e+015, -355687428096000. 
  };
  stat |= vtkTestPolynomials(JT23, 17, rootInt, roots,
                tolSturm, divtol, limit, 
                JT2RTS, 17, 1e-4, "mul(x-i, i=1..19)", false, 3, 0);



  // Testing mul(x-10^(-i),i=1..8)
  double JT3[] = {
    1.000000000000000e+00, -1.111111100000000e-01, 1.122334332211000e-03, 
    -1.123456666543211e-06, 1.123557787755321e-10, -1.123456666543211e-15, 
    1.122334332211000e-21, -1.111111100000000e-28, 1.000000000000000e-36
  };
  double JT3RTS[] = {
    1.000000000000000e-08, 1.000000000000000e-07, 1.000000000000000e-06, 
    1.000000000000000e-05, 1.000000000000000e-04, 1.000000000000000e-03, 
    1.000000000000000e-02, 1.000000000000000e-01
  };
  stat |= vtkTestPolynomials(JT3, 8, rootInt, roots,
                1e-9, divtol+1, limit, 
                JT3RTS, 8, 1.000000e-08, "mul(x-10^(-i),i=1..8)", false, 3, 0);

  // Testing (x-1/10)^3*(x-5/10)*(x-6/10)*(x-7/10)
  double JT4[] = {
    1.000000000000000e+00, -2.100000000000000e+00, 1.640000000000000e+00, 
    -5.860000000000000e-01, 9.690000000000000e-02, -7.370000000000000e-03, 
    2.100000000000000e-04
  };
  double JT4RTS[] = {
    1.000000000000000e-01, 5.000000000000000e-01, 6.000000000000000e-01, 
    7.000000000000000e-01
  };
  stat |= vtkTestPolynomials(JT4, 6, rootInt, roots,
                tolSturm, divtol, limit, 
                JT4RTS, 4, 1.000000e-07, "(x-1/10)^3*(x-5/10)*(x-6/10)*(x-7/10)", true, 3, 0);

  // Testing (x-1/10)^4*(x-2/10)^3*(x-3/10)^2*(x-4/10)
  double JT5[] = {
    1.000000000000000e+00, -2.000000000000000e+00, 1.750000000000000e+00, 
    -8.820000000000000e-01, 2.835000000000000e-01, -6.072000000000000e-02, 
    8.777000000000000e-03, -8.458000000000000e-04, 5.204000000000000e-05, 
    -1.848000000000000e-06, 2.880000000000000e-08
  };
  double JT5RTS[] = {
    1.000000000000000e-01, 2.000000000000000e-01, 3.000000000000000e-01, 
    4.000000000000000e-01
  };
  stat |= vtkTestPolynomials(JT5, 10, rootInt, roots,
                tolSturm, divtol, limit, 
                JT5RTS, 4, 1.000000e-07, "(x-1/10)^4*(x-2/10)^3*(x-3/10)^2*(x-4/10)", true, 3, 0);


  // Testing 25*10^9*(x-1/10)*(x-1001/1000)*(x-998/1000)*(x-100002/100000)*(x-99999/100000)
  double JT62[] = {
    2.500000000000000e+10, -1.024752500000000e+11, 1.599232247450000e+11, 
    -1.149182194640050e+11, 3.496776469345551e+10, -2.497519974450501e+09
  };
  double JT62RTS[] = {
    1.000000000000000e-01, 9.980000000000000e-01, 9.999900000000000e-01, 
    1.000020000000000e+00, 1.001000000000000e+00
  };
  stat |= vtkTestPolynomials(JT62, 5, rootInt, roots,
                tolSturm, divtol+5, limit-2, 
                JT62RTS, 5, 1.000000e-05, "25*10^9*(x-1/10)*(x-1001/1000)*(x-998/1000)*(x-100002/100000)*(x-99999/100000)", false, 3, 0);

#if !defined(_MSC_VER) || ( defined(_MSC_VER) && (_MSC_VER != 1310) )
  // Testing (x-1/10)*(x-1001/1000)*(x-998/1000)*(x-100002/100000)*(x-99999/100000)
  // This only works with the Habicht Sequence, and not on MSVC 7.1
  double JT[] = {
    JT62[0]/JT62[0], JT62[1]/JT62[0], JT62[2]/JT62[0],
    JT62[3]/JT62[0], JT62[4]/JT62[0], JT62[5]/JT62[0]
  };
  stat |= vtkTestPolynomials(JT, 5, rootInt, roots,
                tolSturm, divtol+5, limit-2, 
                JT62RTS, 5, 1.000000e-05, "(x-1/10)*(x-1001/1000)*(x-998/1000)*(x-100002/100000)*(x-99999/100000)", false, 2, 0);
#endif


  // Testing (x+1)^5
  double JT8[] = {
    1.000000000000000e+00, 5.000000000000000e+00, 1.000000000000000e+01, 
    1.000000000000000e+01, 5.000000000000000e+00, 1.000000000000000e+00
  };
  double JT8RTS[] = {
    -1.000000000000000e+00
  };
  stat |= vtkTestPolynomials(JT8, 5, rootInt, roots,
                tolSturm, divtol, limit, 
                JT8RTS, 1, 1.000000e-07, "(x+1)^5", true, 3, 0);

  rootInt[0] = -1e14;
  // Testing (x-10^(-13))*(x+10^(13))
  double JT9[] = {
    1.000000000000000e+00, 1.000000000000000e+13, -1.000000000000000e+00
  };
  double JT9RTS[] = {
    -1.000000000000000e+13, 1.000000000000000e-13
  };
  stat |= vtkTestPolynomials(JT9, 2, rootInt, roots,
                tolSturm, divtol, limit, 
                JT9RTS, 2, 1.000000e-07, "(x-10^(-13))*(x+10^(13))", false, 3, 0);

  rootInt[0] = -14;
  rootInt[1] = 1e4;
  // Testing (x-10^3)*(x-1)*(x-10^(-3))
  double P10a[] = {
    1.000000000000000e+00, -1.001001000000000e+03, 1.001001000000000e+03, 
    -1.000000000000000e+00
  };
  double P10aRTS[] = {
    1.000000000000000e-03, 1.000000000000000e+00, 1.000000000000000e+03
  };
  stat |= vtkTestPolynomials(P10a, 3, rootInt, roots,
                tolSturm, divtol, limit, 
                P10aRTS, 3, 1.000000e-07, "(x-10^3)*(x-1)*(x-10^(-3))", false, 3, 0);

  rootInt[1] = 1e7;
  // Testing (x-10^6)*(x-1)*(x-10^(-6))
  double P10b[] = {
    1.000000000000000e+00, -1.000001000001000e+06, 1.000001000001000e+06, 
    -1.000000000000000e+00
  };
  double P10bRTS[] = {
    1.000000000000000e-06, 1.000000000000000e+00, 1.000000000000000e+06
  };
  stat |= vtkTestPolynomials(P10b, 3, rootInt, roots,
                tolSturm, divtol+1, limit-1, 
                P10bRTS, 3, 1.000000e-07, "(x-10^6)*(x-1)*(x-10^(-6))", false, 3, 0);

  rootInt[1] = 1e10;
  // Testing (x-10^9)*(x-1)*(x-10^(-9))
  double P10c[] = {
    1.000000000000000e+00, -1.000000001000000e+09, 1.000000001000000e+09, 
    -1.000000000000000e+00
  };
  double P10cRTS[] = {
    1.000000000000000e-09, 1.000000000000000e+00, 1.000000000000000e+09
  };
  stat |= vtkTestPolynomials(P10c, 3, rootInt, roots,
                tolSturm, divtol+4, limit-2, 
                P10cRTS, 3, 1.000000e-07, "(x-10^9)*(x-1)*(x-10^(-9))", false, 3, 0);

  rootInt[1] = 28.1;
  cout << "------Testing set 'Igarashi Ympa'------" << endl;
  cout << 2 << " polynomials in the set." << endl;
  // Testing (x-2.35)*(x-2.37)*(x-2.39)
  double igyp00[] = {
    1.000000000000000e+00, -7.110000000000000e+00, 1.685030000000000e+01, 
    -1.331110500000000e+01
  };
  double igyp00RTS[] = {
    2.350000000000000e+00, 2.370000000000000e+00, 2.390000000000000e+00
  };
  stat |= vtkTestPolynomials(igyp00, 3, rootInt, roots,
                tolSturm, divtol, limit, 
                igyp00RTS, 3, 1.000000e-07, "(x-2.35)*(x-2.37)*(x-2.39)", false, 3, 0);

  // Testing (x-2.35)^3*(x-2.37)
  double igyp01[] = {
    1.000000000000000e+00, -9.420000000000000e+00, 3.327600000000000e+01, 
    -5.224285000000000e+01, 3.075756375000000e+01
  };
  double igyp01RTS[] = {
    2.350000000000000e+00, 2.370000000000000e+00
  };
  stat |= vtkTestPolynomials(igyp01, 4, rootInt, roots,
                tolSturm, divtol+1, limit, 
                igyp01RTS, 2, 1.000000e-07, "(x-2.35)^3*(x-2.37)", true, 3, 0);

  cout << "------Testing set 'Iliev'------" << endl;
  cout << 2 << " polynomials in the set." << endl;
  // Testing (x-1)*(x+2)^2*(x+3)^3
  double iliev01[] = {
    1.000000000000000e+00, 1.200000000000000e+01, 5.400000000000000e+01, 
    1.040000000000000e+02, 4.500000000000000e+01, -1.080000000000000e+02, 
    -1.080000000000000e+02
  };
  double iliev01RTS[] = {
    -3.000000000000000e+00, -2.000000000000000e+00, 1.000000000000000e+00
  };
  stat |= vtkTestPolynomials(iliev01, 6, rootInt, roots,
                tolSturm, divtol, limit, 
                iliev01RTS, 3, 1.000000e-07, "(x-1)*(x+2)^2*(x+3)^3", true, 3, 0);

  // Testing (x-1)^2*(x-2)^4*(x-3)^6
  double iliev02[] = {
    1.000000000000000e+00, -2.800000000000000e+01, 3.560000000000000e+02, 
    -2.716000000000000e+03, 1.383800000000000e+04, -4.956400000000000e+04, 
    1.278520000000000e+05, -2.390760000000000e+05, 3.212730000000000e+05, 
    -3.021840000000000e+05, 1.885680000000000e+05, -6.998400000000000e+04, 
    1.166400000000000e+04
  };
  double iliev02RTS[] = {
    1.000000000000000e+00, 2.000000000000000e+00, 3.000000000000000e+00
  };
  stat |= vtkTestPolynomials(iliev02, 12, rootInt, roots,
                tolSturm, divtol, limit-1, 
                iliev02RTS, 3, 1.000000e-07, "(x-1)^2*(x-2)^4*(x-3)^6", true, 3, 0);

  cout << "------Testing other sets------" << endl;
  // High multiplicities
  // (x-1)^6(x-2)^6(x-3)^6
  double polymult[] = {1., -36., 606., -6336., 46095., -247716., 1018816., -3278016., 8361951., -17033580., 27767046., 
    -36128736., 37235521., -29981196., 18442620., -8362656., 2632176., -513216., 46656.};
  double expectedmult[] = {1.,2.,3.};
  stat |= vtkTestPolynomials(polymult, 18, rootInt, roots, tolSturm, divtol, limit-1, expectedmult, 3, 1e-7, 
    "(x-1)^6(x-2)^6(x-3)^6", true, 1);

  stat |= vtkTestPolynomials(polymult, 18, rootInt, roots, tolSturm, divtol, limit, expectedmult, 3, 1e-7, 
    "(x-1)^6(x-2)^6(x-3)^6", true, 2);

  // Zeng Polynomials
  double twin01[] = {1., -3.96, 7.1366, -7.736316, 5.61584481, -2.874527064, 1.0633461368, 
    -0.286299144, 0.055658703376, -0.007616429184, 0.00069613420032, -3.8146387968e-005, 
    9.475854336e-007};
  double twinroots[] = {.2,.39,.4};

  stat |= vtkTestPolynomials(twin01, 12, rootInt, roots, tolSturm, divtol, 3, twinroots, 3, 1e-5,
    "(x-.39)^4*(x-.4)^4*(x-.2)^4", true, 1);

  stat |= vtkTestPolynomials(twin01, 12, rootInt, roots, tolSturm, divtol, 3, twinroots, 3, 1e-5,
    "(x-.39)^4*(x-.4)^4*(x-.2)^4", true, 2);

  double toh06a[] = {1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1., 1.};

  stat |= vtkTestPolynomials(toh06a, 20, rootInt, roots, tolSturm, divtol, 2, 0, 0, 1e-7,
    "1+x+x^2+\\cdots+x^20", false);

  stat |= vtkTestPolynomials(toh06a, 20, rootInt, roots, tolSturm, divtol+3, 2, 0, 0, 1e-7,
    "1+x+x^2+\\cdots+x^20", false, 1);

  stat |= vtkTestPolynomials(toh06a, 20, rootInt, roots, tolSturm, divtol+4, 1, 0, 0, 1e-7,
    "1+x+x^2+\\cdots+x^20", false, 2);

  cout << "Test non-Sequence solvers" << endl;
  // 1. find the roots of a degree 4 polynomial with a 1 double root (1) and 2
  // simple roots (2 and 3) using:
  // 1.a FerrariSolve
  // 1.b SturmBissectionSolve
  double tolLinBairstow = 1.e-12;
  double tolDirectSolvers = VTK_DBL_EPSILON;
  int mult[4];
  double P4[] = { 1., -7., 17., -17., 6. } ;
  vtkPolynomialSolversUnivariate::PrintPolynomial(cout, P4, 4 );

  // 1.a FerrariSolve
  timer->StartTimer();
  int testIntValue = vtkPolynomialSolversUnivariate::FerrariSolve( P4 + 1, roots, mult, tolDirectSolvers );
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
    if ( mult[i] > 1 )
      {
      cout << "(" << mult[i] << ")";
      }
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
  double P4RTS[] = {1.,2.,3.};

  stat |= vtkTestPolynomials(P4, 4, rootInt, roots, tolSturm, divtol, limit-1, 
                P4RTS, 3, 1e-7, "(x-1)^2*(x-2)*(x-3)", true, 3);

  // 2. find the roots of a degree 5 polynomial with LinBairstowSolve
  double P5[] = { 1., -10., 35., -50., 24., 0. } ;
  vtkPolynomialSolversUnivariate::PrintPolynomial(cout, P5, 5 );

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

  // 3. find the roots of a quadratic trinomial with the BisectionSolvers
  double P2[] = { 1., -2., 1. };
  double P2RTS[] = {1.};

  stat |= vtkTestPolynomials(P2, 2, rootInt, roots, tolSturm, divtol, limit-1, 
                P2RTS, 1, 1e-7, "(x-1)^2", true, 3);

  // 4. find the roots of a biquadratic trinomial with SturmBisectionSolve,
  // whose 2 double roots (-4 and 4) are also the bounds of the interval, thus
  // being a limiting case of Sturm's theorem, using:
  // 4.a FerrariSolve
  // 4.b SturmBisectionSolve

  double P4_2[] = { 1., 0., -32., 0., 256. };
  vtkPolynomialSolversUnivariate::PrintPolynomial(cout, P4_2, 4 );

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
  rootInt[0] = -4.;
  rootInt[1] = 4.;
  double P4_2RTS[] = {-4.,4.};
  stat |= vtkTestPolynomials(P4_2, 4, rootInt, roots, tolSturm, divtol, limit-1, 
                P4_2RTS, 2, 1e-7, "(x+4)^2*(x-4)^2 on [-4,4]", true, 3, 3);
  stat |= vtkTestPolynomials(P4_2, 4, rootInt, roots, tolSturm, divtol, limit-1, 
                P4_2RTS, 2, 1e-7, "(x+4)^2*(x-4)^2 on [-4,4)", true, 3, 1);
  stat |= vtkTestPolynomials(P4_2, 4, rootInt, roots, tolSturm, divtol, limit-1, 
                P4_2RTS, 2, 1e-7, "(x+4)^2*(x-4)^2 on (-4,4]", true, 3, 2);
  stat |= vtkTestPolynomials(P4_2, 4, rootInt, roots, tolSturm, divtol, limit-1, 
                P4_2RTS, 2, 1e-7, "(x+4)^2*(x-4)^2 on (-4,4)", false, 3, 0);

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
  double P12RTS[] = {1.,2.,3.};

   stat |= vtkTestPolynomials(P12, 12, rootInt, roots, tolSturm, divtol, limit, 
                P12RTS, 3, 1e-7, "(x-1)^4*(x-2)^4*(x-3)^4 on (0,20)", true, 3, 0);

  // 6. Find the roots of a degree 22 polynomial with SturmBisectionSolve
  rootInt[0] = -10.;
  rootInt[1] = 10.;
  double P22[] = {
    -0.0005, -0.001, 0.05, 0.1, -0.2,
    1., 0., -5.1, 0., 4., 
    -1., .2, 3., 2.2, 2.,
    -7., -.3, 3.8, 14., -16.,
    80., -97.9, 5. };
  double P22RTS[] = {-9.5799184021307155490, -4.1659457357018254697, -1.6909764377051033080, 0.053377023199573159218, 1.1281470811227336526, 1.2804679030668921769, 1.7510169549495913088, 9.9112562522641383112};

   stat |= vtkTestPolynomials(P22, 22, rootInt, roots, tolSturm, divtol, limit, 
                P22RTS, 8, 1e-7, "-0.0005*x^22 -0.001*x^21 +0.05*x^20 +0.1*x^19 -0.2*x^18 +1*x^17 -5.1*x^15 +4*x^13 -1*x^12 +0.2*x^11 +3*x^10 +2.2*x^9 +2*x^8 -7*x^7 -0.3*x^6 +3.8*x^5 +14*x^4 -16*x^3 +80*x^2 -97.9*x +5", false, 3, 0);


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
  double P10RTS[] = {-0.38381365387613186613, 0.38381365386885590851};

   stat |= vtkTestPolynomials(P10, 10, rootInt, roots, tolSturm, divtol, limit, 
                P10RTS, 2, 1e-7, "76*x^10 +95*x^2 -14", false, 3, 0);


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
  double P84RTS[] = {-1.0923995943740010262, 0.92872986290603876114};

  stat |= vtkTestPolynomials(P84, 84, rootInt, roots, tolSturm, divtol, limit, 
                P84RTS, 2, 1e-7, "55*x^84-79*x^72+90*x^71+37*x^44+49*x^21-12", false, 3, 0);


  timer->Delete();
  return stat;
}
