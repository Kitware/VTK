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
#include "Pl3dRead.hh"
#include "ByteSwap.hh"

#define BINARY 0
#define ASCII 1

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
}

int vlPLOT3DReader::ReadASCIISolution(FILE *fp)
{
}

int vlPLOT3DReader::ReadASCIIFunctionFile(FILE *fp)
{
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
  delete [] this->TempStorage;
  this->TempStorage = NULL;

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
}

void vlPLOT3DReader::ComputePressure()
{
}

void vlPLOT3DReader::ComputeEnthalpy()
{
}

void vlPLOT3DReader::ComputeInternalEnergy()
{
  this->PointData.SetScalars(this->Energy);
  vlDebugMacro(<<"Created energy scalar");
}

void vlPLOT3DReader::ComputeKineticEnergy()
{
}

void vlPLOT3DReader::ComputeVelocityMagnitude()
{
}

void vlPLOT3DReader::ComputeStagnationEnergy()
{
}

void vlPLOT3DReader::ComputeEntropy()
{
}

void vlPLOT3DReader::ComputeSwirl()
{
}



void vlPLOT3DReader::ComputeVelocity()
{
}

void vlPLOT3DReader::ComputeVorticity()
{
}

void vlPLOT3DReader::ComputeMomentum()
{
  this->PointData.SetVectors(this->Momentum);
  vlDebugMacro(<<"Created momentum vector");
}

void vlPLOT3DReader::ComputePressureGradient()
{
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
}

