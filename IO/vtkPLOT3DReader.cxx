/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLOT3DReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <ctype.h>
#include <math.h>
#include "vtkPLOT3DReader.h"
#include "vtkByteSwap.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

//-------------------------------------------------------------------------
vtkPLOT3DReader* vtkPLOT3DReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPLOT3DReader");
  if(ret)
    {
    return (vtkPLOT3DReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPLOT3DReader;
}

#define VTK_BINARY 0
#define VTK_ASCII 1

#define VTK_RHOINF 1.0
#define VTK_CINF 1.0
#define VTK_PINF ((VTK_RHOINF*VTK_CINF) * (VTK_RHOINF*VTK_CINF) / this->Gamma)
#define VTK_CV (this->R / (this->Gamma-1.0))

vtkPLOT3DReader::vtkPLOT3DReader()
{
  this->FileFormat = VTK_WHOLE_SINGLE_GRID_NO_IBLANKING;

  this->XYZFileName = NULL;
  this->QFileName = NULL;
  this->FunctionFileName = NULL;
  this->VectorFunctionFileName = NULL;

  this->FunctionList = vtkIntArray::New();

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

  this->NumberOfGrids = 0;
} 

vtkPLOT3DReader::~vtkPLOT3DReader()
{
  if ( this->XYZFileName )
    {
    delete [] this->XYZFileName;
    }
  if ( this->QFileName )
    {
    delete [] this->QFileName;
    }
  if ( this->FunctionFileName )
    {
    delete [] this->FunctionFileName;
    }
  if ( this->VectorFunctionFileName )
    {
    delete [] this->VectorFunctionFileName;
    }
  this->FunctionList->Delete();
}

void vtkPLOT3DReader::RemoveFunction(int fnum)
{
  for (int i=0; i < this->FunctionList->GetNumberOfTuples(); i++ )
    {
    if ( this->FunctionList->GetValue(i) == fnum )
      {
      this->FunctionList->SetValue(i,-1);
      }
    }
}

void vtkPLOT3DReader::ExecuteInformation()
{
  FILE *xyzFp;
  int error = 0;
  vtkStructuredGrid *output = this->GetOutput();

  // must go through all the same checks as actual read.
  if ( this->XYZFileName == NULL )
    {
    vtkErrorMacro(<< "Must specify geometry file");
    return;
    }
  if ( (xyzFp = fopen(this->XYZFileName, "r")) == NULL)
    {
    vtkErrorMacro(<< "File: " << this->XYZFileName << " not found");
    return;
    }
  if ( this->GetFileType(xyzFp) == VTK_ASCII )
    {
    vtkWarningMacro("reading ascii grid files currently not supported");
    // error = this->ReadASCIIGrid(xyzFp);
    }
  else
    {
    fclose(xyzFp);
    xyzFp = fopen(this->XYZFileName, "rb");
    // reads the whole extent
    error = this->ReadBinaryGridDimensions(xyzFp,output);
    fclose(xyzFp);
    }
  
  if ( error )
    {
    vtkErrorMacro(<<"Error reading XYZ file");
    return;
    }
}

void vtkPLOT3DReader::Execute()
{
  FILE *xyzFp, *QFp, *funcFp;
  int error = 0;
  vtkStructuredGrid *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  // Initialize output and read geometry
  //
  if ( this->XYZFileName == NULL )
    {
    output->Initialize();
    vtkErrorMacro(<< "Must specify geometry file");
    return;
    }
  if ( (xyzFp = fopen(this->XYZFileName, "r")) == NULL)
    {
    output->Initialize();
    vtkErrorMacro(<< "File: " << this->XYZFileName << " not found");
    return;
    }
  if ( this->GetFileType(xyzFp) == VTK_ASCII )
    {
    vtkWarningMacro("reading ascii grid files currently not supported");
    // error = this->ReadASCIIGrid(xyzFp);
    }
  else
    {
    fclose(xyzFp);
    xyzFp = fopen(this->XYZFileName, "rb");
    error = this->ReadBinaryGrid(xyzFp,output);
    fclose(xyzFp);
    }
  
  if ( error )
    {
    output->Initialize();
    vtkErrorMacro(<<"Error reading XYZ file");
    return;
    }

  // Read solution file (if available and requested)
  //
  if ( this->QFileName && 
  ((this->FunctionFileName == NULL && this->ScalarFunctionNumber >= 0) ||
  (this->VectorFunctionFileName == NULL && this->VectorFunctionNumber >= 0)) )
    {
    if ( (QFp = fopen(this->QFileName, "r")) == NULL)
      {
      output->Initialize();
      vtkErrorMacro(<< "File: " << this->QFileName << " not found");
      return;
      }

    if ( this->GetFileType(QFp) == VTK_ASCII )
      {
      vtkWarningMacro("reading ascii solution files currently not supported");
      // error = this->ReadASCIISolution(QFp);
      }
    else
      {
      fclose(QFp);
      QFp = fopen(this->QFileName, "rb");
      error = this->ReadBinarySolution(QFp,output);
      fclose(QFp);
      }
    
    if ( error )
      {
      output->Initialize();
      vtkErrorMacro(<<"Error reading solution file");
      return;
      }

    // Read solutions as general point attribute data
    if ( this->FunctionList->GetNumberOfTuples() > 0 )
      {
      int fnum;
      for (int i=0; i < this->FunctionList->GetNumberOfTuples(); i++)
        {
        if ( (fnum=this->FunctionList->GetValue(i)) >= 0 )
          {
          this->MapFunction(fnum,outputPD);
          }
        }
      }

    this->MapFunction(this->ScalarFunctionNumber,outputPD);
    this->MapFunction(this->VectorFunctionNumber,outputPD);
    }

  // Read function file (if available)
  //
  if ( this->FunctionFileName != NULL)
    {
    if ( (funcFp = fopen(this->FunctionFileName, "r")) == NULL)
      {
      output->Initialize();
      vtkErrorMacro(<< "File: " << this->FunctionFileName << " not found");
      return;
      }

    if ( this->GetFileType(funcFp) == VTK_ASCII )
      {
      vtkWarningMacro("reading ASCII function files currently not supported");
      // error = this->ReadASCIIFunctionFile(funcFp);
      }
    else
      {
      fclose(funcFp);
      funcFp = fopen(this->FunctionFileName, "rb");
      error = this->ReadBinaryFunctionFile(funcFp,output);
      fclose(funcFp);
      }
    
    if ( error )
      {
      vtkErrorMacro(<<"Error reading function file");
      return;
      }
    }

  // Read vector function file (if available)
  //
  if ( this->VectorFunctionFileName != NULL )
    {
    if ( (funcFp = fopen(this->VectorFunctionFileName, "r")) == NULL)
      {
      output->Initialize();
      vtkErrorMacro(<< "File: " << this->VectorFunctionFileName << " not found");
      return;
      }

    if ( this->GetFileType(funcFp) == VTK_ASCII )
      {
      vtkWarningMacro("reading ASCII vector function files currently not supported");
      // error = this->ReadASCIIFunctionFile(funcFp);
      }
    else
      {
      fclose(funcFp);
      funcFp = fopen(this->VectorFunctionFileName, "rb");
      error = this->ReadBinaryVectorFunctionFile(funcFp,output);
      fclose(funcFp);
      }
    
    if ( error )
      {
      output->Initialize();
      vtkErrorMacro(<<"Error reading vector function file");
      return;
      }
    }

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
  vtkPoints *newPts;
  int dim[3];
  int i, gridFound, offset, gridSize;
  float x[3];
  
  if ( this->FileFormat == VTK_WHOLE_MULTI_GRID_NO_IBLANKING )
    {
    if (fread(&(this->NumberOfGrids), sizeof(int), 1, fp) < 1 )
      {
      return 1;
      }
    vtkByteSwap::Swap4BE(&(this->NumberOfGrids));
    }
  else
    {
    this->NumberOfGrids = 1;
    }

  // Loop over grids, reading one that has been specified
  //
  for (gridFound=0, offset=0, i=0; i<this->NumberOfGrids; i++) 
    {
    //read dimensions
    if ( fread (dim, sizeof(int), 3, fp) < 3 )
      {
      return 1;
      }
    vtkByteSwap::Swap4BERange(dim,3);
    
    gridSize = dim[0] * dim[1] * dim[2];

    if ( i < this->GridNumber ) 
      {
      offset += 3*gridSize;
      }
    else if ( i == this->GridNumber ) 
      {
      gridFound = 1;
      this->NumberOfPoints = gridSize;
      output->SetDimensions(dim);
      }
    }

  if ( ! gridFound )
    {
    vtkErrorMacro (<<"Specified grid not found!");
    return 1;
    }
    
  //allocate temporary storage to read into + points
  this->TempStorage = new float[3*this->NumberOfPoints];
  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(this->NumberOfPoints);

  //seek to correct spot and read grid
  fseek (fp, (long)(offset*sizeof(float)), 1);

  if ( fread(this->TempStorage, sizeof(float), 3*this->NumberOfPoints, fp) < (unsigned long)3*this->NumberOfPoints ) 
    {
    newPts->Delete();
    delete [] this->TempStorage;
    return 1;
    }
  else //successful read, load coordinates in points object
    {
    vtkByteSwap::Swap4BERange(this->TempStorage,3*this->NumberOfPoints);
    for (i=0; i < this->NumberOfPoints; i++)
      {
      x[0] = this->TempStorage[i];
      x[1] = this->TempStorage[this->NumberOfPoints+i];
      x[2] = this->TempStorage[2*this->NumberOfPoints+i];
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

  vtkDebugMacro(<<"Read " << this->NumberOfPoints << " points");
  return 0;
}

// for UpdateInformation
int vtkPLOT3DReader::ReadBinaryGridDimensions(FILE *fp,
					      vtkStructuredGrid *output)
{
  int dim[3];
  int i, offset, gridSize;
  
  if ( this->FileFormat == VTK_WHOLE_MULTI_GRID_NO_IBLANKING )
    {
    if (fread(&(this->NumberOfGrids), sizeof(int), 1, fp) < 1 )
      {
      return 1;
      }
    vtkByteSwap::Swap4BE(&(this->NumberOfGrids));
    }
  else
    {
    this->NumberOfGrids = 1;
    }
  //
  // Loop over grids, reading one that has been specified
  //
  for (offset=0, i=0; i<this->NumberOfGrids; i++) 
    {
    //read dimensions
    if ( fread (dim, sizeof(int), 3, fp) < 3 )
      {
      return 1;
      }
    vtkByteSwap::Swap4BERange(dim,3);
    
    gridSize = dim[0] * dim[1] * dim[2];

    if ( i < this->GridNumber ) 
      {
      offset += 3*gridSize;
      }
    else if ( i == this->GridNumber ) 
      {
      this->NumberOfPoints = gridSize;
      output->SetWholeExtent(0,dim[0]-1, 0,dim[1]-1, 0,dim[2]-1);
      return 0;
      }
    }
  // could not find grid
  return 1;
}

int vtkPLOT3DReader::ReadBinarySolution(FILE *fp,vtkStructuredGrid *output)
{
  vtkFloatArray *newDensity, *newEnergy;
  vtkFloatArray *newMomentum;
  int dim[3];
  int i, gridFound, offset, gridSize;
  float m[3], params[4];
  int numGrids, numPts = 0;

  if ( this->FileFormat == VTK_WHOLE_MULTI_GRID_NO_IBLANKING )
    {
    if ( fread (&this->NumberOfGrids, sizeof(int), 1, fp) < 1 )
      {
      return 1;
      }
    vtkByteSwap::Swap4BE(&numGrids);
    }
  else
    {
    numGrids = 1;
    }

  if ( numGrids != this->NumberOfGrids )
    {
    vtkErrorMacro(<<"Data mismatch in solution file!");
    return 1;
    }
  //
  // Loop over dimensions, reading grid dimensions that have been specified
  //
  for (gridFound=0, offset=0, i=0; i<numGrids; i++) 
    {
    //read dimensions
    if ( fread (dim, sizeof(int), 3, fp) < 3 )
      {
      return 1;
      }
    vtkByteSwap::Swap4BERange(dim,3);
    gridSize = dim[0] * dim[1] * dim[2];

    if ( i < this->GridNumber ) 
      {
      offset += 4; // skip condition values for grid, fix from Tom Johnson
      offset += 5*gridSize;
      }
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
    
  if ( numPts != this->NumberOfPoints )
    {
    vtkErrorMacro (<<"Data mismatch in solution file!");
    delete [] this->TempStorage;
    return 1;
    }
    
  //seek to correct spot and read solution
  fseek (fp, (long)(offset*sizeof(float)), 1);

  //read solution parameters
  if ( fread (params, sizeof(float), 4, fp) < 4 )
    {
    return 1;
    }
  vtkByteSwap::Swap4BERange(params,4);
  this->Fsmach = params[0];
  this->Alpha = params[1];
  this->Re = params[2];
  this->Time = params[3];

  //allocate temporary storage to copy density data into
  newDensity = vtkFloatArray::New();
  newDensity->SetNumberOfTuples(numPts);
  newDensity->SetName("Density");
  newEnergy = vtkFloatArray::New();
  newEnergy->SetNumberOfTuples(numPts);
  newEnergy->SetName("Energy");
  newMomentum = vtkFloatArray::New();
  newMomentum->SetNumberOfComponents(3);
  newMomentum->SetNumberOfTuples(numPts);
  newMomentum->SetName("Momentum");

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
    vtkByteSwap::Swap4BERange(this->TempStorage,numPts);
    for (i=0; i < this->NumberOfPoints; i++) 
      {
      newDensity->SetValue(i,this->TempStorage[i]);
      }
    }

  if (fread(this->TempStorage, sizeof(float), 3*this->NumberOfPoints, fp) < 
      (unsigned int)3*this->NumberOfPoints ) 
    {
    newDensity->Delete();
    newMomentum->Delete();
    newEnergy->Delete();
    delete [] this->TempStorage;
    return 1;
    }
  else //successful read, load coordinates into vector object
    {
    vtkByteSwap::Swap4BERange(this->TempStorage,3*this->NumberOfPoints);
    for (i=0; i < this->NumberOfPoints; i++)
      {
      m[0] = this->TempStorage[i];
      m[1] = this->TempStorage[this->NumberOfPoints+i];
      m[2] = this->TempStorage[2*this->NumberOfPoints+i];
      newMomentum->SetTuple(i,m);
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
    vtkByteSwap::Swap4BERange(this->TempStorage,numPts);
    for (i=0; i < this->NumberOfPoints; i++) 
      {
      newEnergy->SetValue(i,this->TempStorage[i]);
      }
    }

  // Register data for use by computation functions
  //
  this->Density = newDensity;
  this->Density->SetName("Density");
  this->Density->Register(this);
  newDensity->Delete();

  this->Momentum = newMomentum;
  this->Momentum->SetName("Momentum");
  this->Momentum->Register(this);
  newMomentum->Delete();

  this->Energy = newEnergy;
  this->Energy->SetName("Energy");
  this->Energy->Register(this);
  newEnergy->Delete();

  return 0;
}

int vtkPLOT3DReader::ReadBinaryFunctionFile(FILE *fp,vtkStructuredGrid *output)
{
  int numGrids;

  output = output;
  if ( this->FileFormat == VTK_WHOLE_MULTI_GRID_NO_IBLANKING )
    {
    if ( fread (&numGrids, sizeof(int), 1, fp) < 1 )
      {
      return 1;
      }
    vtkByteSwap::Swap4BE(&numGrids);
    }
  else
    {
    numGrids = 1;
    }

  if ( numGrids != this->NumberOfGrids )
    {
    vtkErrorMacro(<<"Data mismatch in function file!");
    return 1;
    }

  return 0;
}

int vtkPLOT3DReader::ReadBinaryVectorFunctionFile(FILE *fp,vtkStructuredGrid *output)
{
  int numGrids;

  output = output;
  if ( this->FileFormat == VTK_WHOLE_MULTI_GRID_NO_IBLANKING )
    {
    if ( fread (&numGrids, sizeof(int), 1, fp) < 1 )
      {
      return 1;
      }
    vtkByteSwap::Swap4BE(&numGrids);
    }
  else
    {
    numGrids = 1;
    }

  if ( numGrids != this->NumberOfGrids )
    {
    vtkErrorMacro(<<"Data mismatch in vector function file!");
    return 1;
    }

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
  outputPD->AddArray(this->Density);
  outputPD->SetActiveScalars("Density");
  vtkDebugMacro(<<"Created density scalar");
}

void vtkPLOT3DReader::ComputeTemperature(vtkPointData *outputPD)
{
  float *m, e, rr, u, v, w, v2, p, d, rrgas;
  int i;
  vtkFloatArray *temperature;

  //  Check that the required data is available
  //
  if ( this->Density == NULL || this->Momentum == NULL || 
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute temperature");
    return;
    }

  temperature = vtkFloatArray::New();
  temperature->SetNumberOfTuples(this->NumberOfPoints);

  //  Compute the temperature
  //
  rrgas = 1.0 / this->R;
  for (i=0; i < this->NumberOfPoints; i++) 
    {
    d = this->Density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetTuple(i);
    e = this->Energy->GetComponent(i,0);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    p = (this->Gamma-1.) * (e - 0.5 * d * v2);
    temperature->SetValue(i, p*rr*rrgas);
  }

  temperature->SetName("Temperature");
  outputPD->AddArray(temperature);
  outputPD->SetActiveScalars("Temperature");
  
  temperature->Delete();
  vtkDebugMacro(<<"Created temperature scalar");
}

void vtkPLOT3DReader::ComputePressure(vtkPointData *outputPD)
{
  float *m, e, u, v, w, v2, p, d, rr;
  int i;
  vtkFloatArray *pressure;

  //  Check that the required data is available
  //
  if ( this->Density == NULL || this->Momentum == NULL || 
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute pressure");
    return;
    }

  pressure = vtkFloatArray::New();
  pressure->SetNumberOfTuples(this->NumberOfPoints);

  //  Compute the pressure
  //
  for (i=0; i < this->NumberOfPoints; i++) 
    {
    d = this->Density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetTuple(i);
    e = this->Energy->GetComponent(i,0);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    p = (this->Gamma-1.) * (e - 0.5 * d * v2);
    pressure->SetValue(i, p);
  }

  pressure->SetName("Pressure");
  outputPD->AddArray(pressure);
  outputPD->SetActiveScalars("Pressure");
  pressure->Delete();
  vtkDebugMacro(<<"Created pressure scalar");
}

void vtkPLOT3DReader::ComputeEnthalpy(vtkPointData *outputPD)
{
  float *m, e, u, v, w, v2, d, rr;
  int i;
  vtkFloatArray *enthalpy;

  //  Check that the required data is available
  //
  if ( this->Density == NULL || this->Momentum == NULL || 
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute enthalpy");
    return;
    }

  enthalpy = vtkFloatArray::New();
  enthalpy->SetNumberOfTuples(this->NumberOfPoints);

  //  Compute the enthalpy
  //
  for (i=0; i < this->NumberOfPoints; i++) 
    {
    d = this->Density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetTuple(i);
    e = this->Energy->GetComponent(i,0);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    enthalpy->SetValue(i, this->Gamma*(e*rr - 0.5*v2));
  }
  enthalpy->SetName("Enthalpy");
  outputPD->AddArray(enthalpy);
  outputPD->SetActiveScalars("Enthalpy");
  enthalpy->Delete();
  vtkDebugMacro(<<"Created enthalpy scalar");
}

void vtkPLOT3DReader::ComputeInternalEnergy(vtkPointData *outputPD)
{
  outputPD->AddArray(this->Energy);
  outputPD->SetActiveScalars("Energy");

  vtkDebugMacro(<<"Created energy scalar");
}

void vtkPLOT3DReader::ComputeKineticEnergy(vtkPointData *outputPD)
{
  float *m, u, v, w, v2, d, rr;
  int i;
  vtkFloatArray *kineticEnergy;

  //  Check that the required data is available
  //
  if ( this->Density == NULL || this->Momentum == NULL )
    {
    vtkErrorMacro(<<"Cannot compute kinetic energy");
    return;
    }

  kineticEnergy = vtkFloatArray::New();
  kineticEnergy->SetNumberOfTuples(this->NumberOfPoints);

  //  Compute the kinetic energy
  //
  for (i=0; i < this->NumberOfPoints; i++) 
    {
    d = this->Density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetTuple(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    kineticEnergy->SetValue(i, 0.5*v2);
  }
  kineticEnergy->SetName("Kinetic Energy");
  outputPD->AddArray(kineticEnergy);
  outputPD->SetActiveScalars("Kinetic Energy");
  kineticEnergy->Delete();
  vtkDebugMacro(<<"Created kinetic energy scalar");
}

void vtkPLOT3DReader::ComputeVelocityMagnitude(vtkPointData *outputPD)
{
  float *m, u, v, w, v2, d, rr;
  int i;
  vtkFloatArray *velocityMag;

  //  Check that the required data is available
  //
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute velocity magnitude");
    return;
    }

  velocityMag = vtkFloatArray::New();
  velocityMag->SetNumberOfTuples(this->NumberOfPoints);

  //  Compute the velocity magnitude
  //
  for (i=0; i < this->NumberOfPoints; i++) 
    {
    d = this->Density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetTuple(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    velocityMag->SetValue(i, sqrt((double)v2));
  }
  velocityMag->SetName("Velocity Magnitude");
  outputPD->AddArray(velocityMag);
  outputPD->SetActiveScalars("Velocity Magnitude");
  velocityMag->Delete();
  vtkDebugMacro(<<"Created velocity magnitude scalar");
}

void vtkPLOT3DReader::ComputeStagnationEnergy(vtkPointData *outputPD)
{
  outputPD->AddArray(this->Energy);
  outputPD->SetActiveScalars("Energy");
  
  vtkDebugMacro(<<"Created stagnation energy scalar");
}

void vtkPLOT3DReader::ComputeEntropy(vtkPointData *outputPD)
{
  float *m, u, v, w, v2, d, rr, s, p, e;
  int i;
  vtkFloatArray *entropy;

  //  Check that the required data is available
  //
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute entropy");
    return;
    }

  entropy = vtkFloatArray::New();
  entropy->SetNumberOfTuples(this->NumberOfPoints);

  //  Compute the entropy
  //
  for (i=0; i < this->NumberOfPoints; i++) 
    {
    d = this->Density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetTuple(i);
    e = this->Energy->GetComponent(i,0);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    p = (this->Gamma-1.)*(e - 0.5*d*v2);
    s = VTK_CV * log((p/VTK_PINF)/pow((double)d/VTK_RHOINF,(double)this->Gamma));
    entropy->SetValue(i,s);
  }
  entropy->SetName("Entropy");
  outputPD->AddArray(entropy);
  outputPD->SetActiveScalars("Entropy");
  entropy->Delete();
  vtkDebugMacro(<<"Created entropy scalar");
}

void vtkPLOT3DReader::ComputeSwirl(vtkPointData *outputPD)
{
  vtkDataArray *currentVector;
  vtkDataArray *vorticity;
  float d, rr, *m, u, v, w, v2, *vort, s;
  int i;
  vtkFloatArray *swirl;

  //  Check that the required data is available
  //
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute swirl");
    return;
    }

  swirl = vtkFloatArray::New();
  swirl->SetNumberOfTuples(this->NumberOfPoints);

  currentVector = outputPD->GetVectors();
  if (currentVector)
    {
    currentVector->Register(this);
    }

  this->ComputeVorticity(outputPD);
  vorticity = outputPD->GetVectors();
//
//  Compute the swirl
//
  for (i=0; i < this->NumberOfPoints; i++) 
    {
    d = this->Density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetTuple(i);
    vort = vorticity->GetTuple(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    if ( v2 != 0.0 ) 
      {
      s = (vort[0]*m[0] + vort[1]*m[1] + vort[2]*m[2]) / v2;
      }
    else 
      {
      s = 0.0;
      }

    swirl->SetValue(i,s);
  }
  swirl->SetName("Swirl");
  outputPD->AddArray(swirl);
  outputPD->SetActiveScalars("Swirl");
  swirl->Delete();
  vtkDebugMacro(<<"Created swirl scalar");

  // reset current vector
  if (currentVector)
    {
    outputPD->SetVectors(currentVector);
    currentVector->UnRegister(this);
    }
}

// Vector functions
void vtkPLOT3DReader::ComputeVelocity(vtkPointData *outputPD)
{
  float *m, v[3], d, rr;
  int i;
  vtkFloatArray *velocity;

  //  Check that the required data is available
  //
  if ( this->Density == NULL || this->Momentum == NULL ||
  this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute velocity");
    return;
    }

  velocity = vtkFloatArray::New();
  velocity->SetNumberOfComponents(3);
  velocity->SetNumberOfTuples(this->NumberOfPoints);

  //  Compute the velocity
  //
  for (i=0; i < this->NumberOfPoints; i++) 
    {
    d = this->Density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = this->Momentum->GetTuple(i);
    rr = 1.0 / d;
    v[0] = m[0] * rr;        
    v[1] = m[1] * rr;        
    v[2] = m[2] * rr;        
    velocity->SetTuple(i, v);
  }
  velocity->SetName("Velocity");
  outputPD->AddArray(velocity);
  outputPD->SetActiveVectors("Velocity");
  velocity->Delete();
  vtkDebugMacro(<<"Created velocity vector");
}

void vtkPLOT3DReader::ComputeVorticity(vtkPointData *outputPD)
{
  vtkDataArray *velocity;
  vtkFloatArray *vorticity;
  int dims[3], ijsize;
  vtkPoints *points;
  int i, j, k, idx, idx2, ii;
  float vort[3], xp[3], xm[3], vp[3], vm[3], factor;
  float xxi, yxi, zxi, uxi, vxi, wxi;
  float xeta, yeta, zeta, ueta, veta, weta;
  float xzeta, yzeta, zzeta, uzeta, vzeta, wzeta;
  float aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;

  //  Check that the required data is available
  //
  if ( (points=this->GetOutput()->GetPoints()) == NULL || 
       this->Density == NULL || this->Momentum == NULL || 
       this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute vorticity");
    return;
    }

  vorticity = vtkFloatArray::New();
  vorticity->SetNumberOfComponents(3);
  vorticity->SetNumberOfTuples(this->NumberOfPoints);

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
        //  Xi derivatives.
        if ( dims[0] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
            {
            vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
            }
          xp[0] = 1.0;
          }
        else if ( i == 0 ) 
          {
          factor = 1.0;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
          } 
        else if ( i == (dims[0]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i-1 + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
          } 
        else 
          {
          factor = 0.5;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = (i-1) + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
          }

        xxi = factor * (xp[0] - xm[0]);
        yxi = factor * (xp[1] - xm[1]);
        zxi = factor * (xp[2] - xm[2]);
        uxi = factor * (vp[0] - vm[0]);
        vxi = factor * (vp[1] - vm[1]);
        wxi = factor * (vp[2] - vm[2]);

        //  Eta derivatives.
        if ( dims[1] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
            {
            vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
            }
          xp[1] = 1.0;
          }
        else if ( j == 0 ) 
          {
          factor = 1.0;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
          } 
        else if ( j == (dims[1]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
          } 
        else 
          {
          factor = 0.5;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
          }


        xeta = factor * (xp[0] - xm[0]);
        yeta = factor * (xp[1] - xm[1]);
        zeta = factor * (xp[2] - xm[2]);
        ueta = factor * (vp[0] - vm[0]);
        veta = factor * (vp[1] - vm[1]);
        weta = factor * (vp[2] - vm[2]);

        //  Zeta derivatives.
        if ( dims[2] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
            {
            vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
            }
          xp[2] = 1.0;
          }
        else if ( k == 0 ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
          } 
        else if ( k == (dims[2]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
          } 
        else 
          {
          factor = 0.5;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
          }

        xzeta = factor * (xp[0] - xm[0]);
        yzeta = factor * (xp[1] - xm[1]);
        zzeta = factor * (xp[2] - xm[2]);
        uzeta = factor * (vp[0] - vm[0]);
        vzeta = factor * (vp[1] - vm[1]);
        wzeta = factor * (vp[2] - vm[2]);

        // Now calculate the Jacobian.  Grids occasionally have
        // singularities, or points where the Jacobian is infinite (the
        // inverse is zero).  For these cases, we'll set the Jacobian to
        // zero, which will result in a zero vorticity.
        //
        aj =  xxi*yeta*zzeta+yxi*zeta*xzeta+zxi*xeta*yzeta
              -zxi*yeta*xzeta-yxi*xeta*zzeta-xxi*zeta*yzeta;
        if (aj != 0.0)
          {
          aj = 1. / aj;
          }

        //  Xi metrics.
        xix  =  aj*(yeta*zzeta-zeta*yzeta);
        xiy  = -aj*(xeta*zzeta-zeta*xzeta);
        xiz  =  aj*(xeta*yzeta-yeta*xzeta);

        //  Eta metrics.
        etax = -aj*(yxi*zzeta-zxi*yzeta);
        etay =  aj*(xxi*zzeta-zxi*xzeta);
        etaz = -aj*(xxi*yzeta-yxi*xzeta);

        //  Zeta metrics.
        zetax=  aj*(yxi*zeta-zxi*yeta);
        zetay= -aj*(xxi*zeta-zxi*xeta);
        zetaz=  aj*(xxi*yeta-yxi*xeta);

        //  Finally, the vorticity components.
        //
        vort[0]= xiy*wxi+etay*weta+zetay*wzeta - xiz*vxi-etaz*veta-zetaz*vzeta;
        vort[1]= xiz*uxi+etaz*ueta+zetaz*uzeta - xix*wxi-etax*weta-zetax*wzeta;
        vort[2]= xix*vxi+etax*veta+zetax*vzeta - xiy*uxi-etay*ueta-zetay*uzeta;
        idx = i + j*dims[0] + k*ijsize;
        vorticity->SetTuple(idx,vort);
        }
      }
    }
  vorticity->SetName("Vorticity");
  outputPD->AddArray(vorticity);
  outputPD->SetActiveVectors("Vorticity");
  vorticity->Delete();
  vtkDebugMacro(<<"Created vorticity vector");
}

void vtkPLOT3DReader::ComputeMomentum(vtkPointData *outputPD)
{
  this->Momentum->SetName("Momentum");
  outputPD->SetVectors(this->Momentum);
  vtkDebugMacro(<<"Created momentum vector");
}

void vtkPLOT3DReader::ComputePressureGradient(vtkPointData *outputPD)
{
  vtkDataArray *currentScalar;
  vtkDataArray *pressure;
  vtkFloatArray *gradient;
  int dims[3], ijsize;
  vtkPoints *points;
  int i, j, k, idx, idx2, ii;
  float g[3], xp[3], xm[3], pp, pm, factor;
  float xxi, yxi, zxi, pxi;
  float xeta, yeta, zeta, peta;
  float xzeta, yzeta, zzeta, pzeta;
  float aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;

  //  Check that the required data is available
  //
  if ( (points=this->GetOutput()->GetPoints()) == NULL || 
       this->Density == NULL || this->Momentum == NULL || 
       this->Energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute pressure gradient");
    return;
    }

  gradient = vtkFloatArray::New();
  gradient->SetNumberOfComponents(3);
  gradient->SetNumberOfTuples(this->NumberOfPoints);

  currentScalar = outputPD->GetScalars();
  if (currentScalar)
    {
    currentScalar->Register(this);
    }
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
        //  Xi derivatives.
        if ( dims[0] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
	    {
	    xp[ii] = xm[ii] = 0.0;
	    }
          xp[0] = 1.0; pp = pm = 0.0;
          }
        else if ( i == 0 ) 
          {
          factor = 1.0;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
          } 
        else if ( i == (dims[0]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i-1 + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
          } 
        else 
          {
          factor = 0.5;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = (i-1) + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
          }

        xxi = factor * (xp[0] - xm[0]);
        yxi = factor * (xp[1] - xm[1]);
        zxi = factor * (xp[2] - xm[2]);
        pxi = factor * (pp - pm);

        //  Eta derivatives.
        if ( dims[1] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
	    {
	    xp[ii] = xm[ii] = 0.0;
	    }
          xp[1] = 1.0; pp = pm = 0.0;
          }
        else if ( j == 0 ) 
          {
          factor = 1.0;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
          } 
        else if ( j == (dims[1]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
          } 
        else 
          {
          factor = 0.5;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
          }

        xeta = factor * (xp[0] - xm[0]);
        yeta = factor * (xp[1] - xm[1]);
        zeta = factor * (xp[2] - xm[2]);
        peta = factor * (pp - pm);

        //  Zeta derivatives.
        if ( dims[2] == 1 ) // 2D in this direction
          {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
	    {
	    xp[ii] = xm[ii] = 0.0;
	    }
          xp[2] = 1.0; pp = pm = 0.0;
          }
        else if ( k == 0 ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
          } 
        else if ( k == (dims[2]-1) ) 
          {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
          } 
        else 
          {
          factor = 0.5;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
          }

        xzeta = factor * (xp[0] - xm[0]);
        yzeta = factor * (xp[1] - xm[1]);
        zzeta = factor * (xp[2] - xm[2]);
        pzeta = factor * (pp - pm);

        //  Now calculate the Jacobian.  Grids occasionally have
        //  singularities, or points where the Jacobian is infinite (the
        //  inverse is zero).  For these cases, we'll set the Jacobian to
        //  zero, which will result in a zero vorticity.
        //
        aj =  xxi*yeta*zzeta+yxi*zeta*xzeta+zxi*xeta*yzeta
              -zxi*yeta*xzeta-yxi*xeta*zzeta-xxi*zeta*yzeta;
        if (aj != 0.0)
	  {
	  aj = 1. / aj;
	  }

        //  Xi metrics.
        xix  =  aj*(yeta*zzeta-zeta*yzeta);
        xiy  = -aj*(xeta*zzeta-zeta*xzeta);
        xiz  =  aj*(xeta*yzeta-yeta*xzeta);

        //  Eta metrics.
        etax = -aj*(yxi*zzeta-zxi*yzeta);
        etay =  aj*(xxi*zzeta-zxi*xzeta);
        etaz = -aj*(xxi*yzeta-yxi*xzeta);

        //  Zeta metrics.
        zetax=  aj*(yxi*zeta-zxi*yeta);
        zetay= -aj*(xxi*zeta-zxi*xeta);
        zetaz=  aj*(xxi*yeta-yxi*xeta);

        //  Finally, the vorticity components.
        g[0]= xix*pxi+etax*peta+zetax*pzeta;
        g[1]= xiy*pxi+etay*peta+zetay*pzeta;
        g[2]= xiz*pxi+etaz*peta+zetaz*pzeta;

        idx = i + j*dims[0] + k*ijsize;
        gradient->SetTuple(idx,g);
        }
      }
    }
  gradient->SetName("Pressure Gradient");
  outputPD->AddArray(gradient);
  outputPD->SetActiveVectors("Pressure Gradient");
  gradient->Delete();
  vtkDebugMacro(<<"Created pressure gradient vector");

  // reset current scalar
  if (currentScalar)
    {
    outputPD->SetScalars(currentScalar);
    currentScalar->UnRegister(this);
    }
}

int vtkPLOT3DReader::GetFileType(FILE *fp)
{
  char fourBytes[4];
  int type, i;

  //  Read a little from the file to figure what type it is.
  //
  fgets (fourBytes, 4, fp);
  for (i=0, type=VTK_ASCII; i<4 && type == VTK_ASCII; i++)
    {
    if ( ! isprint(fourBytes[i]) )
      {
      type = VTK_BINARY;
      }
    }

  // Reset file for reading
  //
  rewind (fp);
  return type;
}

void vtkPLOT3DReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredGridSource::PrintSelf(os,indent);

  os << indent << "XYZ File Name: " << 
    (this->XYZFileName ? this->XYZFileName : "(none)") << "\n";
  os << indent << "Q File Name: " <<
    (this->QFileName ? this->QFileName : "(none)") << "\n";
  os << indent << "Function File Name: " << 
    (this->FunctionFileName ? this->FunctionFileName : "(none)") << "\n";

  os << indent << "File Format: " << this->FileFormat << "\n";

  os << indent << "Grid Number: " << this->GridNumber << "\n";
  os << indent << "Scalar Function Number: " 
     << this->ScalarFunctionNumber << "\n";

  if ( this->VectorFunctionFileName )
    {
    os << indent << "Vector Function Filename: " <<
          this->VectorFunctionFileName << "\n";
    }
  else
    {
    os << indent << "Vector Function Filename: (none)\n";
    }

  os << indent << "Vector Function Number: " 
     << this->VectorFunctionNumber << "\n";
  os << indent << "Function Number: " 
     << this->FunctionFileFunctionNumber << "\n";

  os << indent << "Free Stream Mach Number: " << this->Fsmach << "\n";
  os << indent << "Alpha: " << this->Alpha << "\n";
  os << indent << "Reynolds Number " << this->Re << "\n";
  os << indent << "Total Integration Time: " << this->Time << "\n";

  os << indent << "R: " << this->R << "\n";
  os << indent << "Gamma: " << this->Gamma << "\n";
  os << indent << "UVinf: " << this->Uvinf << "\n";
  os << indent << "VVinf: " << this->Vvinf << "\n";
  os << indent << "WVinf: " << this->Wvinf << "\n";

  os << indent << "Number Of Grids: " << this->NumberOfGrids << "\n";
}

