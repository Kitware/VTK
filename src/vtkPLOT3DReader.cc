/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLOT3DReader.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <ctype.h>
#include <math.h>
#include "vtkPLOT3DReader.hh"
#include "vtkByteSwap.hh"

#define BINARY 0
#define ASCII 1

#define RHOINF 1.0
#define CINF 1.0
#define PINF ((RHOINF*CINF) * (RHOINF*CINF) / this->Gamma)
#define CV (this->R / (this->Gamma-1.0))
#define VINF (this->Fsmach*CINF)

vtkPLOT3DReader::vtkPLOT3DReader()
{
  this->FileFormat = VTK_WHOLE_SINGLE_GRID_NO_IBLANKING;

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

vtkPLOT3DReader::~vtkPLOT3DReader()
{
  if ( this->XYZFilename ) delete [] this->XYZFilename;
  if ( this->QFilename ) delete [] this->QFilename;
  if ( this->FunctionFilename ) delete [] this->FunctionFilename;
}

void vtkPLOT3DReader::Execute()
{
  FILE *xyzFp, *QFp, *funcFp;
  int error = 0;
  vtkStructuredGrid *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  //
  // Initialize output and read geometry
  //

  if ( this->XYZFilename == NULL )
    {
    vtkErrorMacro(<< "Must specify geometry file");
    return;
    }
  if ( (xyzFp = fopen(this->XYZFilename, "r")) == NULL)
    {
    vtkErrorMacro(<< "File: " << this->XYZFilename << " not found");
    return;
    }
  if ( this->GetFileType(xyzFp) == ASCII )
    vtkWarningMacro("reading ascii grid files currently not supported");
    // error = this->ReadASCIIGrid(xyzFp);
  else
    {
    fclose(xyzFp);
    xyzFp = fopen(this->XYZFilename, "rb");
    error = this->ReadBinaryGrid(xyzFp,output);
    }
  
  if ( error )
    {
    vtkErrorMacro(<<"Error reading XYZ file");
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
      vtkErrorMacro(<< "File: " << this->QFilename << " not found");
      return;
      }

    if ( this->GetFileType(QFp) == ASCII )
      vtkWarningMacro("reading ascii solution files currently not supported");
    // error = this->ReadASCIISolution(QFp);
    else
      {
      fclose(QFp);
      QFp = fopen(this->QFilename, "rb");
      error = this->ReadBinarySolution(QFp,output);
      }
    
    if ( error )
      {
      vtkErrorMacro(<<"Error reading solution file");
      return;
      }
    this->MapFunction(this->ScalarFunctionNumber,outputPD);
    this->MapFunction(this->VectorFunctionNumber,outputPD);
    }
//
// Read function file (if available)
//
  if ( this->FunctionFileFunctionNumber >= 0 && this->FunctionFilename != NULL)
    {
    if ( (funcFp = fopen(this->FunctionFilename, "r")) == NULL)
      {
      vtkErrorMacro(<< "File: " << this->FunctionFilename << " not found");
      return;
      }

    if ( this->GetFileType(funcFp) == ASCII )
      vtkWarningMacro("reading function files currently not supported");
    // error = this->ReadASCIIFunctionFile(funcFp);
    else
      {
      fclose(funcFp);
      funcFp = fopen(this->FunctionFilename, "rb");
      vtkWarningMacro("reading function files currently not supported");
      // error = this->ReadBinaryFunctionFile(funcFp);
      }
    
    if ( error )
      {
      vtkErrorMacro(<<"Error reading function file");
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

int vtkPLOT3DReader::ReadBinaryGrid(FILE *fp,vtkStructuredGrid *output)
{
  vtkFloatPoints *newPts;
  int dim[3];
  int i, gridFound, offset, gridSize, maxGridSize;
  float x[3];
  vtkByteSwap swapper;
  
  if ( this->FileFormat == VTK_WHOLE_MULTI_GRID_NO_IBLANKING )
    {
    if (fread(&(this->NumGrids), sizeof(int), 1, fp) < 1 ) return 1;
    swapper.Swap4BE(&(this->NumGrids));
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
    swapper.Swap4BERange(dim,3);
    
    gridSize = dim[0] * dim[1] * dim[2];

    if ( i < this->GridNumber ) 
      offset += 3*gridSize;
    else if ( i == this->GridNumber ) 
      {
      gridFound = 1;
      this->NumPts = gridSize;
      output->SetDimensions(dim);
      }
    }

  if ( ! gridFound )
    {
    vtkErrorMacro (<<"Specified grid not found!");
    return 1;
    }
    
  //allocate temporary storage to read into + points
  this->TempStorage = new float[3*this->NumPts];
  newPts = new vtkFloatPoints(this->NumPts);

  //seek to correct spot and read grid
  fseek (fp, offset*sizeof(float), 1);

  if ( fread(this->TempStorage, sizeof(float), 3*this->NumPts, fp) < (unsigned long)3*this->NumPts ) 
    {
    newPts->Delete();
    delete [] this->TempStorage;
    return 1;
    }
  else //successful read, load coordinates in points object
    {
    swapper.Swap4BERange(this->TempStorage,3*this->NumPts);
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
  output->SetPoints(newPts);
  newPts->Delete();

  vtkDebugMacro(<<"Read " << this->NumPts << " points");
  return 0;
}

int vtkPLOT3DReader::ReadBinarySolution(FILE *fp,vtkStructuredGrid *output)
{
  vtkFloatScalars *newDensity, *newEnergy;
  vtkFloatVectors *newMomentum;
  int dim[3];
  int i, gridFound, offset, gridSize, maxGridSize;
  float m[3], params[4];
  int numGrids, numPts = 0;
  vtkByteSwap swapper;

  if ( this->FileFormat == VTK_WHOLE_MULTI_GRID_NO_IBLANKING )
    {
    if ( fread (&numGrids, sizeof(int), 1, fp) < 1 ) return 1;
    swapper.Swap4BE(&numGrids);
    }
  else
    {
    numGrids = 1;
    }

  if ( numGrids != this->NumGrids )
    {
    vtkErrorMacro(<<"Data mismatch in solution file!");
    return 1;
    }
//
// Loop over dimensions, reading grid dimensions that have been specified
//
  for (gridFound=0, offset=0, maxGridSize=0, i=0; i<numGrids; i++) 
    {
    //read dimensions
    if ( fread (dim, sizeof(int), 3, fp) < 3 ) return 1;
    swapper.Swap4BERange(dim,3);
    gridSize = dim[0] * dim[1] * dim[2];

    if ( i < this->GridNumber ) 
      offset += 5*gridSize;
    else if ( i == this->GridNumber ) 
      {
      gridFound = 1;
      numPts = gridSize;
      output->SetDimensions(dim);
      }
    }

  if ( ! gridFound )
    {
    vtkErrorMacro (<<"Specified grid not found!");
    return 1;
    }
    
  if ( numPts != this->NumPts )
    {
    vtkErrorMacro (<<"Data mismatch in solution file!");
    delete [] this->TempStorage;
    return 1;
    }
    
  //seek to correct spot and read solution
  fseek (fp, offset*sizeof(float), 1);

  //read solution parameters
  if ( fread (params, sizeof(float), 4, fp) < 4 ) return 1;
  swapper.Swap4BERange(params,4);
  this->Fsmach = params[0];
  this->Alpha = params[1];
  this->Re = params[2];
  this->Time = params[3];

  //allocate temporary storage to copy density data into
  newDensity = new vtkFloatScalars(numPts);
  newEnergy = new vtkFloatScalars(numPts);
  newMomentum = new vtkFloatVectors(numPts);

  if (fread(this->TempStorage, sizeof(float), numPts, fp) < 
      (unsigned int)numPts ) 
    {
    newDensity->Delete();
    newMomentum->Delete();
    newEnergy->Delete();
    delete [] this->TempStorage;
    return 1;
    }
  else //successful read
    {
    swapper.Swap4BERange(this->TempStorage,numPts);
    for (i=0; i < this->NumPts; i++) 
      newDensity->SetScalar(i,this->TempStorage[i]);
    }

  if (fread(this->TempStorage, sizeof(float), 3*this->NumPts, fp) < 
      (unsigned int)3*this->NumPts ) 
    {
    newDensity->Delete();
    newMomentum->Delete();
    newEnergy->Delete();
    delete [] this->TempStorage;
    return 1;
    }
  else //successful read, load coordinates into vector object
    {
    swapper.Swap4BERange(this->TempStorage,3*this->NumPts);
    for (i=0; i < this->NumPts; i++)
      {
      m[0] = this->TempStorage[i];
      m[1] = this->TempStorage[this->NumPts+i];
      m[2] = this->TempStorage[2*this->NumPts+i];
      newMomentum->SetVector(i,m);
      }
    }

  if (fread(this->TempStorage, sizeof(float), numPts, fp) < 
      (unsigned int)numPts) 
    {
    newDensity->Delete();
    newMomentum->Delete();
    newEnergy->Delete();
    delete [] this->TempStorage;
    return 1;
    }
  else //successful read
    {
    swapper.Swap4BERange(this->TempStorage,numPts);
    for (i=0; i < this->NumPts; i++) 
      newEnergy->SetScalar(i,this->TempStorage[i]);
    }
//
// Register data for use by computation functions
//
  this->Density = newDensity;
  this->Density->Register(this);
  newDensity->Delete();

  this->Momentum = newMomentum;
  this->Momentum->Register(this);
  newMomentum->Delete();

  this->Energy = newEnergy;
  this->Energy->Register(this);
  newEnergy->Delete();

  return 0;
}

//
// Various PLOT3D functions.....................
//
void vtkPLOT3DReader::MapFunction(int fNumber,vtkPointData *outputPD)
{
  switch (fNumber)
    {
    case -1: //empty mapping
      break;

    case 100: //Density
      this->ComputeDensity(outputPD);
      break;

    case 110: //Pressure
      this->ComputePressure(outputPD);
      break;

    case 120: //Temperature
      this->ComputeTemperature(outputPD);
      break;

    case 130: //Enthalpy
      this->ComputeEnthalpy(outputPD);
      break;

    case 140: //Internal Energy
      this->ComputeInternalEnergy(outputPD);
      break;

    case 144: //Kinetic Energy
      this->ComputeKineticEnergy(outputPD);
      break;

    case 153: //Velocity Magnitude
      this->ComputeVelocityMagnitude(outputPD);
      break;

    case 163: //Stagnation energy
      this->ComputeStagnationEnergy(outputPD);
      break;

    case 170: //Entropy
      this->ComputeEntropy(outputPD);
      break;

    case 184: //Swirl
      this->ComputeSwirl(outputPD);
      break;

    case 200: //Velocity
      this->ComputeVelocity(outputPD);
      break;

    case 201: //Vorticity
      this->ComputeVorticity(outputPD);
      break;

    case 202: //Momentum
      this->ComputeMomentum(outputPD);
      break;

    case 210: //PressureGradient
      this->ComputePressureGradient(outputPD);
      break;

    default:
      vtkErrorMacro(<<"No function number " << fNumber);
    }
}

void vtkPLOT3DReader::ComputeDensity(vtkPointData *outputPD)
{
  outputPD->SetScalars(this->Density);
  vtkDebugMacro(<<"Created density scalar");
}

void vtkPLOT3DReader::ComputeTemperature(vtkPointData *outputPD)
{
  float *m, e, rr, u, v, w, v2, p, d, rrgas;
  int i;
  vtkFloatScalars *temperature;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL || 
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute temperature");
    return;
    }

  temperature = new vtkFloatScalars(this->NumPts);
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
  outputPD->SetScalars(temperature);
  vtkDebugMacro(<<"Created temperature scalar");
}

void vtkPLOT3DReader::ComputePressure(vtkPointData *outputPD)
{
  float *m, e, u, v, w, v2, p, d, rr;
  int i;
  vtkFloatScalars *pressure;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL || 
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute pressure");
    return;
    }

  pressure = new vtkFloatScalars(this->NumPts);
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
  outputPD->SetScalars(pressure);
  vtkDebugMacro(<<"Created pressure scalar");
}

void vtkPLOT3DReader::ComputeEnthalpy(vtkPointData *outputPD)
{
  float *m, e, u, v, w, v2, d, rr;
  int i;
  vtkFloatScalars *enthalpy;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL || 
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute enthalpy");
    return;
    }

  enthalpy = new vtkFloatScalars(this->NumPts);
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
  outputPD->SetScalars(enthalpy);
  vtkDebugMacro(<<"Created enthalpy scalar");
}

void vtkPLOT3DReader::ComputeInternalEnergy(vtkPointData *outputPD)
{
  outputPD->SetScalars(this->Energy);
  vtkDebugMacro(<<"Created energy scalar");
}

void vtkPLOT3DReader::ComputeKineticEnergy(vtkPointData *outputPD)
{
  float *m, u, v, w, v2, d, rr;
  int i;
  vtkFloatScalars *kineticEnergy;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL )
    {
    vtkErrorMacro(<<"Cannot compute kinetic energy");
    return;
    }

  kineticEnergy = new vtkFloatScalars(this->NumPts);
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
  outputPD->SetScalars(kineticEnergy);
  vtkDebugMacro(<<"Created kinetic energy scalar");
}

void vtkPLOT3DReader::ComputeVelocityMagnitude(vtkPointData *outputPD)
{
  float *m, u, v, w, v2, d, rr, e;
  int i;
  vtkFloatScalars *velocityMag;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute velocity magnitude");
    return;
    }

  velocityMag = new vtkFloatScalars(this->NumPts);
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
  outputPD->SetScalars(velocityMag);
  vtkDebugMacro(<<"Created velocity magnitude scalar");
}

void vtkPLOT3DReader::ComputeStagnationEnergy(vtkPointData *outputPD)
{
  outputPD->SetScalars(this->Energy);
  vtkDebugMacro(<<"Created stagnation energy scalar");
}

void vtkPLOT3DReader::ComputeEntropy(vtkPointData *outputPD)
{
  float *m, u, v, w, v2, d, rr, s, p, e;
  int i;
  vtkFloatScalars *entropy;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute entropy");
    return;
    }

  entropy = new vtkFloatScalars(this->NumPts);
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
  outputPD->SetScalars(entropy);
  vtkDebugMacro(<<"Created entropy scalar");
}

void vtkPLOT3DReader::ComputeSwirl(vtkPointData *outputPD)
{
  vtkVectors *currentVector;
  vtkVectors *vorticity;
  float d, rr, *m, u, v, w, v2, *vort, s;
  int i;
  vtkFloatScalars *swirl;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute swirl");
    return;
    }

  swirl = new vtkFloatScalars(this->NumPts);

  currentVector = outputPD->GetVectors();
  currentVector->Register(this);

  this->ComputeVorticity(outputPD);
  vorticity = outputPD->GetVectors();
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
  outputPD->SetScalars(swirl);
  vtkDebugMacro(<<"Created swirl scalar");

  // reset current vector
  outputPD->SetVectors(currentVector);
  currentVector->UnRegister(this);
}

// Vector functions
void vtkPLOT3DReader::ComputeVelocity(vtkPointData *outputPD)
{
  float *m, v[3], d, rr;
  int i;
  vtkFloatVectors *velocity;
//
//  Check that the required data is available
//
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute velocity");
    return;
    }

  velocity = new vtkFloatVectors(this->NumPts);
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
  outputPD->SetVectors(velocity);
  vtkDebugMacro(<<"Created velocity vector");
}

void vtkPLOT3DReader::ComputeVorticity(vtkPointData *outputPD)
{
  vtkVectors *velocity;
  vtkFloatVectors *vorticity;
  int dims[3], ijsize;
  vtkPoints *points;
  int i, j, k, idx, idx2;
  float vort[3], xp[3], xm[3], vp[3], vm[3], factor;
  float xxi, yxi, zxi, uxi, vxi, wxi;
  float xeta, yeta, zeta, ueta, veta, weta;
  float xzeta, yzeta, zzeta, uzeta, vzeta, wzeta;
  float aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;
//
//  Check that the required data is available
//
  if ( (points=this->GetOutput()->GetPoints()) == NULL || this->Density == NULL || 
  this->Momentum == NULL || this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute vorticity");
    return;
    }

  vorticity = new vtkFloatVectors(this->NumPts);

  this->ComputeVelocity(outputPD);
  velocity = outputPD->GetVectors();

  this->GetOutput()->GetDimensions(dims);
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

  outputPD->SetVectors(vorticity);
  vtkDebugMacro(<<"Created vorticity vector");
}

void vtkPLOT3DReader::ComputeMomentum(vtkPointData *outputPD)
{
  outputPD->SetVectors(this->Momentum);
  vtkDebugMacro(<<"Created momentum vector");
}

void vtkPLOT3DReader::ComputePressureGradient(vtkPointData *outputPD)
{
  vtkScalars *currentScalar;
  vtkScalars *pressure;
  vtkFloatVectors *gradient;
  int dims[3], ijsize;
  vtkPoints *points;
  int i, j, k, idx, idx2;
  float g[3], xp[3], xm[3], pp, pm, factor;
  float xxi, yxi, zxi, pxi;
  float xeta, yeta, zeta, peta;
  float xzeta, yzeta, zzeta, pzeta;
  float aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;
//
//  Check that the required data is available
//
  if ( (points=this->GetOutput()->GetPoints()) == NULL || this->Density == NULL || 
  this->Momentum == NULL || this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute pressure gradient");
    return;
    }

  gradient = new vtkFloatVectors(this->NumPts);

  currentScalar = outputPD->GetScalars();
  currentScalar->Register(this);

  this->ComputePressure(outputPD);
  pressure = outputPD->GetScalars();

  this->GetOutput()->GetDimensions(dims);
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

  outputPD->SetVectors(gradient);
  vtkDebugMacro(<<"Created pressure gradient vector");

  // reset current scalar
  outputPD->SetScalars(currentScalar);
  currentScalar->UnRegister(this);
}

int vtkPLOT3DReader::GetFileType(FILE *fp)
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

void vtkPLOT3DReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredGridSource::PrintSelf(os,indent);

  os << indent << "XYZ Filename: " << 
    (this->XYZFilename ? this->XYZFilename : "(none)") << "\n";
  os << indent << "Q Filename: " <<
    (this->QFilename ? this->QFilename : "(none)") << "\n";
  os << indent << "Function Filename: " << 
    (this->FunctionFilename ? this->FunctionFilename : "(none)") << "\n";

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

