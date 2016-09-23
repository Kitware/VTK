/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricRandomHills.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricRandomHills.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkDoubleArray.h"
#include "vtkMinimalStandardRandomSequence.h"

#include <ctime>

vtkStandardNewMacro(vtkParametricRandomHills);

//----------------------------------------------------------------------------
vtkParametricRandomHills::vtkParametricRandomHills() :
  NumberOfHills(30)
  , HillXVariance(2.5)
  , HillYVariance(2.5)
  , HillAmplitude(2)
  , RandomSeed(1)
  , XVarianceScaleFactor(1.0/3.0)
  , YVarianceScaleFactor(1.0/3.0)
  , AmplitudeScaleFactor(1.0/3.0)
  , AllowRandomGeneration(1)
{
  // Preset triangulation parameters
  this->MinimumU = -10;
  this->MinimumV = -10;
  this->MaximumU = 10;
  this->MaximumV = 10;

  this->JoinU = 0;
  this->JoinV = 0;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 1;
  this->DerivativesAvailable = 0;

  this->hillData = vtkDoubleArray::New();

  this->previousNumberOfHills = 0;
  this->previousHillXVariance = 0;
  this->previousHillYVariance = 0;
  this->previousHillAmplitude = 0;
  this->previousRandomSeed = 0;
  this->previousXVarianceScaleFactor = 0;
  this->previousYVarianceScaleFactor = 0;
  this->previousAmplitudeScaleFactor = 0;
  this->previousAllowRandomGeneration = 0;

  this->randomSequenceGenerator = vtkMinimalStandardRandomSequence::New();
  // Initialise the sequence.
  this->randomSequenceGenerator->SetSeed(this->RandomSeed);
}

//----------------------------------------------------------------------------
vtkParametricRandomHills::~vtkParametricRandomHills()
{
  this->hillData->Delete();
  this->randomSequenceGenerator->Delete();
}

//----------------------------------------------------------------------------
void vtkParametricRandomHills::InitRNG ( int randomSeed )
{
  (randomSeed < 0) ?
    this->randomSequenceGenerator->SetSeed(static_cast<int>(time(NULL))):
    this->randomSequenceGenerator->SetSeed(randomSeed);
}

//----------------------------------------------------------------------------
double vtkParametricRandomHills::Rand ( void )
{
  double x = this->randomSequenceGenerator->GetValue();
  this->randomSequenceGenerator->Next();
  return x;
}

//----------------------------------------------------------------------------
void vtkParametricRandomHills::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  // If parameters have changed then regenerate the hills.
  if (this->ParametersChanged())
  {
    this->MakeTheHillData();
  }

  double u = uvw[0];
  double v = uvw[1];
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  // Zero out the point and derivatives.
  for ( int i = 0; i < 3; ++i )
    Pt[i] = Du[i] = Dv[i] = 0;

  // The point
  // The height of the surface is made up from
  // the contributions from all the Hills.
  Pt[0] = u;
  Pt[1] = this->MaximumV - v; // Texturing is oriented OK if we do this.
  for ( int j = 0; j < NumberOfHills; ++j )
  {
    double hillTuple[5]; // 0: mX, 1: mY, 2: VarX, 3: VarY, 4: Amplitude
    this->hillData->GetTuple(j,hillTuple);
    double x = (u - hillTuple[0])/hillTuple[2];
    double y = (v - hillTuple[1])/hillTuple[3];
    Pt[2] += hillTuple[4] * exp( -(x*x+y*y) / 2.0 );
  }
}

//----------------------------------------------------------------------------
double vtkParametricRandomHills::EvaluateScalar(double* vtkNotUsed(uv[3]),
                                                double* vtkNotUsed(Pt[3]),
                                                double* vtkNotUsed(Duv[9]))
{
  return 0;
}

void vtkParametricRandomHills::MakeTheHillData( void )
{
  this->hillData->Initialize();
  this->hillData->SetNumberOfComponents(5);
  this->hillData->SetNumberOfTuples(NumberOfHills);

  double dU = MaximumU - MinimumU;
  double dV = MaximumV - MinimumV;
  double hillTuple[5]; // 0: mX, 1: mY, 2: VarX, 3: VarY, 4: Amplitude
  // Generate the centers of the Hills, standard deviations and amplitudes.
  if ( AllowRandomGeneration != 0 )
  {
    InitRNG(this->RandomSeed);
    for (int i = 0; i < this->NumberOfHills; ++i)
    {
      hillTuple[0] = MinimumU + Rand() * dU;
      hillTuple[1] = MinimumV + Rand() * dV;
      hillTuple[2] = this->HillXVariance *
                    (Rand() + this->XVarianceScaleFactor);
      hillTuple[3] = this->HillYVariance *
                    (Rand() + this->YVarianceScaleFactor);
      hillTuple[4] = this->HillAmplitude *
                    (Rand() + this->AmplitudeScaleFactor);
      this->hillData->SetTuple(i, hillTuple);
    }
  }
  else
  {
    // Here the generation is nonrandom.
    // We put hills in a regular grid over the whole surface.
    double gridMax = sqrt(static_cast<double>(this->NumberOfHills));
    int counter = 0;

    double midU = dU/2.0;
    double shiftU = midU / gridMax;
    double midV = dV/2.0;
    double shiftV = midV / gridMax;

    hillTuple[2] = this->HillXVariance * this->XVarianceScaleFactor;
    hillTuple[3] = this->HillYVariance * this->YVarianceScaleFactor;
    hillTuple[4] = this->HillAmplitude * this->AmplitudeScaleFactor;
    for (int i = 0; i < static_cast<int>(gridMax); ++i)
    {
      hillTuple[0] = MinimumU + shiftU + (i / gridMax) * dU;
      for ( int j = 0; j < static_cast<int>(gridMax); ++j )
      {
        hillTuple[1] = MinimumV + shiftV + (j / gridMax) * dV;
        this->hillData->SetTuple(counter,hillTuple);
        ++counter;
      }
    }
    // Zero out the variance and amplitude for the remaining hills.
    hillTuple[2] = 0;
    hillTuple[3] = 0;
    hillTuple[4] = 0;
    for (int k = counter; k < this->NumberOfHills; ++k)
    {
      hillTuple[0] = MinimumU + midU;
      hillTuple[1] = MinimumV + midV;
      this->hillData->SetTuple(k,hillTuple);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkParametricRandomHills::ParametersChanged()
{
  if (this->previousNumberOfHills != this->NumberOfHills)
  {
    this->CopyParameters();
    return true;
  }
  if (this->previousHillXVariance != this->HillXVariance)
  {
    this->CopyParameters();
    return true;
  }
  if (this->previousHillYVariance != this->HillYVariance)
  {
    this->CopyParameters();
    return true;
  }
  if (this->previousHillAmplitude != this->HillAmplitude)
  {
    this->CopyParameters();
    return true;
  }
  if (this->previousRandomSeed != this->RandomSeed)
  {
    this->CopyParameters();
    return true;
  }
  if (this->previousXVarianceScaleFactor != this->XVarianceScaleFactor)
  {
    this->CopyParameters();
    return true;
  }
  if (this->previousYVarianceScaleFactor != this->YVarianceScaleFactor)
  {
    this->CopyParameters();
    return true;
  }
  if (this->previousAmplitudeScaleFactor != this->AmplitudeScaleFactor)
  {
    this->CopyParameters();
    return true;
  }
  if (this->previousAllowRandomGeneration != this->AllowRandomGeneration)
  {
    this->CopyParameters();
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkParametricRandomHills::CopyParameters()
{
  this->previousNumberOfHills = this->NumberOfHills;
  this->previousHillXVariance = this->HillXVariance;
  this->previousHillYVariance = this->HillYVariance;
  this->previousHillAmplitude = this->HillAmplitude;
  this->previousRandomSeed = this->RandomSeed;
  this->previousXVarianceScaleFactor = this->XVarianceScaleFactor;
  this->previousYVarianceScaleFactor = this->YVarianceScaleFactor;
  this->previousAmplitudeScaleFactor = this->AmplitudeScaleFactor;
  this->previousAllowRandomGeneration = this->AllowRandomGeneration;
}

//----------------------------------------------------------------------------
void vtkParametricRandomHills::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

   os << indent << "Hills: " << this->NumberOfHills << "\n";
   os << indent << "Hill variance x-direction: " << this->HillXVariance << "\n";
   os << indent << "Hill variance x-direction scaling factor: " <<
                    this->XVarianceScaleFactor << "\n";
   os << indent << "Hill variance y-direction: " << this->HillYVariance << "\n";
   os << indent << "Hill variance y-direction scaling factor: " <<
                    this->YVarianceScaleFactor << "\n";
   os << indent << "Hill amplitude (height): " << this->HillAmplitude << "\n";
   os << indent << "Amplitude scaling factor: " <<
                    this->AmplitudeScaleFactor << "\n";
   os << indent << "Random number generator seed: " <<
                    this->RandomSeed << "\n";
   os << indent << "Allow random generation: " <<
                    this->AllowRandomGeneration << "\n";
}
