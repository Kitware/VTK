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

#include <time.h>

vtkCxxRevisionMacro(vtkParametricRandomHills, "1.1");
vtkStandardNewMacro(vtkParametricRandomHills);

vtkParametricRandomHills::vtkParametricRandomHills() :
  NumberOfHills(30)
  , HillXVariance(2.5)
  , HillYVariance(2.5)
  , HillAmplitude(2)
  , RandomSeed(1)
  , XVarianceScaleFactor(1.0/3.0)
  , YVarianceScaleFactor(1.0/3.0)
  , AmplitudeScaleFactor(1.0/3.0)
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

  GenerateTheHills();
}

vtkParametricRandomHills::~vtkParametricRandomHills()
{
  this->hillData->Delete();
}

//! Initialise the random number generator.
void vtkParametricRandomHills::InitSeed ( int RandomSeed )
{
  if ( RandomSeed >= 0 )
    srand( (unsigned int) RandomSeed );
  else
    srand( (unsigned)time( NULL ) );
}

//! Return a random number between 0 and 1.
double vtkParametricRandomHills::Rand ( void )
{
  return double(rand())/double(RAND_MAX);
}

void vtkParametricRandomHills::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
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
  double hillTuple[5]; // 0: mX, 1: mY, 2: VarX, 3: VarY, 4: Amplitude
  for ( int i = 0; i < NumberOfHills; ++i )
  {
    this->hillData->GetTuple(i,hillTuple);
    double x = (Pt[0] - hillTuple[0])/hillTuple[2];
    double y = (Pt[1] - hillTuple[1])/hillTuple[3];
    Pt[2] += hillTuple[4] * exp( -(x*x+y*y) / 2.0 );
  }
}

double vtkParametricRandomHills::EvaluateScalar(double* vtkNotUsed(uv[3]), 
                                                double* vtkNotUsed(Pt[3]), 
                                                double* vtkNotUsed(Duv[9]))
{
  return 0;
}

void vtkParametricRandomHills::GenerateTheHills( void )
{
  double min_x = (MaximumU>MinimumU)?MinimumU:MaximumU;
  double min_y = (MaximumV>MinimumV)?MinimumV:MaximumV;

  this->hillData->Initialize();
  this->hillData->SetNumberOfComponents(5);
  this->hillData->SetNumberOfTuples(NumberOfHills);

  double hillTuple[5]; // 0: mX, 1: mY, 2: VarX, 3: VarY, 4: Amplitude

  // Generate the centers of the Hills, standard deviations and amplitudes.
  InitSeed(this->RandomSeed);
  for ( int i = 0; i < this->NumberOfHills; ++ i )
  {
    hillTuple[0] = min_x + Rand() * (MaximumU - MinimumU);
    hillTuple[1] = min_y + Rand() * (MaximumV - MinimumV);
    hillTuple[2] = this->HillXVariance * Rand() + this->HillXVariance * this->XVarianceScaleFactor;
    hillTuple[3] = this->HillYVariance * Rand() + this->HillYVariance * this->YVarianceScaleFactor;
    hillTuple[4] = this->HillAmplitude * Rand() + this->HillAmplitude * this->AmplitudeScaleFactor;
    this->hillData->SetTuple(i,hillTuple);
  }
  this->Modified();
}

void vtkParametricRandomHills::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

   os << indent << "Hills: " << this->NumberOfHills << "\n";
   os << indent << "Hill variance x-direction: " << this->HillXVariance << "\n";
   os << indent << "Hill variance x-direction scaling factor: " << this->XVarianceScaleFactor << "\n";
   os << indent << "Hill variance y-direction: " << this->HillYVariance << "\n";
   os << indent << "Hill variance y-direction scaling factor: " << this->YVarianceScaleFactor << "\n";
   os << indent << "Hill amplitude (height): " << this->HillAmplitude << "\n";
   os << indent << "Amplitude scaling factor: " << this->AmplitudeScaleFactor << "\n";
   os << indent << "Random number generator seed: " << this->RandomSeed << "\n";
}
