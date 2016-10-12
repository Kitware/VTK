/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMersenneTwister.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

/* Many thanks to M. Matsumoto, T. Nishimura and M. Saito for the       */
/* implementation of their algorithm, the Mersenne Twister, taken from  */
/* http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/DC/dc.html           */

/* Dynamic Creation (DC) of Mersenne Twister generators   */
/*                                                        */
/* Reference:                                             */
/* Makoto Matsumoto and Takuji Nishimura,                 */
/* "Dynamic Creation of Pseudorandom Number Generators",  */
/* Monte Carlo and Quasi-Monte Carlo Methods 1998,        */
/* Springer, 2000, pp 56--69.                             */

#include "vtkMersenneTwister_Private.cxx"

#include <map>

namespace
{
class MersenneTwister
{
public:
 typedef std::map<uint32_t, mt_struct*> mt_parameter_map;
 typedef mt_parameter_map::value_type mt_parameter;
 typedef mt_parameter_map::iterator mt_parameter_it;

  ~MersenneTwister()
  {
     // Free the initialized MT states.
    for (mt_parameter_it it = this->Parameters.begin();
         it != this->Parameters.end(); ++it)
    {
      free_mt_struct(it->second);
    }
  }

  void InitializeSequence(uint32_t key, uint32_t seed, int periodExp=521)
  {
   mt_parameter_it it = this->Parameters.find(key);
   if (it != this->Parameters.end())
   {
     // If a sequence already exists with this id, free its contents...
     free_mt_struct(it->second);
   }
   else
   {
     //...otherwise, add a key/value pair for this sequence id
     mt_parameter parameter(key, static_cast<mt_struct*>(0));
     it = this->Parameters.insert(parameter).first;
   }
   // Instantiate the sequence.
   it->second = get_mt_parameter_id_st(32, periodExp, key, seed);
   sgenrand_mt(seed, it->second);
  }

 uint32_t InitializeNewSequence(uint32_t seed, int periodExp=521)
 {
   // Identify an id that has not yet been used...
   uint32_t key = static_cast<uint32_t>(this->Parameters.size());
   while (this->Parameters.find(key) != this->Parameters.end())
   {
     ++key;
   }

    //...and use it to instantiate a sequence.
    mt_parameter parameter(key, get_mt_parameter_id_st(32,periodExp,key,seed));
    sgenrand_mt(seed, parameter.second);
    // Here and throughout, we use insert() with a hint to mitigate the cost of
    // using a map to associate keys with MT states (the upshot is we get to use
    // whatever ids we want to grab MT states).
    this->Parameters.insert(
      (this->Parameters.begin() == this->Parameters.end() ?
       this->Parameters.begin() : --this->Parameters.end()), parameter);
    return key;
 }

  uint32_t Random32(uint32_t sequenceId)
  {
    mt_parameter_it it = this->Parameters.find(sequenceId);
    if (it == this->Parameters.end())
    {
      mt_parameter parameter(sequenceId,
                             get_mt_parameter_id_st(32, 521, sequenceId, 0));
      sgenrand_mt(0, parameter.second);
      it = this->Parameters.insert(--it, parameter);
    }
    return genrand_mt(it->second);
  }

  vtkTypeUInt64 Random64(uint32_t sequenceId)
  {
    vtkTypeUInt64 value = this->Random32(sequenceId);
    return (value << 32) + this->Random32(sequenceId);
  }

protected:
  mt_parameter_map Parameters;
};

static const int NMersenneExponents = 15;
static const int MersenneExponents[NMersenneExponents] = {521,   607,   1279,
                                                          2203,  2281,  3217,
                                                          4253,  4423,  9689,
                                                          9941,  11213, 19937,
                                                          21701, 23209, 44497};
static const int* MersenneExponentsEnd = MersenneExponents + NMersenneExponents;
}

#include <algorithm>

#include "vtkMersenneTwister.h"
#include "vtkObjectFactory.h"

class vtkMersenneTwisterInternals : public MersenneTwister
{
public:
  typedef std::map<uint32_t, double> ValueMap;
  typedef ValueMap::iterator ValueIt;
  typedef ValueMap::value_type Value;

  ValueMap Values;
};

vtkStandardNewMacro(vtkMersenneTwister);

// ----------------------------------------------------------------------------
vtkMersenneTwister::vtkMersenneTwister()
{
  this->Internal = new vtkMersenneTwisterInternals();
}

// ----------------------------------------------------------------------------
vtkMersenneTwister::~vtkMersenneTwister()
{
  delete this->Internal;
}

// ----------------------------------------------------------------------------
void vtkMersenneTwister::InitializeSequence(vtkMersenneTwister::SequenceId id,
                                            vtkTypeUInt32 seed, int periodExp)
{
  if (std::find(MersenneExponents, MersenneExponentsEnd, periodExp) ==
      MersenneExponentsEnd)
  {
    periodExp = MersenneExponents[periodExp%15];
  }

  if (this->Internal->Values.insert(
        vtkMersenneTwisterInternals::Value(id, 0.)).second == false)
  {
    vtkWarningMacro(<< "Initializing process "<<id<<" which is already "
                    << "initialized. This may break sequence encapsulation.");
  }
  this->Internal->InitializeSequence(id, seed, periodExp);
}

// ----------------------------------------------------------------------------
vtkMersenneTwister::SequenceId
vtkMersenneTwister::InitializeNewSequence(vtkTypeUInt32 seed, int periodExp)
{
  if (std::find(MersenneExponents, MersenneExponentsEnd, periodExp) ==
      MersenneExponentsEnd)
  {
    periodExp = MersenneExponents[periodExp%15];
  }

  SequenceId id = this->Internal->InitializeNewSequence(seed, periodExp);
  this->Internal->Values.insert(
    (this->Internal->Values.begin() == this->Internal->Values.end() ?
     this->Internal->Values.begin() : --this->Internal->Values.end()),
    vtkMersenneTwisterInternals::Value(id, 0.));
  return id;
}

// ----------------------------------------------------------------------------
double vtkMersenneTwister::GetValue(vtkMersenneTwister::SequenceId id)
{
  vtkMersenneTwisterInternals::ValueIt it = this->Internal->Values.find(id);
  if (it == this->Internal->Values.end())
  {
    this->Next(id);
  }

  return this->Internal->Values.find(id)->second;
}

// ----------------------------------------------------------------------------
void vtkMersenneTwister::Next(vtkMersenneTwister::SequenceId id)
{
  static const double norm =
    1./static_cast<double>(std::numeric_limits<vtkTypeUInt64>::max());

  vtkMersenneTwisterInternals::ValueIt it = this->Internal->Values.find(id);
  if (it == this->Internal->Values.end())
  {
    vtkWarningMacro(<< "Using an ininitialized vtkMersenneTwister process. "
                    << "Initializing process "<<id<<" with default values.");
    vtkMersenneTwisterInternals::Value value(id, 0.);
    it = this->Internal->Values.insert(
      (this->Internal->Values.begin() == this->Internal->Values.end() ?
       this->Internal->Values.begin() : --this->Internal->Values.end()),
      value);
    this->Internal->InitializeSequence(id, 0);
  }
  it->second = this->Internal->Random64(id)*norm;
}

// ----------------------------------------------------------------------------
void vtkMersenneTwister::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
