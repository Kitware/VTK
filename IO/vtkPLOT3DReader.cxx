/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLOT3DReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPLOT3DReader.h"

#include "vtkByteSwap.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkPLOT3DReader, "1.66");
vtkStandardNewMacro(vtkPLOT3DReader);

#define VTK_RHOINF 1.0
#define VTK_CINF 1.0
#define VTK_PINF ((VTK_RHOINF*VTK_CINF) * (VTK_RHOINF*VTK_CINF) / this->Gamma)
#define VTK_CV (this->R / (this->Gamma-1.0))

vtkPLOT3DReader::vtkPLOT3DReader()
{
  this->XYZFileName = NULL;
  this->QFileName = NULL;
  this->BinaryFile = 1;
  this->HasByteCount = 0;
  this->FileSize = 0;
  this->MultiGrid = 0;
  this->ForceRead = 0;
  this->ByteOrder = FILE_BIG_ENDIAN;
  this->IBlanking = 0;
  this->DoNotReduceNumberOfOutputs = 1;

  this->R = 1.0;
  this->Gamma = 1.4;
  this->Uvinf = 0.0;
  this->Vvinf = 0.0;
  this->Wvinf = 0.0;

  this->FunctionList = vtkIntArray::New();

  this->ScalarFunctionNumber = -1;
  this->SetScalarFunctionNumber(100);
  this->VectorFunctionNumber = -1;
  this->SetVectorFunctionNumber(202);
} 

vtkPLOT3DReader::~vtkPLOT3DReader()
{
  delete [] this->XYZFileName;
  delete [] this->QFileName;
  this->FunctionList->Delete();
}

int vtkPLOT3DReader::CheckFile(FILE*& fp, const char* fname)
{
  if ( (fp = fopen(fname, "r")) == NULL)
    {
    this->SetErrorCode(vtkErrorCode::FileNotFoundError);
    vtkErrorMacro(<< "File: " << fname << " not found.");
    return VTK_ERROR;
    }
  return VTK_OK;
}

int vtkPLOT3DReader::CheckGeometryFile(FILE*& xyzFp)
{
  if ( this->XYZFileName == NULL || this->XYZFileName[0] == '\0'  )
    {
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    vtkErrorMacro(<< "Must specify geometry file");
    return VTK_ERROR;
    }
  return this->CheckFile(xyzFp, this->XYZFileName);
}

int vtkPLOT3DReader::CheckSolutionFile(FILE*& qFp)
{
  if ( this->QFileName == NULL || this->QFileName[0] == '\0' )
    {
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    vtkErrorMacro(<< "Must specify geometry file");
    return VTK_ERROR;
    }
  return this->CheckFile(qFp, this->QFileName);
}

// Skip Fortran style byte count.
void vtkPLOT3DReader::SkipByteCount(FILE* fp)
{
  if (this->BinaryFile && this->HasByteCount)
    {
    int tmp;
    fread(&tmp, sizeof(int), 1, fp);
    }
}

// Read a block of ints (ascii or binary) and return number read.
int vtkPLOT3DReader::ReadIntBlock(FILE* fp, int n, int* block)
{
  if (this->BinaryFile)
    {
    int retVal=static_cast<int>(fread(block, sizeof(int), n, fp));
    if (this->ByteOrder == FILE_LITTLE_ENDIAN)
      {
      vtkByteSwap::Swap4LERange(block, n);
      }
    else
      {
      vtkByteSwap::Swap4BERange(block, n);
      }
    return retVal;
    }
  else
    {
    int count = 0;
    for(int i=0; i<n; i++)
      {
      int num = fscanf(fp, "%d", &(block[i]));
      if ( num > 0 )
        {
        count++;
        }
      else
        {
        return 0;
        }
      }
    return count;
    }
}

int vtkPLOT3DReader::ReadFloatBlock(FILE* fp, int n, float* block)
{
  if (this->BinaryFile)
    {
    int retVal=static_cast<int>(fread(block, sizeof(float), n, fp));
    if (this->ByteOrder == FILE_LITTLE_ENDIAN)
      {
      vtkByteSwap::Swap4LERange(block, n);
      }
    else
      {
      vtkByteSwap::Swap4BERange(block, n);
      }
    return retVal;
    }
  else
    {
    int count = 0;
    for(int i=0; i<n; i++)
      {
      int num = fscanf(fp, "%f", &(block[i]));
      if ( num > 0 )
        {
        count++;
        }
      else
        {
        return 0;
        }
      }
    return count;
    }
}

// Read a block of floats (ascii or binary) and return number read.
void vtkPLOT3DReader::CalculateFileSize(FILE* fp)
{
  long curPos = ftell(fp);
  fseek(fp, 0, SEEK_END);
  this->FileSize = ftell(fp);
  fseek(fp, curPos, SEEK_SET);
}


// Estimate the size of a grid (binary file only)
long vtkPLOT3DReader::EstimateSize(int ni, int nj, int nk)
{
  long size = 3*4; // the header portion, 3 ints
  size += ni*nj*nk*3*4; // x, y, z
  if (this->HasByteCount)
    {
    size += 2*4; // the byte counts
    }
  if (this->IBlanking)
    {
    size += ni*nj*nk*4;
    }

  return size;
}

int vtkPLOT3DReader::CanReadFile(const char* fname)
{
  FILE* xyzFp;

  if (!fname || fname[0] == '\0')
    {
    return 0;
    }

  if ( this->CheckFile(xyzFp, fname) != VTK_OK)
    {
    return 0;
    }
  int numOutputs = this->GetNumberOfOutputsInternal(xyzFp, 1);
  fclose(xyzFp);
  if (numOutputs != 0)
    {
    return 1;
    }
  return 0;
}

int vtkPLOT3DReader::GetNumberOfOutputs()
{
  FILE* xyzFp;

  if ( this->CheckGeometryFile(xyzFp) != VTK_OK)
    {
    return 0;
    }
  int numOutputs = this->GetNumberOfOutputsInternal(xyzFp, 1);
  fclose(xyzFp);
  if (numOutputs != 0)
    {
    return numOutputs;
    }
  return 1;
}

// Read the header and return the number of grids.
int vtkPLOT3DReader::GetNumberOfOutputsInternal(FILE* xyzFp, int verify)
{
  int numGrid=0;
  int numOutputs=0;

  if ( this->MultiGrid )
    {
    this->SkipByteCount(xyzFp);
    this->ReadIntBlock(xyzFp, 1, &numGrid);
    this->SkipByteCount(xyzFp);
    }
  else
    {
    numGrid=1;
    }

  if (!verify)
    {
    // We were told not the verify the number of grid. Just return it.
    numOutputs = numGrid;
    }
  else
    {
    // We were told to make sure that the file can really contain
    // the number of grid in the header (we can only check this
    // if file is binary)
    int error=0;
    if ( this->BinaryFile )
      {
      // Store the beginning of first grid.
      long pos = ftell(xyzFp);

      long fileSize = 0;
      // Size of number of grids information.
      if ( this->MultiGrid )
        {
        fileSize += 4; // numGrids
        if (this->HasByteCount)
          {
          fileSize += 4*4; // byte counts for the header
          }
        }
      // Add the size of each grid.
      this->SkipByteCount(xyzFp);
      for(int i=0; i<numGrid; i++)
        {
        int ni, nj, nk;
        this->ReadIntBlock(xyzFp, 1, &ni);
        this->ReadIntBlock(xyzFp, 1, &nj);
        this->ReadIntBlock(xyzFp, 1, &nk);
        fileSize += this->EstimateSize(ni, nj, nk);
        // If this number is larger than the file size, there
        // is something wrong.
        if ( fileSize > this->FileSize )
          {
          error = 1;
          break;
          }
        }
      this->SkipByteCount(xyzFp);
      // If this number is different than the file size, there
      // is something wrong.
      if ( fileSize != this->FileSize && !this->ForceRead)
        {
        this->SetErrorCode(vtkErrorCode::FileFormatError);
        error = 1;
        }

      fseek(xyzFp, pos, SEEK_SET);
      }
    else
      {
      if (numGrid == 0)
        {
        this->SetErrorCode(vtkErrorCode::FileFormatError);
        }
      }
    
    // Now set the number of outputs.
    if (!error && numGrid != 0)
      {
      if ( !this->DoNotReduceNumberOfOutputs || 
           numGrid > this->NumberOfOutputs )
        {
        this->SetNumberOfOutputs(numGrid);
        }
      for (int i=1; i<numGrid; i++)
        {
        if (!this->Outputs[i])
          {
          vtkStructuredGrid* sg = vtkStructuredGrid::New();
          this->SetNthOutput(i, sg);
          sg->Delete();
          }
        }
      numOutputs = numGrid;
      }
    else
      {
      numOutputs = 0;
      }
    }

  return numOutputs;
}

int vtkPLOT3DReader::ReadGeometryHeader(FILE* fp)
{
  int numGrid = this->GetNumberOfOutputsInternal(fp, 1);
  int i;
  if ( numGrid == 0 )
    {
    // Bad file, set all extents to invalid.
    for (i=0; i<this->NumberOfOutputs; i++)
      {
      this->GetOutput(i)->SetWholeExtent(0, -1, 0, -1, 0, -1);
      }
    return VTK_ERROR;
    }

  // Read and set extents of all outputs.
  this->SkipByteCount(fp);
  for(i=0; i<numGrid; i++)
    {
    int ni, nj, nk;
    this->ReadIntBlock(fp, 1, &ni);
    this->ReadIntBlock(fp, 1, &nj);
    this->ReadIntBlock(fp, 1, &nk);
    this->GetOutput(i)->SetWholeExtent(0, ni-1, 0, nj-1, 0, nk-1);
    }
  this->SkipByteCount(fp);
  return VTK_OK;
}

int vtkPLOT3DReader::ReadQHeader(FILE* fp)
{
  int numGrid = this->GetNumberOfOutputsInternal(fp, 0);
  if ( numGrid == 0 )
    {
    return VTK_ERROR;
    }

  this->SkipByteCount(fp);
  for(int i=0; i<numGrid; i++)
    {
    int ni, nj, nk;
    this->ReadIntBlock(fp, 1, &ni);
    this->ReadIntBlock(fp, 1, &nj);
    this->ReadIntBlock(fp, 1, &nk);

    int extent[6];
    this->GetOutput(i)->GetWholeExtent(extent);
    if ( extent[1] != ni-1 || extent[3] != nj-1 || extent[5] != nk-1)
      {
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      vtkErrorMacro("Geometry and data dimensions do not match. "
                    "Data file may be corrupt.");
      return VTK_ERROR;
      }
    }
  this->SkipByteCount(fp);
  return VTK_OK;
}

void vtkPLOT3DReader::SetScalarFunctionNumber(int num)
{
  if ( this->ScalarFunctionNumber == num)
    {
    return;
    }
  if (num >= 0)
    {
    // If this function is not in the list, add it.
    int found=0;
    for (int i=0; i < this->FunctionList->GetNumberOfTuples(); i++ )
      {
      if ( this->FunctionList->GetValue(i) == num )
        {
        int found=1;
        }
      }
    if (!found)
      {
      this->AddFunction(num);
      }
    }
  this->ScalarFunctionNumber = num;
}

void vtkPLOT3DReader::SetVectorFunctionNumber(int num)
{
  if ( this->VectorFunctionNumber == num)
    {
    return;
    }
  if (num >= 0)
    {
    // If this function is not in the list, add it.
    int found=0;
    for (int i=0; i < this->FunctionList->GetNumberOfTuples(); i++ )
      {
      if ( this->FunctionList->GetValue(i) == num )
        {
        int found=1;
        }
      }
    if (!found)
      {
      this->AddFunction(num);
      }
    }
  this->VectorFunctionNumber = num;
}

void vtkPLOT3DReader::RemoveFunction(int fnum)
{
  for (int i=0; i < this->FunctionList->GetNumberOfTuples(); i++ )
    {
    if ( this->FunctionList->GetValue(i) == fnum )
      {
      this->FunctionList->SetValue(i,-1);
      this->Modified();
      }
    }
}

void vtkPLOT3DReader::ExecuteInformation()
{
  FILE* xyzFp;

  if ( this->CheckGeometryFile(xyzFp) != VTK_OK)
    {
    return;
    }

  this->CalculateFileSize(xyzFp);
  this->ReadGeometryHeader(xyzFp);

  fclose(xyzFp);
}

void vtkPLOT3DReader::Execute()
{
  this->SetErrorCode(vtkErrorCode::NoError);

  int i;

  FILE* xyzFp;
  if ( this->CheckGeometryFile(xyzFp) != VTK_OK)
    {
    return;
    }

  if ( this->ReadGeometryHeader(xyzFp) != VTK_OK )
    {
    vtkErrorMacro("Error reading geometry file.");
    fclose(xyzFp);
    return;
    }

  int ndim, nx, ny, nz;
  vtkIdType index;

  for(i=0; i<this->NumberOfOutputs; i++)
    {

    // Read the geometry of this grid.
    this->SkipByteCount(xyzFp);

    vtkStructuredGrid* nthOutput = this->GetOutput(i);
    int dims[6];
    nthOutput->GetWholeExtent(dims);
    nthOutput->SetExtent(dims);
    nthOutput->GetDimensions(dims);
    vtkFloatArray* parray = vtkFloatArray::New();
    parray->SetNumberOfComponents(3);
    parray->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );

    vtkPoints* points = vtkPoints::New();
    points->SetData(parray);
    nthOutput->SetPoints(points);
    points->Delete();
    parray->Delete();

    float coord;
    for(ndim=0; ndim < 3; ndim++)
      {
      for(nz=0; nz < dims[2]; nz++)
        {
        for(ny=0; ny < dims[1]; ny++)
          {
          for(nx=0; nx < dims[0]; nx++)
            {
            if ( this->ReadFloatBlock(xyzFp, 1, &coord) == 0 )
              {
              vtkErrorMacro("Encountered premature end-of-file while reading "
                            "the geometry file (or the file is corrupt).");
              this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
              // We need to generate output (otherwise, this filter will
              // keep executing). So we produce all 0's
              float nullpt[3] = {0.0, 0.0, 0.0};
              vtkIdType ipts, npts=parray->GetNumberOfTuples();
              for(ipts=0; ipts < npts; ipts++)
                {
                parray->SetTuple(ipts, nullpt);
                }
              return;
              }
            index = nz*dims[0]*dims[1]+ny*dims[0]+nx;
            parray->SetComponent(index, ndim, coord);
            }
          }
        }
      }
    if (this->IBlanking)
      {
      vtkUnsignedCharArray* iblank = vtkUnsignedCharArray::New();
      iblank->SetNumberOfComponents(1);
      iblank->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
      iblank->SetName("Visibility");
      int* ib = new int[dims[0]*dims[1]*dims[2]];
      if ( this->ReadIntBlock(xyzFp, dims[0]*dims[1]*dims[2], ib) == 0)
        {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        return;
        }
      vtkIdType ipts, npts=iblank->GetNumberOfTuples();
      unsigned char* ib2 = iblank->GetPointer(0);
      for (ipts=0; ipts<npts; ipts++)
        {
        ib2[ipts] = ib[ipts];
        }
      delete[] ib;
      nthOutput->SetPointVisibility(iblank);
      iblank->Delete();
      }

    this->SkipByteCount(xyzFp);
    }

  fclose(xyzFp);

  // Now read the solution.
  if (this->QFileName && this->QFileName[0] != '\0')
    {
    FILE* qFp;
    if ( this->CheckSolutionFile(qFp) != VTK_OK)
      {
      return;
      }
    
    if ( this->ReadQHeader(qFp) != VTK_OK )
      {
      fclose(qFp);
      return;
      }

    for(i=0; i<this->NumberOfOutputs; i++)
      {
      vtkStructuredGrid* nthOutput = this->GetOutput(i);

      float fsmach, alpha, re, time;

      this->SkipByteCount(qFp);
      this->ReadFloatBlock(qFp, 1, &fsmach);
      this->ReadFloatBlock(qFp, 1, &alpha);
      this->ReadFloatBlock(qFp, 1, &re);
      this->ReadFloatBlock(qFp, 1, &time);
      this->SkipByteCount(qFp);

      // Save the properties first
      vtkFloatArray* properties = vtkFloatArray::New();
      properties->SetName("Properties");
      properties->SetNumberOfTuples(4);
      properties->SetTuple1(0, fsmach); 
      properties->SetTuple1(1, alpha); 
      properties->SetTuple1(2, re); 
      properties->SetTuple1(3, time); 
      nthOutput->GetFieldData()->AddArray(properties);
      properties->Delete();
      
      int dims[6];
      nthOutput->GetWholeExtent(dims);
      nthOutput->SetExtent(dims);
      nthOutput->GetDimensions(dims);

      this->SkipByteCount(qFp);

      vtkFloatArray* density = vtkFloatArray::New();
      density->SetNumberOfComponents(1);
      density->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
      density->SetName("Density");
      float* dens = density->GetPointer(0);
      if ( this->ReadFloatBlock(qFp, dims[0]*dims[1]*dims[2], dens) == 0)
        {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        return;
        }
      nthOutput->GetPointData()->AddArray(density);
      density->Delete();

      vtkFloatArray* momentum = vtkFloatArray::New();
      momentum->SetNumberOfComponents(3);
      momentum->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
      momentum->SetName("Momentum");

      float comp;
      for(ndim=0; ndim < 3; ndim++)
        {
        for(nz=0; nz < dims[2]; nz++)
          {
          for(ny=0; ny < dims[1]; ny++)
            {
            for(nx=0; nx < dims[0]; nx++)
              {
              if ( this->ReadFloatBlock(qFp, 1, &comp) == 0 )
                {
                vtkErrorMacro("Encountered premature end-of-file while "
                              "reading the q file (or the file is corrupt).");
                return;
                }
              index = nz*dims[0]*dims[1]+ny*dims[0]+nx;
              momentum->SetComponent(index, ndim, comp);
              }
            }
          }
        }

      nthOutput->GetPointData()->AddArray(momentum);
      momentum->Delete();

      vtkFloatArray* se = vtkFloatArray::New();
      se->SetNumberOfComponents(1);
      se->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
      se->SetName("StagnationEnergy");
      float* sen = se->GetPointer(0);
      if (this->ReadFloatBlock(qFp, dims[0]*dims[1]*dims[2], sen) == 0)
        {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        return;
        }
      nthOutput->GetPointData()->AddArray(se);
      se->Delete();

      
      if ( this->FunctionList->GetNumberOfTuples() > 0 )
        {
        int fnum;
        for (int i=0; i < this->FunctionList->GetNumberOfTuples(); i++)
          {
          if ( (fnum=this->FunctionList->GetValue(i)) >= 0 )
            {
            this->MapFunction(fnum, nthOutput);
            }
          }
        }
      this->AssignAttribute(this->ScalarFunctionNumber, nthOutput,
                            vtkDataSetAttributes::SCALARS);
      this->AssignAttribute(this->VectorFunctionNumber, nthOutput,
                            vtkDataSetAttributes::VECTORS);
      }
    }

}

// Various PLOT3D functions.....................
void vtkPLOT3DReader::MapFunction(int fNumber, vtkStructuredGrid* output)
{
  switch (fNumber)
    {
    case 100: //Density
      break;

    case 110: //Pressure
      this->ComputePressure(output);
      break;

    case 120: //Temperature
      this->ComputeTemperature(output);
      break;

    case 130: //Enthalpy
      this->ComputeEnthalpy(output);
      break;

    case 140: //Internal Energy
      break;

    case 144: //Kinetic Energy
      this->ComputeKineticEnergy(output);
      break;

    case 153: //Velocity Magnitude
      this->ComputeVelocityMagnitude(output);
      break;

    case 163: //Stagnation energy
      break;

    case 170: //Entropy
      this->ComputeEntropy(output);
      break;

    case 184: //Swirl
      this->ComputeSwirl(output);
      break;

    case 200: //Velocity
      this->ComputeVelocity(output);
      break;

    case 201: //Vorticity
      this->ComputeVorticity(output);
      break;

    case 202: //Momentum
      break;

    case 210: //PressureGradient
      this->ComputePressureGradient(output);
      break;

    default:
      vtkErrorMacro(<<"No function number " << fNumber);
    }
}

void vtkPLOT3DReader::AssignAttribute(int fNumber, vtkStructuredGrid* output,
                                  int attributeType)
{
  switch (fNumber)
    {
    case -1:  //empty mapping
      output->GetPointData()->SetActiveAttribute(0, 
                                                 attributeType);
      break;

    case 100: //Density
      output->GetPointData()->SetActiveAttribute("Density", 
                                                 attributeType);
      break;

    case 110: //Pressure
      output->GetPointData()->SetActiveAttribute("Pressure", 
                                                 attributeType);
      break;

    case 120: //Temperature
      output->GetPointData()->SetActiveAttribute("Temperature", 
                                                 attributeType);
      break;

    case 130: //Enthalpy
      output->GetPointData()->SetActiveAttribute("Enthalpy", 
                                                 attributeType);
      break;

    case 140: //Internal Energy
      output->GetPointData()->SetActiveAttribute("StagnationEnergy", 
                                                 attributeType);
      break;

    case 144: //Kinetic Energy
      output->GetPointData()->SetActiveAttribute("KineticEnergy", 
                                                 attributeType);
      break;

    case 153: //Velocity Magnitude
      output->GetPointData()->SetActiveAttribute("VelocityMagnitude", 
                                                 attributeType);
      break;

    case 163: //Stagnation energy
      output->GetPointData()->SetActiveAttribute("StagnationEnergy", 
                                                 attributeType);
      break;

    case 170: //Entropy
      output->GetPointData()->SetActiveAttribute("Entropy", 
                                                 attributeType);
      break;

    case 184: //Swirl
      output->GetPointData()->SetActiveAttribute("Swirl", 
                                                 attributeType);
      break;

    case 200: //Velocity
      output->GetPointData()->SetActiveAttribute("Velocity", 
                                                 attributeType);
      break;

    case 201: //Vorticity
      output->GetPointData()->SetActiveAttribute("Vorticity", 
                                                 attributeType);
      break;

    case 202: //Momentum
      output->GetPointData()->SetActiveAttribute("Momentum", 
                                                 attributeType);
      break;

    case 210: //PressureGradient
      output->GetPointData()->SetActiveAttribute("PressureGradient", 
                                                 attributeType);
      break;

    default:
      vtkErrorMacro(<<"No function number " << fNumber);
    }
}

void vtkPLOT3DReader::ComputeTemperature(vtkStructuredGrid* output)
{
  float *m, e, rr, u, v, w, v2, p, d, rrgas;
  vtkIdType i;
  vtkFloatArray *temperature;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  
  if ( density == NULL || momentum == NULL || 
       energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute temperature");
    return;
    }

  vtkIdType numPts = density->GetNumberOfTuples();
  temperature = vtkFloatArray::New();
  temperature->SetNumberOfTuples(numPts);

  //  Compute the temperature
  //
  rrgas = 1.0 / this->R;
  for (i=0; i < numPts; i++) 
    {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    e = energy->GetComponent(i,0);
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
  
  temperature->Delete();
  vtkDebugMacro(<<"Created temperature scalar");
}

void vtkPLOT3DReader::ComputePressure(vtkStructuredGrid* output)
{
  float *m, e, u, v, w, v2, p, d, rr;
  vtkIdType i;
  vtkFloatArray *pressure;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL || 
       energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute pressure");
    return;
    }

  vtkIdType numPts = density->GetNumberOfTuples();
  pressure = vtkFloatArray::New();
  pressure->SetNumberOfTuples(numPts);

  //  Compute the pressure
  //
  for (i=0; i < numPts; i++) 
    {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    e = energy->GetComponent(i,0);
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
  pressure->Delete();
  vtkDebugMacro(<<"Created pressure scalar");
}

void vtkPLOT3DReader::ComputeEnthalpy(vtkStructuredGrid* output)
{
  float *m, e, u, v, w, v2, d, rr;
  vtkIdType i;
  vtkFloatArray *enthalpy;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL || 
       energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute enthalpy");
    return;
    }

  vtkIdType numPts = density->GetNumberOfTuples();
  enthalpy = vtkFloatArray::New();
  enthalpy->SetNumberOfTuples(numPts);

  //  Compute the enthalpy
  //
  for (i=0; i < numPts; i++) 
    {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    e = energy->GetComponent(i,0);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    enthalpy->SetValue(i, this->Gamma*(e*rr - 0.5*v2));
  }
  enthalpy->SetName("Enthalpy");
  outputPD->AddArray(enthalpy);
  enthalpy->Delete();
  vtkDebugMacro(<<"Created enthalpy scalar");
}

void vtkPLOT3DReader::ComputeKineticEnergy(vtkStructuredGrid* output)
{
  float *m, u, v, w, v2, d, rr;
  vtkIdType i;
  vtkFloatArray *kineticEnergy;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  if ( density == NULL || momentum == NULL )
    {
    vtkErrorMacro(<<"Cannot compute kinetic energy");
    return;
    }

  vtkIdType numPts = density->GetNumberOfTuples();
  kineticEnergy = vtkFloatArray::New();
  kineticEnergy->SetNumberOfTuples(numPts);

  //  Compute the kinetic energy
  //
  for (i=0; i < numPts; i++) 
    {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    kineticEnergy->SetValue(i, 0.5*v2);
  }
  kineticEnergy->SetName("KineticEnergy");
  outputPD->AddArray(kineticEnergy);
  kineticEnergy->Delete();
  vtkDebugMacro(<<"Created kinetic energy scalar");
}

void vtkPLOT3DReader::ComputeVelocityMagnitude(vtkStructuredGrid* output)
{
  float *m, u, v, w, v2, d, rr;
  vtkIdType i;
  vtkFloatArray *velocityMag;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL ||
       energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute velocity magnitude");
    return;
    }

  vtkIdType numPts = density->GetNumberOfTuples();
  velocityMag = vtkFloatArray::New();
  velocityMag->SetNumberOfTuples(numPts);

  //  Compute the velocity magnitude
  //
  for (i=0; i < numPts; i++) 
    {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    rr = 1.0 / d;
    u = m[0] * rr;        
    v = m[1] * rr;        
    w = m[2] * rr;        
    v2 = u*u + v*v + w*w;
    velocityMag->SetValue(i, sqrt((double)v2));
  }
  velocityMag->SetName("VelocityMagnitude");
  outputPD->AddArray(velocityMag);
  velocityMag->Delete();
  vtkDebugMacro(<<"Created velocity magnitude scalar");
}

void vtkPLOT3DReader::ComputeEntropy(vtkStructuredGrid* output)
{
  float *m, u, v, w, v2, d, rr, s, p, e;
  vtkIdType i;
  vtkFloatArray *entropy;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL ||
       energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute entropy");
    return;
    }

  vtkIdType numPts = density->GetNumberOfTuples();
  entropy = vtkFloatArray::New();
  entropy->SetNumberOfTuples(numPts);

  //  Compute the entropy
  //
  for (i=0; i < numPts; i++) 
    {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    e = energy->GetComponent(i,0);
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
  entropy->Delete();
  vtkDebugMacro(<<"Created entropy scalar");
}

void vtkPLOT3DReader::ComputeSwirl(vtkStructuredGrid* output)
{
  vtkDataArray *vorticity;
  float d, rr, *m, u, v, w, v2, *vort, s;
  vtkIdType i;
  vtkFloatArray *swirl;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL ||
       energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute swirl");
    return;
    }

  vtkIdType numPts = density->GetNumberOfTuples();
  swirl = vtkFloatArray::New();
  swirl->SetNumberOfTuples(numPts);

  this->ComputeVorticity(output);
  vorticity = outputPD->GetArray("Vorticity");
//
//  Compute the swirl
//
  for (i=0; i < numPts; i++) 
    {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
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
  swirl->Delete();
  vtkDebugMacro(<<"Created swirl scalar");

}

// Vector functions
void vtkPLOT3DReader::ComputeVelocity(vtkStructuredGrid* output)
{
  float *m, v[3], d, rr;
  vtkIdType i;
  vtkFloatArray *velocity;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL ||
       energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute velocity");
    return;
    }

  vtkIdType numPts = density->GetNumberOfTuples();
  velocity = vtkFloatArray::New();
  velocity->SetNumberOfComponents(3);
  velocity->SetNumberOfTuples(numPts);

  //  Compute the velocity
  //
  for (i=0; i < numPts; i++) 
    {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    rr = 1.0 / d;
    v[0] = m[0] * rr;        
    v[1] = m[1] * rr;        
    v[2] = m[2] * rr;        
    velocity->SetTuple(i, v);
  }
  velocity->SetName("Velocity");
  outputPD->AddArray(velocity);
  velocity->Delete();
  vtkDebugMacro(<<"Created velocity vector");
}

void vtkPLOT3DReader::ComputeVorticity(vtkStructuredGrid* output)
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
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( (points=output->GetPoints()) == NULL || 
       density == NULL || momentum == NULL || 
       energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute vorticity");
    return;
    }

  vtkIdType numPts = density->GetNumberOfTuples();
  vorticity = vtkFloatArray::New();
  vorticity->SetNumberOfComponents(3);
  vorticity->SetNumberOfTuples(numPts);

  this->ComputeVelocity(output);
  velocity = outputPD->GetArray("Velocity");

  output->GetDimensions(dims);
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
  vorticity->Delete();
  vtkDebugMacro(<<"Created vorticity vector");
}

void vtkPLOT3DReader::ComputePressureGradient(vtkStructuredGrid* output)
{
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
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( (points=output->GetPoints()) == NULL || 
       density == NULL || momentum == NULL || 
       energy == NULL )
    {
    vtkErrorMacro(<<"Cannot compute pressure gradient");
    return;
    }

  vtkIdType numPts = density->GetNumberOfTuples();
  gradient = vtkFloatArray::New();
  gradient->SetNumberOfComponents(3);
  gradient->SetNumberOfTuples(numPts);

  this->ComputePressure(output);
  pressure = outputPD->GetArray("Pressure");

  output->GetDimensions(dims);
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
  gradient->SetName("PressureGradient");
  outputPD->AddArray(gradient);
  gradient->Delete();
  vtkDebugMacro(<<"Created pressure gradient vector");
}

void vtkPLOT3DReader::SetByteOrderToBigEndian()
{
  this->ByteOrder = FILE_BIG_ENDIAN;
}

void vtkPLOT3DReader::SetByteOrderToLittleEndian()
{
  this->ByteOrder = FILE_LITTLE_ENDIAN;
}

const char *vtkPLOT3DReader::GetByteOrderAsString()
{
  if ( this->ByteOrder ==  FILE_LITTLE_ENDIAN)
    {
    return "LittleEndian";
    }
  else
    {
    return "BigEndian";
    }
}

void vtkPLOT3DReader::SetOutput(int idx, vtkStructuredGrid *output)
{ 
  this->Superclass::SetNthOutput(idx, output); 
}

void vtkPLOT3DReader::AddFunction(int functionNumber)
{
  this->FunctionList->InsertNextValue(functionNumber); 
  this->Modified();
}

void vtkPLOT3DReader::RemoveAllFunctions()
{
  this->FunctionList->Reset();
  this->Modified();
}

void vtkPLOT3DReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "XYZ File Name: " << 
    (this->XYZFileName ? this->XYZFileName : "(none)") << "\n";
  os << indent << "Q File Name: " <<
    (this->QFileName ? this->QFileName : "(none)") << "\n";
  os << indent << "BinaryFile: " << this->BinaryFile << endl;
  os << indent << "HasByteCount: " << this->HasByteCount << endl;
  os << indent << "Gamma: " << this->Gamma << endl;
  os << indent << "R: " << this->R << endl;
  os << indent << "Uvinf: " << this->Uvinf << endl;
  os << indent << "Vvinf: " << this->Vvinf << endl;
  os << indent << "Wvinf: " << this->Wvinf << endl;
  os << indent << "ScalarFunctionNumber: " << this->ScalarFunctionNumber << endl;
  os << indent << "VectorFunctionNumber: " << this->VectorFunctionNumber << endl;
  os << indent << "MultiGrid: " << this->MultiGrid << endl;
  os << indent << "DoNotReduceNumberOfOutputs: " 
     << this->DoNotReduceNumberOfOutputs << endl;
  os << indent << "ForceRead: " << this->ForceRead << endl;
  os << indent << "IBlanking: " << this->IBlanking << endl;
  os << indent << "ByteOrder: " << this->ByteOrder << endl;
}

