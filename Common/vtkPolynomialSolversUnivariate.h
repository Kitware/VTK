/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolynomialSolversUnivariate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
  Copyright 2007 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: pppebay@sandia.gov,dcthomp@sandia.gov

=========================================================================*/
// .NAME vtkPolynomialSolversUnivariate - polynomial solvers
// .SECTION Description
// vtkPolynomialSolversUnivariate provides solvers for
// univariate polynomial equations with real coefficients.
// The Tartaglia-Cardan and Ferrari solvers work on polynomials of fixed
// degree 3 and 4, respectively.
// The Lin-Bairstow and Sturm solvers work on polynomials of arbitrary
// degree. The Sturm solver is the most robust solver but only reports
// roots within an interval and does not report multiplicities.
// The Lin-Bairstow solver reports multiplicities.
//
// For difficult polynomials, you may wish to use FilterRoots to
// eliminate some of the roots reported by the Sturm solver.
// FilterRoots evaluates the derivatives near each root to
// eliminate cases where a local minimum or maximum is close
// to zero.
//
// .SECTION Thanks
// Thanks to Philippe Pebay, Korben Rusek, and Maurice Rojas for
// implementing these solvers.

#ifndef __vtkPolynomialSolversUnivariate_h
#define __vtkPolynomialSolversUnivariate_h

#include "vtkObject.h"
#include <vtkstd/map>
#include <vtkstd/vector>

#ifndef DBL_EPSILON
#  define VTK_DBL_EPSILON    2.2204460492503131e-16
#else  // DBL_EPSILON
#  define VTK_DBL_EPSILON    DBL_EPSILON
#endif  // DBL_EPSILON

#ifndef DBL_MIN
#  define VTK_DBL_MIN    2.2250738585072014e-308
#else  // DBL_MIN
#  define VTK_DBL_MIN    DBL_MIN
#endif  // DBL_MIN

class VTK_COMMON_EXPORT vtkPolynomialSolversUnivariate : public vtkObject
{
public:
  static vtkPolynomialSolversUnivariate *New();
  vtkTypeRevisionMacro(vtkPolynomialSolversUnivariate,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static ostream& PrintPolynomial( ostream& os, double* P, int degP );

  // Description:
  // Finds all REAL roots (within tolerance \a tol) of the \a d -th degree polynomial 
  //   P[0] X^d + ... + P[d-1] X + P[d] 
  // in ]\a a[0] ; \a a[1]] using Sturm's theorem ( polynomial 
  // coefficients are REAL ) and returns the count \a nr. All roots are bracketed
  // in the \nr first ]\a upperBnds[i] - \a tol ; \a upperBnds[i]] intervals.
  // Returns -1 if anything went wrong (such as: polynomial does not have
  // degree \a d, the interval provided by the other is absurd, etc.).
  //
  // intervalType specifies the search interval as follows:
  // 0 = 00 = ]a,b[
  // 1 = 10 = [a,b[
  // 2 = 01 = ]a,b]
  // 3 = 11 = [a,b]
  // This defaults to 0.
  //
  // The last non-zero item in the Sturm sequence is the gcd of P and P'. The
  // parameter divideGCD specifies whether the program should attempt to divide
  // by the gcd and run again. It works better with polynomials known to have
  // high multiplicities. This defaults to 0.
  //
  // Warning: it is the user's responsibility to make sure the \a upperBnds 
  // array is large enough to contain the maximal number of expected roots.
  // Note that \a nr is smaller or equal to the actual number of roots in 
  // ]\a a[0] ; \a a[1]] since roots within \tol are lumped in the same bracket.
  // array is large enough to contain the maximal number of expected upper bounds.
  static int SturmBisectionSolve( double* P, int d, double* a, double *upperBnds, double tol, int intervalType, bool divideGCD );
  static int SturmBisectionSolve( double* P, int d, double* a, double *upperBnds, double tol, int intervalType );
  static int SturmBisectionSolve( double* P, int d, double* a, double *upperBnds, double tol );

  // Description:
  // This uses the derivative sequence to filter possible roots of a polynomial.
  // If the number of sign changes of the derivative sequence at a root at
  // upperBnds[i] == that at upperBnds[i]  - diameter then the i^th value is 
  // removed from upperBnds. It returns the new number of roots.
  static int FilterRoots(double* P, int d, double *upperBnds, int rootcount, double diameter);

  // Description:
  // Seeks all REAL roots of the \a d -th degree polynomial 
  //   c[0] X^d + ... + c[d-1] X + c[d] = 0
  // equation Lin-Bairstow's method ( polynomial coefficients are REAL ) and 
  // stores the \a nr roots found ( multiple roots are multiply stored ) in \a r.
  // \a tolerance is the user-defined solver tolerance; this variable may be 
  // relaxed by the iterative solver if needed.
  // Returns \a nr.
  // Warning: it is the user's responsibility to make sure the \a r
  // array is large enough to contain the maximal number of expected roots.
  static int LinBairstowSolve( double* c, int d, double* r, double& tolerance );

  // Description:
  // Algebraically extracts REAL roots of the quartic polynomial with 
  // REAL coefficients X^4 + c[0] X^3 + c[1] X^2 + c[2] X + c[3]
  // and stores them (when they exist) and their respective multiplicities
  // in the \a r and \a m arrays, based on Ferrari's method.
  // Some numerical noise can be filtered by the use of a tolerance \a tol 
  // instead of equality with 0 (one can use, e.g., VTK_DBL_EPSILON).
  // Returns the number of roots.
  // Warning: it is the user's responsibility to pass a non-negative \a tol.
  static int FerrariSolve( double* c, double* r, int* m, double tol );

  // Description:
  // Algebraically extracts REAL roots of the cubic polynomial with 
  // REAL coefficients X^3 + c[0] X^2 + c[1] X + c[2]
  // and stores them (when they exist) and their respective multiplicities
  // in the \a r and \a m arrays.
  // Some numerical noise can be filtered by the use of a tolerance \a tol 
  // instead of equality with 0 (one can use, e.g., VTK_DBL_EPSILON).
  // The main differences with SolveCubic are that (1) the polynomial must have
  // unit leading coefficient, (2) complex roots are discarded upfront, 
  // (3) non-simple roots are stored only once, along with their respective
  // multiplicities, and (4) some numerical noise is filtered by the use of 
  // relative tolerance instead of equality with 0.
  // Returns the number of roots.
  // <i> In memoriam </i> Niccolo Tartaglia (1500 - 1559), unfairly forgotten.
  static int TartagliaCardanSolve( double* c, double* r, int* m, double tol );

  // Description:
  // Set/get the tolerance used when performing polynomial Euclidean division
  // to find polynomial roots. This tolerance is used to decide whether the
  // leading coefficient(s) of a polynomial remainder are close enough to
  // zero to be neglected.
  static void SetDivisionTolerance( double tol );
  static double GetDivisionTolerance();

protected:
  vtkPolynomialSolversUnivariate() {};
  ~vtkPolynomialSolversUnivariate() {};
  
  static double DivisionTolerance;

private:
  vtkPolynomialSolversUnivariate(const vtkPolynomialSolversUnivariate&);  // Not implemented.
  void operator=(const vtkPolynomialSolversUnivariate&);  // Not implemented.
};

#endif
