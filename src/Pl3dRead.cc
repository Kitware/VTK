/*=========================================================================

  Program:   Visualization Library
  Module:    Pl3dRead.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <ctype.h>
#include <math.h>
#include "Pl3dRead.hh"
#include "ByteSwap.hh"

#define BINARY 0
#define ASCII 1

#define RHOINF 1.0
#define CINF 1.0
#define PINF ((RHOINF*CINF) * (RHOINF*CINF) / this->Gamma)
#define CV (this->R / (this->Gamma-1.0))
#define VINF (this->Fsmach*CINF)

vlPLOT3DReader::vlPLOT3DReader()
{
  this->FileFormat = WHOLE_SINGLE_GRID_NO_IBLANKING;

  this->XYZFilename = NULL;
  this->QFilename = NULL;
  this->FunctionFilename = NULL;

  this->GridNumber = 0;
  this->ScalarFunctionNumber = 100;
  this->VectorFunctionNumber = 202;
  this->FunctionFileFunctionNumber = -1;

  this->Fsmach = 0.0;
  this->Alpha = 0.0;
  this->Re = 0.0;
  this->Time = 0.0;

  this->R = 1.0;
  this->Gamma = 1.4;
  this->Uvinf = 0.0;
  this->Vvinf = 0.0;
  this->Wvinf = 0.0;

  this->Grid = NULL;
  this->Energy = NULL;
  this->Density = NULL;
  this->Momentum = NULL;

} 

vlPLOT3DReader::~vlPLOT3DReader()
{
  if ( this->XYZFilename ) delete [] this->XYZFilename;
  if ( this->QFilename ) delete [] this->QFilename;
  if ( this->FunctionFilename ) delete [] this->FunctionFilename;
}

void vlPLOT3DReader::Execute()
{
  FILE *xyzFp, *QFp, *funcFp;
  int error;
//
// Initialize output and read geometry
//
  this->Initialize();

  if ( this->XYZFilename == NULL )
    {
    vlErrorMacro(<< "Must specify geometry file");
    return;
    }
  if ( (xyzFp = fopen(this->XYZFilename, "r")) == NULL)
    {
    vlErrorMacro(<< "File: " << this->XYZFilename << " not found");
    return;
    }
  if ( this->GetFileType(xyzFp) == ASCII )
    error = this->ReadASCIIGrid(xyzFp);
  else
    error = this->ReadBinaryGrid(xyzFp);

  if ( error )
    {
    vlErrorMacro(<<"Error reading XYZ file");
    return;
    }
//
// Read solution file (if available)
//
  if ( this->QFilename && 
  (this->ScalarFunctionNumber >= 0 || this->VectorFunctionNumber >= 0) )
    {
    if ( (QFp = fopen(this->QFilename, "r")) == NULL)
      {
      vlErrorMacro(<< "File: " << this->QFilename << " not found");
      return;
      }

    if ( this->GetFileType(QFp) == ASCII )
      error = this->ReadASCIISolution(QFp);
    else
      error = this->ReadBinarySolution(QFp);

    if ( error )
      {
      vlErrorMacro(<<"Error reading solution file");
      return;
      }
    this->MapFunction(this->ScalarFunctionNumber);
    this->MapFunction(this->VectorFunctionNumber);
    }
//
// Read function file (if available)
//
  if ( this->FunctionFileFunctionNumber >= 0 && this->FunctionFilename != NULL)
    {
    if ( (funcFp = fopen(this->FunctionFilename, "r")) == NULL)
      {
      vlErrorMacro(<< "File: " << this->FunctionFilename << " not found");
      return;
      }

    if ( this->GetFileType(funcFp) == ASCII )
      error = this->ReadASCIIFunctionFile(funcFp);
    else
      error = this->ReadBinaryFunctionFile(funcFp);

    if ( error )
      {
      vlErrorMacro(<<"Error reading function file");
      return;
      }
    }
//
// Reading is finished; free any extra memory. Data objects that comprise the
// output will not be released with the UnRegister() method since they are
// registered more than once.
//
  delete [] this->TempStorage;
  this->TempStorage = NULL;

  this->Grid->UnRegister(this);
  this->Grid = NULL;

  if ( this->Density ) 
    {
    this->Density->UnRegister(this);
    this->Density = NULL;
    }

  if ( this->Energy ) 
    {
    this->Energy->UnRegister(this);
    this->Energy = NULL;
    }

  if ( this->Momentum ) 
    {
    this->Momentum->UnRegister(this);
    this->Momentum = NULL;
    }
}

int vlPLOT3DReader::ReadASCIIGrid(FILE *fp)
{
  return 1;
}

int vlPLOT3DReader::ReadASCIISolution(FILE *fp)
{
  return 1;
}

int vlPLOT3DReader::ReadASCIIFunctionFile(FILE *fp)
{
  return 1;
}

int vlPLOT3DReader::ReadBinaryGrid(FILE *fp)
{
  vlFloatPoints *newPts;
  int dim[3];
  int i, gridFound, offset, gridSize, maxGridSize;
  float x[3];

  if ( this->FileFormat == WHOLE_MULTI_GRID_NO_IBLANKING )
    {
    if ( fread (&(this->NumGrids), sizeof(int), 1, fp) < 1 ) return 1;
    }
  else
    {
    this->NumGrids = 1;
    }
//
// Loop over grids, reading one that has been specified
//
  for (gridFound=0, offset=0, maxGridSize=0, i=0; i<this->NumGrids; i++) 
    {
    //read dimensions
    if ( fread (dim, sizeof(int), 3, fp) < 3 ) return 1;
    gridSize = dim[0] * dim[1] * dim[2];

    if ( i < this->GridNumber ) 
      offset += 3*gridSize;
    else if ( i == this->GridNumber ) 
      {
      gridFound = 1;
      this->NumPts = gridSize;
      this->SetDimensions(dim);
      }
    }

  if ( ! gridFound )
    {
    vlErrorMacro (<<"Specified grid not found!");
    return 1;
    }
    
  //allocate temporary storage to read into + points
  this->TempStorage = new float[3*this->NumPts];
  newPts = new vlFloatPoints(this->NumPts);

  //seek to correct spot and read grid
  fseek (fp, offset*sizeof(float), 1);

  if ( fread (this->TempStorage, sizeof(float), 3*this->NumPts, fp) < 3*this->NumPts ) 
    {
    delete newPts;
    delete [] this->TempStorage;
    return 1;
    }
  else //successful read, load coordinates in points object
    {
    for (i=0; i < this->NumPts; i++)
      {
      x[0] = this->TempStorage[i];
      x[1] = this->TempStorage[this->NumPts+i];
      x[2] = this->TempStorage[2*this->NumPts+i];
      newPts->SetPoint(i,x);
      }
    }
//
// Now send data to ourselves
//
  this->Grid = newPts;
  this->Grid->Register(this);
  this->SetPoints(newPts);

  vlDebugMacro(<<"Read " << this->NumPts << " points");
  return 0;
}

int vlPLOT3DReader::ReadBinarySolution(FILE *fp)
{
  vlFloatScalars *newDensity, *newEnergy;
  vlFloatVectors *newMomentum;
  int dim[3];
  int i, gridFound, offset, gridSize, maxGridSize;
  float *temp, m[3], params[4];
  int numGrids, numPts;

  if ( this->FileFormat == WHOLE_MULTI_GRID_NO_IBLANKING )
    {
    if ( fread (&numGrids, sizeof(int), 1, fp) < 1 ) return 1;
    }
  else
    {
    numGrids = 1;
    }

  if ( numGrids != this->NumGrids )
    {
    vlErrorMacro(<<"Data mismatch in solution file!");
    return 1;
    }
//
// Loop over dimensions, reading grid dimensions that have been specified
//
  for (gridFound=0, offset=0, maxGridSize=0, i=0; i<numGrids; i++) 
    {
    //read dimensions
    if ( fread (dim, sizeof(int), 3, fp) < 3 ) return 1;
    gridSize = dim[0] * dim[1] * dim[2];

    if ( i < this->GridNumber ) 
      offset += 5*gridSize;
    else if ( i == this->GridNumber ) 
      {
      gridFound = 1;
      numPts = gridSize;
      this->SetDimensions(dim);
      }
    }

  if ( ! gridFound )
    {
    vlErrorMacro (<<"Specified grid not found!");
    return 1;
    }
    
  if ( numPts != this->NumPts )
    {
    vlErrorMacro (<<"Data mismatch in solution file!");
    delete [] this->TempStorage;
    return 1;
    }
    
  //seek to correct spot and read solution
  fseek (fp, offset*sizeof(float), 1);

  //read solution parameters
  if ( fread (params, sizeof(float), 4, fp) < 4 ) return 1;
  this->Fsmach = params[0];
  this->Alpha = params[1];
  this->Re = params[2];
  this->Time = params[3];

  //allocate temporary storage to copy density data into
  newDensity = new vlFloatScalars(numPts);
  newEnergy = new vlFloatScalars(numPts);
  newMomentum = new vlFloatVectors(numPts);

  if ( fread (this->TempStorage, sizeof(float), numPts, fp) < numPts ) 
    {
    delete newDensity;
    delete newMomentum;
    delete newEnergy;
    delete [] this->TempStorage;
    return 1;
    }
  else //successful read
    {
    for (i=0; i < this->NumPts; i++) 
      newDensity->SetScalar(i,this->TempStorage[i]);
    }

  if ( fread (this->TempStorage, sizeof(float), 3*this->NumPts, fp) < 3*this->NumPts ) 
    {
    delete newDensity;
    delete newMomentum;
    delete newEnergy;
    delete [] this->TempStorage;
    return 1;
    }
  else //successful read, load coordinates into vector object
    {
    for (i=0; i < this->NumPts; i++)
      {
      m[0] = this->TempStorage[i];
      m[1] = this->TempStorage[this->NumPts+i];
      m[2] = this->TempStorage[2*this->NumPts+i];
      newMomentum->SetVector(i,m);
      }
    }

  if ( fread (this->TempStorage, sizeof(float), numPts, fp) < numPts ) 
    {
    delete newDensity;
    delete newMomentum;
    delete newEnergy;
    delete [] this->TempStorage;
    return 1;
    }
  else //successful read
    {
    for (i=0; i < this->NumPts; i++) 
      newEnergy->SetScalar(i,this->TempStorage[i]);
    }
//
// Register data for use by computation functions
//
  this->Density = newDensity;
  this->Density->Register(this);

  this->Momentum = newMomentum;
  this->Momentum->Register(this);

  this->Energy = newEnergy;
  this->Energy->Register(this);

  return 0;
}

int vlPLOT3DReader::ReadBinaryFunctionFile(FILE *fp)
{
}

//
// Various PLOT3D functions.....................
//

void vlPLOT3DReader::MapFunction(int fNumber)
{
  switch (fNumber)
    {
    case -1: //empty mapping
      break;

    case 100: //Density
      this->ComputeDensity();
      break;

    case 110: //Pressure
      this->ComputePressure();
      break;

    case 120: //Temperature
      this->ComputeTemperature();
      break;

    case 130: //Enthalpy
      this->ComputeEnthalpy();
      break;

    case 140: //Internal Energy
      this->ComputeInternalEnergy();
      break;

    case 144: //Kinetic Energy
      this->ComputeKineticEnergy();
      break;

    case 153: //Velocity Magnitude
      this->ComputeVelocityMagnitude();
      break;

    case 163: //Stagnation energy
      this->ComputeStagnationEnergy();
      break;

    case 170: //Entropy
      this->ComputeEntropy();
      break;

    case 184: //Swirl
      this->ComputeSwirl();
      break;

    case 200: //Velocity
      this->ComputeVelocity();
      break;

    case 201: //Vorticity
      this->ComputeVorticity();
      break;

    case 202: //Momentum
      this->ComputeMomentum();
      break;

    case 210: //PressureGradient
      this->ComputePressureGradient();
      break;

    default:
      vlErrorMacro(<<"No function number " << fNumber);
    }
}

void vlPLOT3DReader::ComputeDensity()
{
  this->PointData.SetScalars(this->Density);
  vlDebugMacro(<<"Created density scalar");
}

void vlPLOT3DReader::ComputeTemperature()
{
  float *m, e, rr, u, v, w, v2, p, d, rrgas;
  int i;
  vlFloatScalars *temperature;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL || 
  this->Energy == NULL )
    {
    vlErrorMacro(<<"Cannot compute temperature");
    return;
    }

  temperature = new vlFloatScalars(this->NumPts);
//
//  Compute the temperature
//
  rrgas = 1.0 / this->R;
  for (i=0; i < this->NumPts; i++) 
    {
    d = this->Density->GetScalar(i);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetVector(i);
    e = this->Energy->GetScalar(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    p = (this->Gamma-1.) * (e - 0.5 * d * v2);
    temperature->SetScalar(i, p*rr*rrgas);
  }
  this->PointData.SetScalars(temperature);
  vlDebugMacro(<<"Created temperature scalar");
}

void vlPLOT3DReader::ComputePressure()
{
  float *m, e, u, v, w, v2, p, d, t, rr;
  int i;
  vlFloatScalars *pressure;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL || 
  this->Energy == NULL )
    {
    vlErrorMacro(<<"Cannot compute pressure");
    return;
    }

  pressure = new vlFloatScalars(this->NumPts);
//
//  Compute the pressure
//
  for (i=0; i < this->NumPts; i++) 
    {
    d = this->Density->GetScalar(i);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetVector(i);
    e = this->Energy->GetScalar(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    p = (this->Gamma-1.) * (e - 0.5 * d * v2);
    pressure->SetScalar(i, p);
  }
  this->PointData.SetScalars(pressure);
  vlDebugMacro(<<"Created pressure scalar");
}

void vlPLOT3DReader::ComputeEnthalpy()
{
  float *m, e, u, v, w, v2, d, rr;
  int i;
  vlFloatScalars *enthalpy;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL || 
  this->Energy == NULL )
    {
    vlErrorMacro(<<"Cannot compute enthalpy");
    return;
    }

  enthalpy = new vlFloatScalars(this->NumPts);
//
//  Compute the enthalpy
//
  for (i=0; i < this->NumPts; i++) 
    {
    d = this->Density->GetScalar(i);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetVector(i);
    e = this->Energy->GetScalar(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    enthalpy->SetScalar(i, this->Gamma*(e*rr - 0.5*v2));
  }
  this->PointData.SetScalars(enthalpy);
  vlDebugMacro(<<"Created enthalpy scalar");
}

void vlPLOT3DReader::ComputeInternalEnergy()
{
  this->PointData.SetScalars(this->Energy);
  vlDebugMacro(<<"Created energy scalar");
}

void vlPLOT3DReader::ComputeKineticEnergy()
{
  float *m, u, v, w, v2, d, rr;
  int i;
  vlFloatScalars *kineticEnergy;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL )
    {
    vlErrorMacro(<<"Cannot compute kinetic energy");
    return;
    }

  kineticEnergy = new vlFloatScalars(this->NumPts);
//
//  Compute the kinetic energy
//
  for (i=0; i < this->NumPts; i++) 
    {
    d = this->Density->GetScalar(i);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetVector(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    kineticEnergy->SetScalar(i, 0.5*v2);
  }
  this->PointData.SetScalars(kineticEnergy);
  vlDebugMacro(<<"Created kinetic energy scalar");
}

void vlPLOT3DReader::ComputeVelocityMagnitude()
{
  float *m, u, v, w, v2, d, rr, e;
  int i;
  vlFloatScalars *velocityMag;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vlErrorMacro(<<"Cannot compute velocity magnitude");
    return;
    }

  velocityMag = new vlFloatScalars(this->NumPts);
//
//  Compute the velocity magnitude
//
  for (i=0; i < this->NumPts; i++) 
    {
    d = this->Density->GetScalar(i);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetVector(i);
    e = this->Energy->GetScalar(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    velocityMag->SetScalar(i, sqrt((double)v2));
  }
  this->PointData.SetScalars(velocityMag);
  vlDebugMacro(<<"Created velocity magnitude scalar");
}

void vlPLOT3DReader::ComputeStagnationEnergy()
{
  this->PointData.SetScalars(this->Energy);
  vlDebugMacro(<<"Created stagnation energy scalar");
}

void vlPLOT3DReader::ComputeEntropy()
{
  float *m, u, v, w, v2, d, rr, s, p, e;
  int i;
  vlFloatScalars *entropy;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vlErrorMacro(<<"Cannot compute entropy");
    return;
    }

  entropy = new vlFloatScalars(this->NumPts);
//
//  Compute the entropy
//
  for (i=0; i < this->NumPts; i++) 
    {
    d = this->Density->GetScalar(i);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetVector(i);
    e = this->Energy->GetScalar(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    p = (this->Gamma-1.)*(e - 0.5*d*v2);
    s = CV * log( pow((double)(p/PINF)/d/RHOINF,(double)this->Gamma) );
    entropy->SetScalar(i, s);
  }
  this->PointData.SetScalars(entropy);
  vlDebugMacro(<<"Created entropy scalar");
}

void vlPLOT3DReader::ComputeSwirl()
{
  vlVectors *currentVector;
  vlVectors *vorticity;
  float d, rr, *m, u, v, w, v2, *vort, s;
  int i;
  vlFloatScalars *swirl;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vlErrorMacro(<<"Cannot compute swirl");
    return;
    }

  swirl = new vlFloatScalars(this->NumPts);

  currentVector = this->PointData.GetVectors();
  currentVector->Register(this);

  this->ComputeVorticity();
  vorticity = this->PointData.GetVectors();
//
//  Compute the swirl
//
  for (i=0; i < this->NumPts; i++) 
    {
    d = this->Density->GetScalar(i);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetVector(i);
    vort = vorticity->GetVector(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    if ( v2 != 0.0 ) 
      s = (vort[0]*m[0] + vort[1]*m[1] + vort[2]*m[2]) / v2;
    else 
      s = 0.0;

    swirl->SetScalar(i,s);
  }
  this->PointData.SetScalars(swirl);
  vlDebugMacro(<<"Created swirl scalar");

  // reset current vector
  this->PointData.SetVectors(currentVector);
  currentVector->UnRegister(this);
}

// Vector functions
void vlPLOT3DReader::ComputeVelocity()
{
  float *m, v[3], d, rr;
  int i;
  vlFloatVectors *velocity;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vlErrorMacro(<<"Cannot compute velocity");
    return;
    }

  velocity = new vlFloatVectors(this->NumPts);
//
//  Compute the velocity
//
  for (i=0; i < this->NumPts; i++) 
    {
    d = this->Density->GetScalar(i);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetVector(i);
    rr = 1.0 / d;
    v[0] = m[0] * rr;        
    v[1] = m[1] * rr;        
    v[2] = m[2] * rr;        
    velocity->SetVector(i, v);
  }
  this->PointData.SetVectors(velocity);
  vlDebugMacro(<<"Created velocity vector");
}

void vlPLOT3DReader::ComputeVorticity()
{
  vlVectors *velocity;
  vlFloatVectors *vorticity;
  int dims[3], ijsize;
  vlPoints *points;
  int i, j, k, idx, idx2;
  float vort[3], xp[3], xm[3], vp[3], vm[3], factor;
  float xxi, yxi, zxi, uxi, vxi, wxi;
  float xeta, yeta, zeta, ueta, veta, weta;
  float xzeta, yzeta, zzeta, uzeta, vzeta, wzeta;
  float aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;
//
//  Check that the required data is available
//
  if ( (points=this->GetPoints()) == NULL || this->Density == NULL || 
  this->Momentum == NULL || this->Energy == NULL )
    {
    vlErrorMacro(<<"Cannot compute vorticity");
    return;
    }

  vorticity = new vlFloatVectors(this->NumPts);

  this->ComputeVelocity();
  velocity = this->PointData.GetVectors();

  this->GetDimensions(dims);
  ijsize = dims[0]*dims[1];

  for (k=0; k<dims[2]; k++) 
    {
    for (j=0; j<dims[1]; j++) 
      {
      for (i=0; i<dims[0]; i++) 
        {
//
//  Xi derivatives.
//
        if ( dims[0] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (i=0; i<3; i++) vp[i] = vm[i] = xp[i] = xm[i] = 0.0;
          xp[0] = 1.0;
          }
        else if ( i == 0 ) 
          {
          factor = 1.0;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetVector(idx,vp);
          velocity->GetVector(idx2,vm);
          } 
        else if ( i == (dims[0]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i-1 + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetVector(idx,vp);
          velocity->GetVector(idx2,vm);
          } 
        else 
          {
          factor = 0.5;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = (i-1) + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetVector(idx,vp);
          velocity->GetVector(idx2,vm);
          }

        xxi = factor * (xp[0] - xm[0]);
        yxi = factor * (xp[1] - xm[1]);
        zxi = factor * (xp[2] - xm[2]);
        uxi = factor * (vp[0] - vm[0]);
        vxi = factor * (vp[1] - vm[1]);
        wxi = factor * (vp[2] - vm[2]);
//
//  Eta derivatives.
//
        if ( dims[1] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (i=0; i<3; i++) vp[i] = vm[i] = xp[i] = xm[i] = 0.0;
          xp[1] = 1.0;
          }
        else if ( j == 0 ) 
          {
          factor = 1.0;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetVector(idx,vp);
          velocity->GetVector(idx2,vm);
          } 
        else if ( j == (dims[1]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetVector(idx,vp);
          velocity->GetVector(idx2,vm);
          } 
        else 
          {
          factor = 0.5;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetVector(idx,vp);
          velocity->GetVector(idx2,vm);
          }

        xeta = factor * (xp[0] - xm[0]);
        yeta = factor * (xp[1] - xm[1]);
        zeta = factor * (xp[2] - xm[2]);
        ueta = factor * (vp[0] - vm[0]);
        veta = factor * (vp[1] - vm[1]);
        weta = factor * (vp[2] - vm[2]);
//
//  Zeta derivatives.
//
        if ( dims[2] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (i=0; i<3; i++) vp[i] = vm[i] = xp[i] = xm[i] = 0.0;
          xp[2] = 1.0;
          }
        else if ( k == 0 ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetVector(idx,vp);
          velocity->GetVector(idx2,vm);
          } 
        else if ( k == (dims[2]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetVector(idx,vp);
          velocity->GetVector(idx2,vm);
          } 
        else 
          {
          factor = 0.5;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetVector(idx,vp);
          velocity->GetVector(idx2,vm);
          }

        xzeta = factor * (xp[0] - xm[0]);
        yzeta = factor * (xp[1] - xm[1]);
        zzeta = factor * (xp[2] - xm[2]);
        uzeta = factor * (vp[0] - vm[0]);
        vzeta = factor * (vp[1] - vm[1]);
        wzeta = factor * (vp[2] - vm[2]);
//
//  Now calculate the Jacobian.  Grids occasionally have singularities, or
//  points where the Jacobian is infinite (the inverse is zero).  For these
//  cases, we'll set the Jacobian to zero, which will result in a zero 
//  vorticity.
//
        aj =  xxi*yeta*zzeta+yxi*zeta*xzeta+zxi*xeta*yzeta
              -zxi*yeta*xzeta-yxi*xeta*zzeta-xxi*zeta*yzeta;
        if (aj != 0.0) aj = 1. / aj;
//
//  Xi metrics.
//
        xix  =  aj*(yeta*zzeta-zeta*yzeta);
        xiy  = -aj*(xeta*zzeta-zeta*xzeta);
        xiz  =  aj*(xeta*yzeta-yeta*xzeta);
//
//  Eta metrics.
//
        etax = -aj*(yxi*zzeta-zxi*yzeta);
        etay =  aj*(xxi*zzeta-zxi*xzeta);
        etaz = -aj*(xxi*yzeta-yxi*xzeta);
//
//  Zeta metrics.
//
        zetax=  aj*(yxi*zeta-zxi*yeta);
        zetay= -aj*(xxi*zeta-zxi*xeta);
        zetaz=  aj*(xxi*yeta-yxi*xeta);
//
//
//  Finally, the vorticity components.
//
        vort[0]= xiy*wxi+etay*weta+zetay*wzeta - xiz*vxi-etaz*veta-zetaz*vzeta;
        vort[1]= xiz*uxi+etaz*ueta+zetaz*uzeta - xix*wxi-etax*weta-zetax*wzeta;
        vort[2]= xix*vxi+etax*veta+zetax*vzeta - xiy*uxi-etay*ueta-zetay*uzeta;
        idx = i + j*dims[0] + k*ijsize;
        vorticity->SetVector(idx,vort);
        }
      }
    }

  this->PointData.SetVectors(vorticity);
  vlDebugMacro(<<"Created vorticity vector");
}

void vlPLOT3DReader::ComputeMomentum()
{
  this->PointData.SetVectors(this->Momentum);
  vlDebugMacro(<<"Created momentum vector");
}

void vlPLOT3DReader::ComputePressureGradient()
{
  vlScalars *currentScalar;
  vlScalars *pressure;
  vlFloatVectors *gradient;
  int dims[3], ijsize;
  vlPoints *points;
  int i, j, k, idx, idx2;
  float g[3], xp[3], xm[3], pp, pm, factor;
  float xxi, yxi, zxi, pxi;
  float xeta, yeta, zeta, peta;
  float xzeta, yzeta, zzeta, pzeta;
  float aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;
//
//  Check that the required data is available
//
  if ( (points=this->GetPoints()) == NULL || this->Density == NULL || 
  this->Momentum == NULL || this->Energy == NULL )
    {
    vlErrorMacro(<<"Cannot compute pressure gradient");
    return;
    }

  gradient = new vlFloatVectors(this->NumPts);

  currentScalar = this->PointData.GetScalars();
  currentScalar->Register(this);

  this->ComputePressure();
  pressure = this->PointData.GetScalars();

  this->GetDimensions(dims);
  ijsize = dims[0]*dims[1];

  for (k=0; k<dims[2]; k++) 
    {
    for (j=0; j<dims[1]; j++) 
      {
      for (i=0; i<dims[0]; i++) 
        {
//
//  Xi derivatives.
//
        if ( dims[0] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (i=0; i<3; i++) xp[i] = xm[i] = 0.0;
          xp[0] = 1.0; pp = pm = 0.0;
          }
        else if ( i == 0 ) 
          {
          factor = 1.0;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetScalar(idx);
          pm = pressure->GetScalar(idx2);
          } 
        else if ( i == (dims[0]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i-1 + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetScalar(idx);
          pm = pressure->GetScalar(idx2);
          } 
        else 
          {
          factor = 0.5;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = (i-1) + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetScalar(idx);
          pm = pressure->GetScalar(idx2);
          }

        xxi = factor * (xp[0] - xm[0]);
        yxi = factor * (xp[1] - xm[1]);
        zxi = factor * (xp[2] - xm[2]);
        pxi = factor * (pp - pm);
//
//  Eta derivatives.
//
        if ( dims[1] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (i=0; i<3; i++) xp[i] = xm[i] = 0.0;
          xp[1] = 1.0; pp = pm = 0.0;
          }
        else if ( j == 0 ) 
          {
          factor = 1.0;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetScalar(idx);
          pm = pressure->GetScalar(idx2);
          } 
        else if ( j == (dims[1]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetScalar(idx);
          pm = pressure->GetScalar(idx2);
          } 
        else 
          {
          factor = 0.5;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetScalar(idx);
          pm = pressure->GetScalar(idx2);
          }

        xeta = factor * (xp[0] - xm[0]);
        yeta = factor * (xp[1] - xm[1]);
        zeta = factor * (xp[2] - xm[2]);
        peta = factor * (pp - pm);
//
//  Zeta derivatives.
//
        if ( dims[2] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (i=0; i<3; i++) xp[i] = xm[i] = 0.0;
          xp[2] = 1.0; pp = pm = 0.0;
          }
        else if ( k == 0 ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetScalar(idx);
          pm = pressure->GetScalar(idx2);
          } 
        else if ( k == (dims[2]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetScalar(idx);
          pm = pressure->GetScalar(idx2);
          } 
        else 
          {
          factor = 0.5;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetScalar(idx);
          pm = pressure->GetScalar(idx2);
          }

        xzeta = factor * (xp[0] - xm[0]);
        yzeta = factor * (xp[1] - xm[1]);
        zzeta = factor * (xp[2] - xm[2]);
        pzeta = factor * (pp - pm);
//
//  Now calculate the Jacobian.  Grids occasionally have singularities, or
//  points where the Jacobian is infinite (the inverse is zero).  For these
//  cases, we'll set the Jacobian to zero, which will result in a zero 
//  vorticity.
//
        aj =  xxi*yeta*zzeta+yxi*zeta*xzeta+zxi*xeta*yzeta
              -zxi*yeta*xzeta-yxi*xeta*zzeta-xxi*zeta*yzeta;
        if (aj != 0.0) aj = 1. / aj;
//
//  Xi metrics.
//
        xix  =  aj*(yeta*zzeta-zeta*yzeta);
        xiy  = -aj*(xeta*zzeta-zeta*xzeta);
        xiz  =  aj*(xeta*yzeta-yeta*xzeta);
//
//  Eta metrics.
//
        etax = -aj*(yxi*zzeta-zxi*yzeta);
        etay =  aj*(xxi*zzeta-zxi*xzeta);
        etaz = -aj*(xxi*yzeta-yxi*xzeta);
//
//  Zeta metrics.
//
        zetax=  aj*(yxi*zeta-zxi*yeta);
        zetay= -aj*(xxi*zeta-zxi*xeta);
        zetaz=  aj*(xxi*yeta-yxi*xeta);
//
//
//  Finally, the vorticity components.
//
        g[0]= xix*pxi+etax*peta+zetax*pzeta;
        g[1]= xiy*pxi+etay*peta+zetay*pzeta;
        g[2]= xiz*pxi+etaz*peta+zetaz*pzeta;

        idx = i + j*dims[0] + k*ijsize;
        gradient->SetVector(idx,g);
        }
      }
    }

  this->PointData.SetVectors(gradient);
  vlDebugMacro(<<"Created pressure gradient vector");

  // reset current scalar
  this->PointData.SetScalars(currentScalar);
  currentScalar->UnRegister(this);
}

int vlPLOT3DReader::GetFileType(FILE *fp)
{
  char fourBytes[4];
  int type, i;
//
//  Read a little from the file to figure what type it is.
//
  fgets (fourBytes, 4, fp);
  for (i=0, type=ASCII; i<4 && type == ASCII; i++)
    if ( ! isprint(fourBytes[i]) )
      type = BINARY;
//
// Reset file for reading
//
  rewind (fp);
  return type;
}

void vlPLOT3DReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredGridSource::PrintSelf(os,indent);

  os << indent << "XYZ Filename: " << this->XYZFilename << "\n";
  os << indent << "Q Filename: " << this->QFilename << "\n";
  os << indent << "Function Filename: " << this->FunctionFilename << "\n";

  os << indent << "Grid Number: " << this->GridNumber << "\n";
  os << indent << "Scalar Function Number: " << this->ScalarFunctionNumber << "\n";
  os << indent << "Vector Function Number: " << this->VectorFunctionNumber << "\n";
  os << indent << "Function Number: " << this->FunctionFileFunctionNumber << "\n";

  os << indent << "Free Stream Mach Number: " << this->Fsmach << "\n";
  os << indent << "Alpha: " << this->Alpha << "\n";
  os << indent << "Reynolds Number " << this->Re << "\n";
  os << indent << "Total Integration Time: " << this->Time << "\n";

  os << indent << "R: " << this->R << "\n";
  os << indent << "Gamma: " << this->Gamma << "\n";
  os << indent << "UVinf: " << this->Uvinf << "\n";
  os << indent << "VVinf: " << this->Vvinf << "\n";
  os << indent << "WVinf: " << this->Wvinf << "\n";
}

