/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoReader.cxx

Copyright (c) 2007, 2009 Los Alamos National Security, LLC

All rights reserved.

Copyright 2007, 2009. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkCosmoReader.h"
#include "vtkDataArraySelection.h"
#include "vtkErrorCode.h"
#include "vtkUnstructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkFieldData.h"
#include "vtkPointData.h"
#include "vtkByteSwap.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkDataArray.h"
#include "vtkConfigure.h"
#include "vtkStdString.h"

#ifdef VTK_TYPE_USE_LONG_LONG
#include "vtkLongLongArray.h"
#endif

vtkCxxRevisionMacro(vtkCosmoReader, "1.23");
vtkStandardNewMacro(vtkCosmoReader);

using namespace cosmo;

//----------------------------------------------------------------------------
vtkCosmoReader::vtkCosmoReader()
{
  this->SetNumberOfInputPorts(0);
  this->FileName               = NULL;
  this->FileStream             = NULL;
  this->ByteOrder              = FILE_LITTLE_ENDIAN;
  this->Stride                 = 1;
  this->BoxSize                = 0;
  this->PositionRange[0]       = 0;
  this->PositionRange[1]       = -1;
  this->NumberOfNodes          = 0;
  this->NumberOfVariables     = 0;
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->MakeCells = 1;
  this->TagSize = 0;
  this->ComponentNumber = new vtkIdType[NUMBER_OF_VAR];
  this->VariableName = new vtkStdString[NUMBER_OF_VAR];
}

//----------------------------------------------------------------------------
vtkCosmoReader::~vtkCosmoReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  this->PointDataArraySelection->Delete();
  delete[] this->ComponentNumber;
  delete[] this->VariableName;
}

//----------------------------------------------------------------------------
void vtkCosmoReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << endl;

  os << indent << "Number Of Variables: " << this->NumberOfVariables << endl;
  for (int i=0; i < this->NumberOfVariables; i++)
    {
    os << "\tVariableName[" << i << "] = " 
       << this->VariableName[i] << endl;
    os << "\tComponentNumber[" << i << "] = " 
       << this->ComponentNumber[i] << endl;
    os << "\tPointDataArraySelection->GetArraySetting(" << i << ") = " 
       << (this->PointDataArraySelection->GetArraySetting(i) 
           ? "ENABLED" : "DISABLED") << endl;
    os << endl;
    }

  os << indent << "PositionRange[0]: " << this->PositionRange[0] << endl;
  os << indent << "PositionRange[1]: " << this->PositionRange[1] << endl;
  os << indent << "Stride: " << this->Stride << endl;
  
  os << indent << "Byte Order: " 
     << (this->ByteOrder ? "LITTLE ENDIAN" : "BIG ENDIAN") << endl;

  os << indent << "Number Of Nodes: " << this->NumberOfNodes << endl;
  os << indent << "BoxSize: " << this->BoxSize << endl;
  os << indent << "MakeCells: " << (this->MakeCells?"on":"off") << endl;
  os << indent << "TagSize: " << (this->TagSize ? "64-bit" : "32-bit")
     << endl;
}

//----------------------------------------------------------------------------
int vtkCosmoReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // Verify that file exists
  if ( !this->FileName )
    {
    vtkErrorMacro("No filename specified");
    return 0;
    }

#if defined(_WIN32) && !defined(__CYGWIN__)
    this->FileStream = new ifstream(this->FileName, ios::in | ios::binary);
#else
    this->FileStream = new ifstream(this->FileName, ios::in);
#endif

  // Can the file be opened
  if (this->FileStream->fail())
    {
    this->SetErrorCode(vtkErrorCode::FileNotFoundError);
    delete this->FileStream;
    this->FileStream = NULL;
    vtkErrorMacro("Specified filename not found");
    return 0;
    }
                                                   
  // Calculates the number of particles based on record size
  this->ComputeDefaultRange();

  // Fields associated with each particle point: velocity, mass, tag
  this->NumberOfVariables = NUMBER_OF_VAR;

  this->VariableName[0] = "velocity";
  this->ComponentNumber[0] = DIMENSION; // x, y, z velocities

  this->VariableName[1] = "mass";
  this->ComponentNumber[1] = 1;         // mass of particle

  this->VariableName[2] = "tag";
  this->ComponentNumber[2] = 1;         // tag id of particle
                                                                                
  // Add scalar arrays for each field to both points and cells
  for (int i = 0; i < this->NumberOfVariables; i++)
    this->PointDataArraySelection->AddArray(this->VariableName[i].c_str());

  vtkDebugMacro( << "RequestInformation: NumberOfNodes = "
                 << this->NumberOfNodes  << endl);
  vtkDebugMacro( << "end of RequestInformation\n");

  delete this->FileStream;
  this->FileStream = 0;

  return 1;
}

//----------------------------------------------------------------------------
int vtkCosmoReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
                                                                                
  // get the output
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
                                                                                
  vtkDebugMacro( << "Reading Cosmo file");
                                                                           
  // Read the file into the output unstructured grid
  this->ReadFile(output);

  return 1;
}

//----------------------------------------------------------------------------
void vtkCosmoReader::ReadFile(vtkUnstructuredGrid *output)
{
  this->SetErrorCode(vtkErrorCode::NoError);

#if defined(_WIN32) && !defined(__CYGWIN__)
  this->FileStream = new ifstream(this->FileName, ios::in | ios::binary);
#else
  this->FileStream = new ifstream(this->FileName, ios::in);
#endif

  // File exists and can be opened
  if (this->FileStream->fail())
    {
    this->SetErrorCode(vtkErrorCode::FileNotFoundError);
    delete this->FileStream;
    this->FileStream = NULL;
    vtkErrorMacro("Specified filename not found");
    return;
    }
  
  // Make sure the stride across the data is legal
  if (this->Stride <= 0)
    {
    this->Stride = 1;
    }
  if (this->Stride > this->PositionRange[1])
    {
    this->Stride = 1;
    }

  // Given the requested stride set the number of nodes to be used
  this->NumberOfNodes = (this->PositionRange[1] - this->PositionRange[0]) / 
                         this->Stride + 1;
  
  // Create the arrays to hold location and field data
  vtkPoints *points       = vtkPoints::New();
  vtkFloatArray *velocity = vtkFloatArray::New();
  vtkFloatArray *mass     = vtkFloatArray::New();
  vtkDataArray *tag;
  if(this->TagSize) 
    {
    if(sizeof(long) == sizeof(vtkTypeInt64)) 
      {
      tag = vtkLongArray::New();
      }
    else if(sizeof(int) == sizeof(vtkTypeInt64))
      {
      tag = vtkIntArray::New();
      }
#ifdef VTK_TYPE_USE_LONG_LONG
    else if(sizeof(long long) == sizeof(vtkTypeInt64))
      {
      tag = vtkLongLongArray::New();
      }
#endif
    else
      {
      vtkErrorMacro("Unable to match 64-bit int type to a compiler type. " <<
                    "Going to use long array to store tag data. " <<
                    "Might truncate data.");
      tag = vtkLongArray::New();
      }
    }
  else
    {
    if(sizeof(int) == sizeof(vtkTypeInt32)) 
      {
      tag = vtkIntArray::New();
      }
    else if(sizeof(long) == sizeof(vtkTypeInt32))
      {
      tag = vtkLongArray::New();
      }
#ifdef VTK_TYPE_USE_LONG_LONG
    else if(sizeof(long long) == sizeof(vtkTypeInt32))
      {
      tag = vtkLongLongArray::New();
      }
#endif
    else 
      {
      vtkErrorMacro("Unable to match 32-bit int type to a compiler type. " <<
                    "Going to use int array to store tag data. " <<
                    "Might truncate data.");
      tag = vtkIntArray::New();
      }
    }

  // Allocate space in the unstructured grid for all nodes
  output->Allocate(this->NumberOfNodes, this->NumberOfNodes);
  output->SetPoints(points);

  // Allocate velocity array if requested, add to point and cell data
  if (this->PointDataArraySelection->GetArraySetting(USE_VELOCITY))
    {
    velocity->SetName("velocity");
    velocity->SetNumberOfComponents(DIMENSION);
    velocity->SetNumberOfTuples(this->NumberOfNodes);
    output->GetPointData()->AddArray(velocity);
    if (!output->GetPointData()->GetVectors())
      {
      output->GetPointData()->SetVectors(velocity);
      }
    }
  
  // Allocate mass array if requested, add to point and cell data
  if (this->PointDataArraySelection->GetArraySetting(USE_MASS))
    {
    mass->SetName("mass");
    mass->SetNumberOfComponents(1);
    mass->SetNumberOfTuples(this->NumberOfNodes);
    output->GetPointData()->AddArray(mass);
    if (!output->GetPointData()->GetScalars())
      {
      output->GetPointData()->SetScalars(mass);
      }
    }

  // Allocate tag array if requested, add to point and cell data
  if (this->PointDataArraySelection->GetArraySetting(USE_TAG))
    {
    tag->SetName("tag");
    tag->SetNumberOfComponents(1);
    tag->SetNumberOfTuples(this->NumberOfNodes);
    output->GetPointData()->AddArray(tag);
    if (!output->GetPointData()->GetScalars())
      {
      output->GetPointData()->SetScalars(tag);
      }
    }

  const unsigned int numFloats = 7;
  const unsigned int numInts = 1;
  vtkTypeFloat32 block[numFloats]; // x,xvel,y,yvel,z,zvel,mass
  char iBlock[sizeof(vtkTypeInt64)];  // it's either going to be 4 or 8
  //bool firstTime = true;

  size_t tagBytes;
  if(this->TagSize)
    {
    tagBytes = sizeof(vtkTypeInt64);
    }
  else
    {
    tagBytes = sizeof(vtkTypeInt32);
    }

  int chunksize = this->NumberOfNodes / 100;

  // Loop to read all particle data
  for (vtkIdType i = this->PositionRange[0]; 
       i <= this->PositionRange[1]; 
       i = i + this->Stride)
    {

    if ((i - this->PositionRange[0]) % chunksize == 0) 
      {
      double progress = (double)(i - this->PositionRange[0]) 
        / (double)(this->NumberOfNodes); 

      this->UpdateProgress(progress);
      }

    // seek and read
    size_t position = 
      i * (BYTES_PER_DATA_MINUS_TAG + tagBytes);
    this->FileStream->seekg(position, ios::beg);

    // Read the floating point part of the data
    this->FileStream->read((char*)block, numFloats * sizeof(vtkTypeFloat32));

    size_t returnValue = this->FileStream->gcount();
    if (returnValue != numFloats * sizeof(vtkTypeFloat32))
      {
      vtkErrorMacro(<< "Only read " 
                    << returnValue << " bytes when reading floats.");
      this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
      continue;
      }

    // Read the integer part of the data
    this->FileStream->read(iBlock, numInts * tagBytes);
    returnValue = this->FileStream->gcount();
    if (returnValue != numInts * tagBytes)
      {
      vtkErrorMacro(<< "Only read " 
                    << returnValue << " bytes when reading ints.");
      this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
      continue;
      }

    // swap if necessary
#ifdef VTK_WORDS_BIG_ENDIAN
    if(this->ByteOrder == FILE_LITTLE_ENDIAN)
      {
      vtkByteSwap::SwapVoidRange(block, numFloats, 
                                 (int)sizeof(vtkTypeFloat32));
      vtkByteSwap::SwapVoidRange(iBlock, numInts, (int)tagBytes);
      }
#else
    if(this->ByteOrder == FILE_BIG_ENDIAN)
      {
      vtkByteSwap::SwapVoidRange(block, numFloats, 
                                 (int)sizeof(vtkTypeFloat32));
      vtkByteSwap::SwapVoidRange(iBlock, numInts, (int)tagBytes);
      }
#endif

    // Negative value is an error so wraparound if it occurs
    if (block[X] < 0.0) 
      {
      block[X] = this->BoxSize + block[X];
      }
    if (block[Y] < 0.0) 
      {
      block[Y] = this->BoxSize + block[Y];
      }
    if (block[Z] < 0.0) 
      {
      block[Z] = this->BoxSize - block[Z];
      }

    // Insert the location into the point array
    vtkIdType vtkPointID = 
      points->InsertNextPoint(block[X], block[Y], block[Z]);
    if (this->MakeCells)
      {
      output->InsertNextCell(1, 1, &vtkPointID);
      }

    // Store velocity data if requested
    if (this->PointDataArraySelection->GetArraySetting(USE_VELOCITY))
      {
      velocity->SetComponent(vtkPointID, 0, block[X_VELOCITY]);
      velocity->SetComponent(vtkPointID, 1, block[Y_VELOCITY]);
      velocity->SetComponent(vtkPointID, 2, block[Z_VELOCITY]);
      }

    // Store mass data if requested
    if (this->PointDataArraySelection->GetArraySetting(USE_MASS))
      {
      mass->SetComponent(vtkPointID, 0, block[MASS]);
      }

    // Store tag data if requested
    if (this->PointDataArraySelection->GetArraySetting(USE_TAG))
      {
      double value;
      if(this->TagSize) 
        {
        value = *((vtkTypeInt64*)iBlock);
        }
      else
        {
        value = *((vtkTypeInt32*)iBlock);
        }

      tag->SetComponent(vtkPointID, 0, value);
      }
    } // end loop over PositionRange

  // Clean up internal storage
  velocity->Delete();
  mass->Delete();
  tag->Delete();
  points->Delete();
  output->Squeeze();
 
  // Close the file stream just read
  delete this->FileStream;
  this->FileStream = NULL;
}

//----------------------------------------------------------------------------
// Sets the range of particle indices based on length of file
void vtkCosmoReader::ComputeDefaultRange()
{
  this->FileStream->seekg(0L, ios::end);
  size_t fileLength = this->FileStream->tellg();

  size_t tagBytes;
  if(this->TagSize)
    {
    tagBytes = sizeof(vtkTypeInt64);
    }
  else 
    {
    tagBytes = sizeof(vtkTypeInt32);
    }

  // Divide by number of components per record (x,xv,y,yv,z,zv,mass,tag)
  // Divide by 4 for single precision float
  this->NumberOfNodes = fileLength / (BYTES_PER_DATA_MINUS_TAG + tagBytes);

  this->PositionRange[0] = 0;
  this->PositionRange[1] = this->NumberOfNodes - 1;
}

//----------------------------------------------------------------------------
void vtkCosmoReader::SetByteOrderToBigEndian()
{
  this->ByteOrder = FILE_BIG_ENDIAN;
}


//----------------------------------------------------------------------------
void vtkCosmoReader::SetByteOrderToLittleEndian()
{
  this->ByteOrder = FILE_LITTLE_ENDIAN;
}

//----------------------------------------------------------------------------
int vtkCosmoReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkCosmoReader::EnableAllPointArrays()
{
    this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkCosmoReader::DisableAllPointArrays()
{
    this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
const char* vtkCosmoReader::GetPointArrayName(int index)
{
  return this->VariableName[index].c_str();
}

//----------------------------------------------------------------------------
int vtkCosmoReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkCosmoReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
    {
    this->PointDataArraySelection->EnableArray(name);
    }
  else
    {
    this->PointDataArraySelection->DisableArray(name);
    }
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkCosmoReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkCosmoReader::GetOutput(int idx)
{
  if (idx)
    {
    return NULL;
    }
  else
    {
    return vtkUnstructuredGrid::SafeDownCast( this->GetOutputDataObject(idx) );
    }
}
