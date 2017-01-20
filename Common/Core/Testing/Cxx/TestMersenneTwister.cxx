/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMersenneTwister.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// This program tests the vtkMersenneTwister class.

#include <cmath>

#include <vtkMath.h>
#include "vtkMersenneTwister.h"
#include "vtkNew.h"
#include "vtkDebugLeaks.h"

#define VTK_SUCCESS 0
#define VTK_FAILURE 1

// Test the first four moments to ensure our random number generator conforms
// to a flat random distribution between 0 and 1.
int MomentCheck(double min, double max, std::size_t nValues)
{
  vtkNew<vtkMersenneTwister> seq;
  // We set the seed to 0 (rather than seeding by time) so that, if the test
  // fails, we can reproduce the failure.
  seq->InitializeSequence(0, 0);

  double n = 0.;
  double mean = 0.;
  double M2 = 0.;
  double M3 = 0.;
  double M4 = 0.;

  for (std::size_t i=0;i<nValues;i++)
  {
    double value = min + (max - min)*seq->GetValue();

    double n1 = n;
    n += 1.;
    double delta = value - mean;
    double delta_n = delta / n;
    double delta_n2 = delta_n * delta_n;
    double term1 = delta * delta_n * n1;
    mean += delta_n;
    M4 += term1*delta_n2*(n*n - 3.*n + 3.) + 6.*delta_n2*M2 - 4.*delta_n*M3;
    M3 += term1*delta_n*(n - 2.) - 3.*delta_n*M2;
    M2 += term1;

    seq->Next();
  }

  double empiricalMean     = mean;
  double empiricalVariance = M2/(nValues - 1.);
  double empiricalSkewness = ((sqrt(static_cast<double>(nValues))*M3) /
                              pow(M2,1.5));
  double empiricalKurtosis = (nValues*M4)/(M2*M2) - 3.;

  double analyticMean     = 1./2.*(min + max);
  double analyticVariance = 1./12.*pow(max-min,2);
  double analyticSkewness = 0.;
  double analyticKurtosis = -6./5.;

  std::cout<<"Mean:     "<<empiricalMean<<" "<<analyticMean<<std::endl;
  std::cout<<"Variance: "<<empiricalVariance<<" "<<analyticVariance<<std::endl;
  std::cout<<"Skewness: "<<empiricalSkewness<<" "<<analyticSkewness<<std::endl;
  std::cout<<"Kurtosis: "<<empiricalKurtosis<<" "<<analyticKurtosis<<std::endl;

#define EPSILON 2.e-3

  if (fabs(empiricalMean - analyticMean) > EPSILON)
  {
    std::cerr<<"Mean deviates from uniform distribution."<<std::endl;
    return VTK_FAILURE;
  }

  if (fabs(empiricalVariance - analyticVariance) > EPSILON)
  {
    std::cerr<<"Variance deviates from uniform distribution."<<std::endl;
    return VTK_FAILURE;
  }

  if (fabs(empiricalSkewness - analyticSkewness) > EPSILON)
  {
    std::cerr<<"Skewness deviates from uniform distribution."<<std::endl;
    return VTK_FAILURE;
  }

  if (fabs(empiricalKurtosis - analyticKurtosis) > EPSILON)
  {
    std::cerr<<"Kurtosis deviates from uniform distribution."<<std::endl;
    return VTK_FAILURE;
  }

#undef EPSILON

  return VTK_SUCCESS;
}

// Construct two instances of vtkMersenneTwister, each with <nThreads>
// independent sequence generators. Extract <nValues> values from each of the
// the sequences, using a different order for each of the two instances. Compare
// the two outputs to ensure that the sequence generators generate the same
// values independent of the order in which sequences values were queried.
int ThreadCheck(std::size_t nThreads, std::size_t nValues)
{
  int retVal = VTK_SUCCESS;

  double** values1 = new double*[nThreads];
  double** values2 = new double*[nThreads];
  for(std::size_t i = 0; i < nThreads; ++i)
  {
    values1[i] = new double[nValues];
    values2[i] = new double[nValues];
  }

  vtkNew<vtkMersenneTwister> seq1, seq2;
  typedef vtkMersenneTwister::SequenceId SequenceId;
  SequenceId* ids1 = new SequenceId[nThreads];
  SequenceId* ids2 = new SequenceId[nThreads];

  for (std::size_t i=0; i< nThreads; i++)
  {
    // For some Windows builds, this is apparently a conversion.
    ids1[i] = seq1->InitializeNewSequence(static_cast<SequenceId>(i));
    ids2[i] = seq2->InitializeNewSequence(static_cast<SequenceId>(i));
  }

  for(std::size_t i = 0; i < nThreads; ++i)
  {
    for(std::size_t j = 0; j < nValues; ++j)
    {
      seq1->Next(ids1[i]);
      values1[i][j] = seq1->GetValue(ids1[i]);
    }
  }

  for(std::size_t j = 0; j < nValues; ++j)
  {
    for(std::size_t i = 0; i < nThreads; ++i)
    {
      seq2->Next(ids2[i]);
      values2[i][j] = seq2->GetValue(ids2[i]);
    }
  }

  for(std::size_t i = 0; i < nThreads; ++i)
  {
    for(std::size_t j = 0; j < nValues; ++j)
    {
      if (fabs(values1[i][j] - values2[i][j]) > VTK_DBL_EPSILON)
      {
        std::cerr<<"Values are not independent across sequence ids."<<std::endl;
        retVal = VTK_FAILURE;
      }
    }
  }

  for(std::size_t i = 0; i < nThreads; ++i)
  {
    delete [] values1[i];
    delete [] values2[i];
  }
  delete [] values1;
  delete [] values2;

  delete [] ids1;
  delete [] ids2;

  return retVal;
}

// Construct an instance of vtkMersenneTwister and initialize two sequences,
// both seeded with the value 0, and an instance that initializes one sequence
// seeded with the value 1. Ensure that the sequence with sequence id = 0 and
// seed = 0 produces the same predetermined values (to test repeatability), and
// ensure that the other two sequences produce different values from the first
// (one because it has a different sequence id, the other because it has a
// different seed).
int ConsistencyCheck()
{
  vtkNew<vtkMersenneTwister> seq;
  vtkMersenneTwister::SequenceId id0 = 0;
  seq->InitializeSequence(id0, 0);
  vtkMersenneTwister::SequenceId id1 = seq->InitializeNewSequence(0);
  vtkNew<vtkMersenneTwister> seq2;
  seq2->InitializeSequence(id0, 1);

  double expectedValues[10] = {0.5862478457291265, 0.1075908798808125,
                               0.712434145798683,  0.6581756278211577,
                               0.6593377378773223, 0.06362405107646187,
                               0.9777108177736147, 0.8852357508063485,
                               0.8330867585347151, 0.183371047990076};

  for (int i=0; i<10; i++)
  {
    seq->Next(id0);
    seq->Next(id1);
    seq2->Next(id0);
    if (fabs(seq->GetValue(id0) - expectedValues[i]) > VTK_DBL_EPSILON)
    {
      std::cerr<<"Sequence seeded with seed 0 has changed."<<std::endl;
      return VTK_FAILURE;
    }
    if (fabs(seq->GetValue(id0) - seq->GetValue(id1)) < VTK_DBL_EPSILON)
    {
      std::cerr<<"Sequence 0 seeded with seed 0 has produced the same value as "
               <<"sequence 1 seeded with seed 0."<<std::endl;
      return VTK_FAILURE;
    }
    if (fabs(seq->GetValue(id0) - seq2->GetValue(id0)) < VTK_DBL_EPSILON)
    {
      std::cerr<<"Sequence 0 seeded with seed 0 has produced the same value as "
               <<"sequence 0 seeded with seed 1."<<std::endl;
      return VTK_FAILURE;
    }
  }

  return VTK_SUCCESS;
}

int TestMersenneTwister(int,char *[])
{
  if (MomentCheck(0.,1.,1.e6) != VTK_SUCCESS)
  {
    return VTK_FAILURE;
  }

  if (ThreadCheck(5,5) != VTK_SUCCESS)
  {
    return VTK_FAILURE;
  }

  if (ConsistencyCheck() != VTK_SUCCESS)
  {
    return VTK_FAILURE;
  }

  return VTK_SUCCESS;
}
