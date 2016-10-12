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
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

  Contact: pppebay@sandia.gov,dcthomp@sandia.gov

=========================================================================*/
/**
 * @class   vtkPolynomialSolversUnivariate
 * @brief   polynomial solvers
 *
 * vtkPolynomialSolversUnivariate provides solvers for
 * univariate polynomial equations with real coefficients.
 * The Tartaglia-Cardan and Ferrari solvers work on polynomials of fixed
 * degree 3 and 4, respectively.
 * The Lin-Bairstow and Sturm solvers work on polynomials of arbitrary
 * degree. The Sturm solver is the most robust solver but only reports
 * roots within an interval and does not report multiplicities.
 * The Lin-Bairstow solver reports multiplicities.
 *
 * For difficult polynomials, you may wish to use FilterRoots to
 * eliminate some of the roots reported by the Sturm solver.
 * FilterRoots evaluates the derivatives near each root to
 * eliminate cases where a local minimum or maximum is close
 * to zero.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay, Korben Rusek, David Thompson, and Maurice Rojas
 * for implementing these solvers.
*/

#ifndef vtkPolynomialSolversUnivariate_h
#define vtkPolynomialSolversUnivariate_h

#include "vtkCommonMathModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONMATH_EXPORT vtkPolynomialSolversUnivariate : public vtkObject
{
public:
  static vtkPolynomialSolversUnivariate *New();
  vtkTypeMacro(vtkPolynomialSolversUnivariate,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static ostream& PrintPolynomial( ostream& os, double* P, int degP );

  //@{
  /**
   * Finds all REAL roots (within tolerance \a tol) of the \a d -th degree polynomial
   * \f[
   * P[0] X^d + ... + P[d-1] X + P[d]
   * \f]
   * in ]\a a[0] ; \a a[1]] using the Habicht sequence (polynomial
   * coefficients are REAL) and returns the count \a nr. All roots are bracketed
   * in the \nr first ]\a upperBnds[i] - \a tol ; \a upperBnds[i]] intervals.
   * Returns -1 if anything went wrong (such as: polynomial does not have
   * degree \a d, the interval provided by the other is absurd, etc.).

   * \a intervalType specifies the search interval as follows:
   * 0 = 00 = ]a,b[
   * 1 = 10 = [a,b[
   * 2 = 01 = ]a,b]
   * 3 = 11 = [a,b]
   * This defaults to 0.

   * The last non-zero item in the Habicht sequence is the gcd of P and P'. The
   * parameter divideGCD specifies whether the program should attempt to divide
   * by the gcd and run again. It works better with polynomials known to have
   * high multiplicities. When divideGCD != 0 then it attempts to divide by the
   * GCD, if applicable. This defaults to 0.

   * Compared to the Sturm solver the Habicht solver is slower,
   * although both are O(d^2). The Habicht solver has the added benefit
   * that it has a built in mechanism to keep the leading coefficients of the
   * result from polynomial division bounded above and below in absolute value.
   * This will tend to keep the coefficients of the polynomials in the sequence
   * from zeroing out prematurely or becoming infinite.

   * Constructing the Habicht sequence is O(d^2) in both time and space.

   * Warning: it is the user's responsibility to make sure the \a upperBnds
   * array is large enough to contain the maximal number of expected roots.
   * Note that \a nr is smaller or equal to the actual number of roots in
   * ]\a a[0] ; \a a[1]] since roots within \tol are lumped in the same bracket.
   * array is large enough to contain the maximal number of expected upper bounds.
   */
  static int HabichtBisectionSolve(
    double* P, int d, double* a, double* upperBnds, double tol );
  static int HabichtBisectionSolve(
    double* P, int d, double* a, double* upperBnds, double tol,
    int intervalType );
  static int HabichtBisectionSolve(
    double* P, int d, double* a, double* upperBnds, double tol,
    int intervalType, bool divideGCD );
  //@}

  //@{
  /**
   * Finds all REAL roots (within tolerance \a tol) of the \a d -th degree polynomial
   * P[0] X^d + ... + P[d-1] X + P[d]
   * in ]\a a[0] ; \a a[1]] using Sturm's theorem ( polynomial
   * coefficients are REAL ) and returns the count \a nr. All roots are bracketed
   * in the \nr first ]\a upperBnds[i] - \a tol ; \a upperBnds[i]] intervals.
   * Returns -1 if anything went wrong (such as: polynomial does not have
   * degree \a d, the interval provided by the other is absurd, etc.).

   * intervalType specifies the search interval as follows:
   * 0 = 00 = ]a,b[
   * 1 = 10 = [a,b[
   * 2 = 01 = ]a,b]
   * 3 = 11 = [a,b]
   * This defaults to 0.

   * The last non-zero item in the Sturm sequence is the gcd of P and P'. The
   * parameter divideGCD specifies whether the program should attempt to divide
   * by the gcd and run again. It works better with polynomials known to have
   * high multiplicities. When divideGCD != 0 then it attempts to divide by the
   * GCD, if applicable. This defaults to 0.

   * Constructing the Sturm sequence is O(d^2) in both time and space.

   * Warning: it is the user's responsibility to make sure the \a upperBnds
   * array is large enough to contain the maximal number of expected roots.
   * Note that \a nr is smaller or equal to the actual number of roots in
   * ]\a a[0] ; \a a[1]] since roots within \tol are lumped in the same bracket.
   * array is large enough to contain the maximal number of expected upper bounds.
   */
  static int SturmBisectionSolve(
    double* P, int d, double* a, double* upperBnds, double tol );
  static int SturmBisectionSolve(
    double* P, int d, double* a, double* upperBnds, double tol,
    int intervalType );
  static int SturmBisectionSolve(
    double* P, int d, double* a, double* upperBnds, double tol,
    int intervalType, bool divideGCD );
  //@}

  /**
   * This uses the derivative sequence to filter possible roots of a polynomial.
   * First it sorts the roots and removes any duplicates.
   * If the number of sign changes of the derivative sequence at a root at
   * upperBnds[i] == that at upperBnds[i]  - diameter then the i^th value is
   * removed from upperBnds. It returns the new number of roots.
   */
  static int FilterRoots(
    double* P, int d, double *upperBnds, int rootcount, double diameter );

  /**
   * Seeks all REAL roots of the \a d -th degree polynomial
   * c[0] X^d + ... + c[d-1] X + c[d] = 0
   * equation Lin-Bairstow's method ( polynomial coefficients are REAL ) and
   * stores the \a nr roots found ( multiple roots are multiply stored ) in \a r.
   * \a tolerance is the user-defined solver tolerance; this variable may be
   * relaxed by the iterative solver if needed.
   * Returns \a nr.
   * Warning: it is the user's responsibility to make sure the \a r
   * array is large enough to contain the maximal number of expected roots.
   */
  static int LinBairstowSolve( double* c, int d, double* r, double& tolerance );

  /**
   * Algebraically extracts REAL roots of the quartic polynomial with
   * REAL coefficients X^4 + c[0] X^3 + c[1] X^2 + c[2] X + c[3]
   * and stores them (when they exist) and their respective multiplicities
   * in the \a r and \a m arrays, based on Ferrari's method.
   * Some numerical noise can be filtered by the use of a tolerance \a tol
   * instead of equality with 0 (one can use, e.g., VTK_DBL_EPSILON).
   * Returns the number of roots.
   * Warning: it is the user's responsibility to pass a non-negative \a tol.
   */
  static int FerrariSolve( double* c, double* r, int* m, double tol );

  /**
   * Algebraically extracts REAL roots of the cubic polynomial with
   * REAL coefficients X^3 + c[0] X^2 + c[1] X + c[2]
   * and stores them (when they exist) and their respective multiplicities
   * in the \a r and \a m arrays.
   * Some numerical noise can be filtered by the use of a tolerance \a tol
   * instead of equality with 0 (one can use, e.g., VTK_DBL_EPSILON).
   * The main differences with SolveCubic are that (1) the polynomial must have
   * unit leading coefficient, (2) complex roots are discarded upfront,
   * (3) non-simple roots are stored only once, along with their respective
   * multiplicities, and (4) some numerical noise is filtered by the use of
   * relative tolerance instead of equality with 0.
   * Returns the number of roots.
   * <i> In memoriam </i> Niccolo Tartaglia (1500 - 1559), unfairly forgotten.
   */
  static int TartagliaCardanSolve( double* c, double* r, int* m, double tol );

  /**
   * Solves a cubic equation c0*t^3 + c1*t^2 + c2*t + c3 = 0 when c0, c1, c2,
   * and c3 are REAL.  Solution is motivated by Numerical Recipes In C 2nd
   * Ed.  Return array contains number of (real) roots (counting multiple
   * roots as one) followed by roots themselves. The value in roots[4] is a
   * integer giving further information about the roots (see return codes for
   * int SolveCubic() ).
   */
  static double* SolveCubic(double c0, double c1, double c2, double c3);

  /**
   * Solves a quadratic equation c1*t^2 + c2*t + c3 = 0 when c1, c2, and c3
   * are REAL.  Solution is motivated by Numerical Recipes In C 2nd Ed.
   * Return array contains number of (real) roots (counting multiple roots as
   * one) followed by roots themselves. Note that roots[3] contains a return
   * code further describing solution - see documentation for SolveCubic()
   * for meaning of return codes.
   */
  static double* SolveQuadratic(double c0, double c1, double c2);

  /**
   * Solves a linear equation c2*t  + c3 = 0 when c2 and c3 are REAL.
   * Solution is motivated by Numerical Recipes In C 2nd Ed.
   * Return array contains number of roots followed by roots themselves.
   */
  static double* SolveLinear(double c0, double c1);

  /**
   * Solves a cubic equation when c0, c1, c2, And c3 Are REAL.  Solution
   * is motivated by Numerical Recipes In C 2nd Ed.  Roots and number of
   * real roots are stored in user provided variables r1, r2, r3, and
   * num_roots. Note that the function can return the following integer
   * values describing the roots: (0)-no solution; (-1)-infinite number
   * of solutions; (1)-one distinct real root of multiplicity 3 (stored
   * in r1); (2)-two distinct real roots, one of multiplicity 2 (stored
   * in r1 & r2); (3)-three distinct real roots; (-2)-quadratic equation
   * with complex conjugate solution (real part of root returned in r1,
   * imaginary in r2); (-3)-one real root and a complex conjugate pair
   * (real root in r1 and real part of pair in r2 and imaginary in r3).
   */
  static int SolveCubic(double c0, double c1, double c2, double c3,
                        double *r1, double *r2, double *r3, int *num_roots);

  /**
   * Solves a quadratic equation c1*t^2  + c2*t  + c3 = 0 when
   * c1, c2, and c3 are REAL.
   * Solution is motivated by Numerical Recipes In C 2nd Ed.
   * Roots and number of roots are stored in user provided variables
   * r1, r2, num_roots
   */
  static int SolveQuadratic(double c0, double c1, double c2,
                            double *r1, double *r2, int *num_roots);

  /**
   * Algebraically extracts REAL roots of the quadratic polynomial with
   * REAL coefficients c[0] X^2 + c[1] X + c[2]
   * and stores them (when they exist) and their respective multiplicities
   * in the \a r and \a m arrays.
   * Returns either the number of roots, or -1 if ininite number of roots.
   */
  static int SolveQuadratic( double* c, double* r, int* m );

  /**
   * Solves a linear equation c2*t + c3 = 0 when c2 and c3 are REAL.
   * Solution is motivated by Numerical Recipes In C 2nd Ed.
   * Root and number of (real) roots are stored in user provided variables
   * r2 and num_roots.
   */
  static int SolveLinear(double c0, double c1, double *r1, int *num_roots);

  //@{
  /**
   * Set/get the tolerance used when performing polynomial Euclidean division
   * to find polynomial roots. This tolerance is used to decide whether the
   * coefficient(s) of a polynomial remainder are close enough to
   * zero to be neglected.
   */
  static void SetDivisionTolerance( double tol );
  static double GetDivisionTolerance();
  //@}

protected:
  vtkPolynomialSolversUnivariate() {}
  ~vtkPolynomialSolversUnivariate() VTK_OVERRIDE {}

  static double DivisionTolerance;

private:
  vtkPolynomialSolversUnivariate(const vtkPolynomialSolversUnivariate&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolynomialSolversUnivariate&) VTK_DELETE_FUNCTION;
};

#endif
