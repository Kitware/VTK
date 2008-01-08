/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolynomialSolvers.h

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
// .NAME vtkPolynomialSolvers - polynomial solvers
// .SECTION Description
// vtkPolynomialSolvers provides solvers for univariate polynomial 
// equations and for multivariate polynomial systems.

#ifndef __vtkPolynomialSolvers_h
#define __vtkPolynomialSolvers_h

#include "vtkObject.h"

#ifndef DBL_EPSILON
#  define VTK_DBL_EPSILON    2.2204460492503131e-16
#else  // DBL_EPSILON
#  define VTK_DBL_EPSILON    DBL_EPSILON
#endif  // DBL_EPSILON

class VTK_COMMON_EXPORT vtkPolynomialSolvers : public vtkObject
{
public:
  static vtkPolynomialSolvers *New();
  vtkTypeRevisionMacro(vtkPolynomialSolvers,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Counts the number of REAL roots of the \a d -th degree polynomial 
  //   P[0] X^d + ... + P[d-1] X + P[d] 
  // in ]\a[0], \a[1]] using Sturm's theorem ( polynomial coefficients are 
  // REAL ) and returns the count.
  // Returns -1 if anything went wrong (such as: polynomial does not have
  // degree d, the interval provided by the other is absurd, etc.).
  static int SturmRootCount( double* P, int d, double* a );

  // Description:
  // Finds all REAL roots (within tolerance \tol) of the \a d -th degree polynomial 
  //   P[0] X^d + ... + P[d-1] X + P[d] 
  // in ]\a[0] ; \a[1]] using Sturm's theorem ( polynomial 
  // coefficients are REAL ) and returns the count \nr. All roots are bracketed
  // in the \nr first ]\upperBnds[i] - tol ; \upperBnds[i]] intervals.
  // Returns -1 if anything went wrong (such as: polynomial does not have
  // degree d, the interval provided by the other is absurd, etc.).
  // Warning: it is the user's responsibility to make sure the \upperBnds 
  // Note that \nr is smaller or equal to the actual number of roots in 
  // ]\a[0] ; \a[1]] since roots within \tol are lumped in the same bracket.
  // array is large enough to contain the maximal number of expected upper bounds.
  static int SturmBisectionSolve( double* P, int d, double* a, double *upperBnds, double tol );

  // Description:
  // Seeks all REAL roots of the \a d -th degree polynomial 
  //   c[0] X^d + ... + c[d-1] X + c[d] = 0
  // equation Lin-Bairstow's method ( polynomial coefficients are REAL ) and 
  // stores the \nr roots found ( multiple roots are multiply stored ) in \a r.
  // \tolerance is the user-defined solver tolerance; this variable may be 
  // relaxed by the iterative solver if needed.
  // Returns \nr.
  // Warning: it is the user's responsibility to make sure the \a r
  // array is large enough to contain the maximal number of expected roots.
  static int LinBairstowSolve( double* c, int d, double* r, double& tolerance );

  // Description:
  // Algebraically extracts REAL roots of the quartic polynomial with 
  // REAL coefficients X^4 + c[0] X^3 + c[1] X^2 + c[2] X + c[3]
  // and stores them (when they exist) and their respective multiplicities
  // in the \a r and \a m arrays, based on Ferrari's method.
  // Some numerical noise can be filtered by the use of a tolerance \tol 
  // instead of equality with 0 (one can use, e.g., VTK_DBL_EPSILON).
  // Returns the number of roots.
  // Warning: it is the user's responsibility to pass a non-negative \tol.
  static int FerrariSolve( double* c, double* r, int* m, double tol );

protected:
  vtkPolynomialSolvers() {};
  ~vtkPolynomialSolvers() {};
  
  static long Seed;
private:
  vtkPolynomialSolvers(const vtkPolynomialSolvers&);  // Not implemented.
  void operator=(const vtkPolynomialSolvers&);  // Not implemented.
};

#endif
