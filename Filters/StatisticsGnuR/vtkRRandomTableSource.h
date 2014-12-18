/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRRandomTableSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkRRandomTableSource - Generates vtkTables with columns of random numbers using Gnu R.
//
// .SECTION Description
//
// Note - An installation of Gnu R is required to build and use this class.
//
// vtkRRandomTableSource uses the Gnu R math C language API for statistical distributions to generate
// vtkTables with columns of random numbers selected from a chosen distribution. The available
// distributions are as follows:
//
//   Normal, Cauchy, F, Student T, Poisson, Chi-Square, Exponential, Binomial, Beta, Geometric,
//   Uniform, Gamma, Log-Normal, Logistic, Hyper-Geometric, Weibull, Negative Binomial, Wilcox
//
// The default output for the class is a table of one column with ten rows of random numbers chosen
// from a Normal distribution of mean 0.0 and standard deviation of 1.0.
//
// Use SetNumberOfRows() to set the number of rows (random numbers) in the output table.
//
// See comments for SetStatisticalDistributionForColumn() to set the distribution output for a particular
// column or all columns in the output table.
//
// .SECTION See Also
// vtkRInterface
//
// .SECTION Thanks
// Developed by Thomas J. Otahal (tjotaha@sandia.gov) at Sandia National Laboratories.
//

#ifndef vtkRRandomTableSource_h
#define vtkRRandomTableSource_h

#include "vtkFiltersStatisticsGnuRModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class vtkRrtsimplementation;

class VTKFILTERSSTATISTICSGNUR_EXPORT vtkRRandomTableSource : public vtkTableAlgorithm
{

public:

  static vtkRRandomTableSource* New();
  vtkTypeMacro(vtkRRandomTableSource,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set and Get the number of rows in the output table (random numbers).
  void SetNumberOfRows(int nrows);
  int GetNumberOfRows();

  // Description:
  // Returns number of columns in the output table.
  int GetNumberOfColumns();

  // Description:
  // Clears table output to zero output columns.  Number of table rows
  // is unchanged.
  void ClearTableOutput();

  // Description:
  // Set the random seed used by Gnu R to generate output.  The default is to use
  // the random seed provided by Gnu R based on the current time.
  void SetRandGenSeed(const int seed);

//BTX
  // Description:
  // Available statistical distribution output types.  Depending on the distribution type, up to three
  // parameters (param1, param2, param3) must be specified when using SetStatisticalDistributionForColumn().
  typedef enum
    {

    WILCOXONRANKSUM = 0,    // Wilcoxon rank sum
                            // param1 - number of observations in the first sample
                            // param2 - number of observations in the second sample
                            // param3 - not used

    WILCOXONSIGNEDRANK = 1, // Wilcoxon signed rank
                            // param1 - number of observations in the sample
                            // param2 - not used
                            // param3 - not used

    LOGISTIC = 2,    // Logistic
                     // param1 - location parameter (usually 0)
                     // param2 - scale parameter (usually 1)
                     // param3 - not used

    WEIBULL = 3,     // Weibull
                     // param1 - shape parameter
                     // param2 - scale parameter (usually 1)
                     // param3 - not used

    POISSON = 4,     // Poisson
                     // param1 - lambda mean
                     // param2 - not used
                     // param3 - not used

    NEGBINOMIAL = 5, // Negative Binomial
                     // param1 - Dispersion parameter, or number of successful trials
                     // param2 - Probability of success on each trial
                     // param3 - not used

    HYPERGEOM = 6, // Hyper Geometric
                   // param1 - number of white balls in the urn
                   // param2 - number of black balls in the urn
                   // param3 - number of balls drawn from the urn

    GEOM = 7,      // Geometric
                   // param1 - rate parameter
                   // param2 - not used
                   // param3 - not used

    EXP = 8,       // Exponential
                   // param1 - rate parameter
                   // param2 - not used
                   // param3 - not used

    CAUCHY = 9,    // Cauchy
                   // param1 - location parameter (usually 0)
                   // param2 - scale parameter (usually 1)
                   // param3 - not used

    T = 10,         // Student T
                    // param1 - degrees of freedom
                    // param2 - not used
                    // param3 - not used

    F = 11,         // F
                    // param1 - degrees of freedom one
                    // param2 - degrees of freedom two
                    // param3 - not used

    LOGNORMAL = 12, // Log-normal
                    // param1 - log mean
                    // param2 - log standard deviation
                    // param3 - not used

    GAMMA = 13,     // Gamma
                    // param1 - shape parameter
                    // param2 - scale parameter
                    // param3 - not used

    UNIF = 14,      // Uniform
                    // param1 - distribution lower limit
                    // param2 - distribution upper limit
                    // param3 - not used

    BETA = 15,      // Beta
                    // param1 - shape parameter one.
                    // param2 - shape parameter two.
                    // param3 - not used

    BINOMIAL = 16,  // Binomial
                    // param1 - number of trials
                    // param2 - probability of success on each trial
                    // param3 - not used

    NORMAL = 17,    // Normal (Gaussian)
                    // param1 - mean
                    // param2 - standard deviation
                    // param3 - not used

    CHISQUARE = 18, // Chi-square
                    // param1 - degrees of freedom
                    // param2 - not used
                    // param3 - not used

  } StatDistType;

  // Description:
  // Set the statistical distribution to generate random numbers for a particular column or all
  // columns in the output table.  Use the above documented distribution types, for example
  // use vtkRRandomTableSource::Normal for a Normal distribution.  Set unused parameter values to 0.0.
  // For example, a Normal distribution uses only param1 and param2 as the mean and the standard deviation
  // respectively.  Set param3 to 0.0.
  // If column_index equals the current number of columns in the output table, a new column will be
  // added to the output table and initialized with the input distribution parameters.
  void SetStatisticalDistributionForColumn(vtkRRandomTableSource::StatDistType t,
                                           double param1,
                                           double param2,
                                           double param3,
                                           const char* ColumnName,
                                           int column_index);
//ETX

  // Description:
  // Python wrapped version of above method.  Use integer equivalent of StatDistType.
  void SetStatisticalDistributionForColumn(int StatDistType,
                                           double param1,
                                           double param2,
                                           double param3,
                                           const char* ColumnName,
                                           int column_index);


protected:
  vtkRRandomTableSource();
  ~vtkRRandomTableSource();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkRRandomTableSource(const vtkRRandomTableSource&); // Not implemented
  void operator=(const vtkRRandomTableSource&);   // Not implemented

  int NumberOfRows;

  vtkRrtsimplementation *impl;

};

#endif

