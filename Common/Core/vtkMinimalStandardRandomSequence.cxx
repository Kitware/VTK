// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMinimalStandardRandomSequence.h"

#include "vtkObjectFactory.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMinimalStandardRandomSequence);

const int VTK_K_A = 16807;
const int VTK_K_M = 2147483647; // Mersenne prime 2^(31)-1
const int VTK_K_Q = 127773;     // M/A
const int VTK_K_R = 2836;       // M%A

//------------------------------------------------------------------------------
vtkMinimalStandardRandomSequence::vtkMinimalStandardRandomSequence()
{
  this->Seed = 1;
}

//------------------------------------------------------------------------------
vtkMinimalStandardRandomSequence::~vtkMinimalStandardRandomSequence() = default;

//------------------------------------------------------------------------------
void vtkMinimalStandardRandomSequence::SetSeedOnly(int value)
{
  this->Seed = value;

  // fit the seed to the valid range [1,2147483646]
  if (this->Seed < 1)
  {
    this->Seed += 2147483646;
  }
  else
  {
    if (this->Seed == 2147483647)
    {
      this->Seed = 1;
    }
  }
}

//------------------------------------------------------------------------------
void vtkMinimalStandardRandomSequence::SetSeed(int value)
{
  this->SetSeedOnly(value);

  // the first random number after setting the seed is proportional to the
  // seed value. To help solve this, call Next() a few times.
  // This doesn't ruin the repeatability of Next().
  this->Next();
  this->Next();
  this->Next();
}

//------------------------------------------------------------------------------
int vtkMinimalStandardRandomSequence::GetSeed()
{
  return this->Seed;
}

//------------------------------------------------------------------------------
double vtkMinimalStandardRandomSequence::GetValue()
{
  double result = static_cast<double>(this->Seed) / VTK_K_M;

  assert("post: unit_range" && result >= 0.0 && result <= 1.0);
  return result;
}

//------------------------------------------------------------------------------
void vtkMinimalStandardRandomSequence::Next()
{
  int hi = this->Seed / VTK_K_Q;
  int lo = this->Seed % VTK_K_Q;
  this->Seed = VTK_K_A * lo - VTK_K_R * hi;
  if (this->Seed <= 0)
  {
    this->Seed += VTK_K_M;
  }
}

//------------------------------------------------------------------------------
double vtkMinimalStandardRandomSequence::GetRangeValue(double rangeMin, double rangeMax)
{
  double result;
  if (rangeMin == rangeMax)
  {
    result = rangeMin;
  }
  else
  {
    result = rangeMin + this->GetValue() * (rangeMax - rangeMin);
  }
  assert("post: valid_result" &&
    ((rangeMin <= rangeMax && result >= rangeMin && result <= rangeMax) ||
      (rangeMax <= rangeMin && result >= rangeMax && result <= rangeMin)));
  return result;
}

//------------------------------------------------------------------------------
double vtkMinimalStandardRandomSequence::GetNextRangeValue(double rangeMin, double rangeMax)
{
  this->Next();
  return this->GetRangeValue(rangeMin, rangeMax);
}

//------------------------------------------------------------------------------
void vtkMinimalStandardRandomSequence::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
