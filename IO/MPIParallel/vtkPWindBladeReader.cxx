/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPWindBladeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This class was developed by Sohail Shafii; University of California Davis;
// Davis, CA 95616. sohailshafii@yahoo.com.

#include "vtkPWindBladeReader.h"

#include "vtkDataArraySelection.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPI.h"
#include "vtkMPIController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtksys/SystemTools.hxx"

#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkPWindBladeReader);

// This macro can be wrapped around MPI function calls to easily report errors.
// Reporting errors is more important with file I/O because, unlike network I/O,
// they usually don't terminate the program.
#define MPICall(funcall) \
  { \
  int __my_result = funcall; \
  if (__my_result != MPI_SUCCESS) \
    { \
    char errormsg[MPI_MAX_ERROR_STRING]; \
    int dummy; \
    MPI_Error_string(__my_result, errormsg, &dummy); \
    vtkErrorMacro(<< "Received error when calling" << endl \
                  << #funcall << endl << endl \
                  << errormsg); \
    } \
  }

class PWindBladeReaderInternal
{
public:
  MPI_File FilePtr;
};

//-----------------------------------------------------------------------------
vtkPWindBladeReader::vtkPWindBladeReader()
{
  this->PInternal = new PWindBladeReaderInternal();
}

//-----------------------------------------------------------------------------
vtkPWindBladeReader::~vtkPWindBladeReader()
{
  delete this->PInternal;
}

//-----------------------------------------------------------------------------
void vtkPWindBladeReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPWindBladeReader::RequestData(vtkInformation *reqInfo,
                                     vtkInformationVector **inVector,
                                     vtkInformationVector *outVector)
{
  if (!vtkMPIController::GetGlobalController()->IsA("vtkMPIController"))
    {
    // serial case
    return this->Superclass::RequestData(reqInfo, inVector, outVector);
    }
  int port = reqInfo->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());

  // field data port
  if (port == 0)
    {
    std::ostringstream fileName;
    vtkStructuredGrid *field = this->GetFieldOutput();
    this->InitFieldData(outVector, fileName, field);
    char* cchar = new char[strlen(fileName.str().c_str()) + 1];
    strcpy(cchar, fileName.str().c_str());
    MPICall(MPI_File_open(MPI_COMM_WORLD, cchar, MPI_MODE_RDONLY, MPI_INFO_NULL, &this->PInternal->FilePtr));
    delete [] cchar;
    if (this->PInternal->FilePtr == NULL)
      {
      vtkWarningMacro(<< "Could not open file " << fileName.str());
      }
    this->SetUpFieldVars(field);
    MPICall(MPI_File_close(&this->PInternal->FilePtr));
    return 1;
    }
  // Request data is on blade and is displayed only by processor 0
  // Even if the blade is turned off, it must update with time along with field
  else if (port == 1)
    {
    if (this->UseTurbineFile == 1 &&
        vtkMultiProcessController::GetGlobalController()->GetLocalProcessId() == 0)
      {
      this->InitBladeData(outVector);
      }
    return 1;
    }
  // Request data in on ground
  else if (port == 2)
    {
    this->SetUpGroundData(outVector);
    }

  return 1;
}


//----------------------------------------------------------------------------
// Calculate pressure from tempg and density
// Calculate pressure - pre from pressure in first z position
// Requires that all data be present
//----------------------------------------------------------------------------
void vtkPWindBladeReader::CalculatePressure(int pressure, int prespre,
                                            int tempg, int density)
{
  if (!vtkMPIController::GetGlobalController()->IsA("vtkMPIController"))
    {
    return this->Superclass::CalculatePressure(pressure, prespre,
                                               tempg, density);
    }
  float *pressureData = NULL, *prespreData = NULL;
  this->InitPressureData(pressure, prespre, pressureData, prespreData);

  // Read tempg and Density components from file
  float* tempgData   = new float[this->BlockSize];
  float* densityData = new float[this->BlockSize];

  MPI_Status status;
  char native[7] = "native";
  MPICall(MPI_File_set_view(this->PInternal->FilePtr, this->VariableOffset[tempg], MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));
  MPICall(MPI_File_read_all(this->PInternal->FilePtr, tempgData, this->BlockSize, MPI_FLOAT, &status));
  MPICall(MPI_File_set_view(this->PInternal->FilePtr, this->VariableOffset[density], MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));
  MPICall(MPI_File_read_all(this->PInternal->FilePtr, densityData, this->BlockSize, MPI_FLOAT, &status));

  // Only the requested subextents are stored on this processor
  this->SetUpPressureData(pressureData, prespreData, tempgData, densityData);

  delete [] tempgData;
  delete [] densityData;
}

//----------------------------------------------------------------------------
// Calculate vorticity from UVW
// Requires ghost cell information so fetch all data from files for now
//----------------------------------------------------------------------------
void vtkPWindBladeReader::CalculateVorticity(int vort, int uvw, int density)
{
  if (!vtkMPIController::GetGlobalController()->IsA("vtkMPIController"))
    return this->Superclass::CalculateVorticity(vort, uvw, density);

  // Set the number of components and tuples for the requested data
  this->Data[vort]->SetNumberOfComponents(1);
  this->Data[vort]->SetNumberOfTuples(this->NumberOfTuples);
  float* vortData = this->Data[vort]->GetPointer(0);

  // Read U and V components (two int block sizes in between)
  float* uData = new float[this->BlockSize];
  float* vData = new float[this->BlockSize];

  MPI_Status status;
  char native[7] = "native";
  MPICall(MPI_File_set_view(this->PInternal->FilePtr, this->VariableOffset[uvw], MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));
  MPICall(MPI_File_read_all(this->PInternal->FilePtr, uData, this->BlockSize, MPI_FLOAT, &status));
  MPICall(MPI_File_set_view(this->PInternal->FilePtr, (2 * sizeof(int)), MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));
  MPICall(MPI_File_read_all(this->PInternal->FilePtr, vData, this->BlockSize, MPI_FLOAT, &status));

  // Read Density component
  float* densityData = new float[this->BlockSize];
  MPICall(MPI_File_set_view(this->PInternal->FilePtr, this->VariableOffset[density], MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));
  MPICall(MPI_File_read_all(this->PInternal->FilePtr, densityData, this->BlockSize, MPI_FLOAT, &status));

  this->SetUpVorticityData(uData, vData, densityData, vortData);

  delete [] uData;
  delete [] vData;
  delete [] densityData;
}

//----------------------------------------------------------------------------
// Load one variable data array of BLOCK structure into ParaView
//----------------------------------------------------------------------------
void vtkPWindBladeReader::LoadVariableData(int var)
{
  if (!vtkMPIController::GetGlobalController()->IsA("vtkMPIController"))
    return this->Superclass::LoadVariableData(var);

  this->Data[var]->Delete();
  this->Data[var] = vtkFloatArray::New();
  this->Data[var]->SetName(VariableName[var].c_str());

  // Skip to the appropriate variable block and read byte count
  char native[7] = "native";
  MPICall(MPI_File_set_view(this->PInternal->FilePtr, this->VariableOffset[var], MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));

  int numberOfComponents = 0, planeSize = 0, rowSize;
  float *varData = NULL;
  float* block = new float[this->BlockSize];
  this->InitVariableData(var, numberOfComponents, varData, planeSize, rowSize);
  for (int comp = 0; comp < numberOfComponents; comp++)
    {
    MPI_Status status;
    MPICall(MPI_File_read_all(this->PInternal->FilePtr, block, this->BlockSize, MPI_FLOAT, &status));

    int pos = comp;
    for (int k = this->SubExtent[4]; k <= this->SubExtent[5]; k++)
      {
      for (int j = this->SubExtent[2]; j <= this->SubExtent[3]; j++)
        {
        for (int i = this->SubExtent[0]; i <= this->SubExtent[1]; i++)
          {
          int index = (k * planeSize) + (j * rowSize) + i;
          varData[pos] = block[index];
          pos += numberOfComponents;
          }
        }
      }

    // Skip closing and opening byte sizes
    MPICall(MPI_File_seek(this->PInternal->FilePtr, (2 * sizeof(int)), MPI_SEEK_CUR));
  }
  delete [] block;
}

//----------------------------------------------------------------------------
// Load one variable data array of BLOCK structure into ParaView
//----------------------------------------------------------------------------
bool vtkPWindBladeReader::ReadGlobalData()
{
  if (!vtkMPIController::GetGlobalController()->IsA("vtkMPIController"))
    {
    return this->Superclass::ReadGlobalData();
    }

  std::string fileName = this->Filename;
  vtksys::SystemTools::ConvertToUnixSlashes(fileName);

  std::vector<char> inBuf(vtkWindBladeReader::LINE_SIZE);
  MPI_File tempFile;
  char native[7] = "native";
  char* cchar = new char[strlen(fileName.c_str()) + 1];
  strcpy(cchar, fileName.c_str());
  MPICall(MPI_File_open(MPI_COMM_WORLD, cchar, MPI_MODE_RDONLY, MPI_INFO_NULL, &tempFile));
  delete [] cchar;

  std::stringstream inStr;
  MPI_Offset i, tempSize;
  MPI_Status status;

  MPICall(MPI_File_get_size(tempFile, &tempSize));
  MPICall(MPI_File_set_view(tempFile, 0, MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));

  for(i = 0; i < tempSize; i = i + vtkWindBladeReader::LINE_SIZE)
    {
    if(i + vtkWindBladeReader::LINE_SIZE > tempSize)
      {
      MPICall(MPI_File_read_all(tempFile, &(inBuf[0]), tempSize - i, MPI_BYTE, &status));
      inStr.write(&(inBuf[0]), tempSize - i);
      }
    else
      {
      MPICall(MPI_File_read_all(tempFile, &(inBuf[0]), vtkWindBladeReader::LINE_SIZE, MPI_BYTE, &status));
      inStr.write(&(inBuf[0]), vtkWindBladeReader::LINE_SIZE);
      }
    }

  MPICall(MPI_File_close(&tempFile));
  return this->SetUpGlobalData(fileName, inStr);
}

//----------------------------------------------------------------------------
//
// Open the first data file and verify that the data is where is should be
// Each data block is enclosed by two ints which record the number of bytes
// Save the file offset for each varible
//
//----------------------------------------------------------------------------
bool vtkPWindBladeReader::FindVariableOffsets()
{
  if (!vtkMPIController::GetGlobalController()->IsA("vtkMPIController"))
    return this->Superclass::FindVariableOffsets();

  // Open the first data file
  std::ostringstream fileName;
  fileName << this->RootDirectory << "/"
           << this->DataDirectory << "/"
           << this->DataBaseName << this->TimeStepFirst;

  char* cchar = new char[strlen(fileName.str().c_str()) + 1];
  strcpy(cchar, fileName.str().c_str());
  MPICall(MPI_File_open(MPI_COMM_WORLD, cchar, MPI_MODE_RDONLY, MPI_INFO_NULL, &this->PInternal->FilePtr));
  delete [] cchar;

  if (this->PInternal->FilePtr == NULL)
    {
    vtkErrorMacro("Could not open file " << fileName.str());
    return false;
    }

  // Scan file recording offsets which points to the first data value
  int byteCount;

  MPI_Status status;
  char native[7] = "native";
  MPICall(MPI_File_set_view(this->PInternal->FilePtr, 0, MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));
  MPICall(MPI_File_read_all(this->PInternal->FilePtr, &byteCount, 1, MPI_INT, &status));

  this->BlockSize = byteCount / BYTES_PER_DATA;

  for (int var = 0; var < this->NumberOfFileVariables; var++)
    {
    MPI_Offset offset;
    MPICall(MPI_File_get_position(this->PInternal->FilePtr, &offset));
    this->VariableOffset[var] = offset;

    // Skip over the SCALAR or VECTOR components for this variable
    int numberOfComponents = 1;
    if (this->VariableStruct[var] == VECTOR)
      {
      numberOfComponents = DIMENSION;
      }

    for (int comp = 0; comp < numberOfComponents; comp++)
      {
      // Skip data plus two integer byte counts
      MPICall(MPI_File_seek(this->PInternal->FilePtr, (byteCount+(2 * sizeof(int))), MPI_SEEK_CUR));
      }
    }
  MPICall(MPI_File_close(&this->PInternal->FilePtr));

  return true;
}

//----------------------------------------------------------------------------
// Create the z topography from 2D (x,y) elevations and return in zData
//----------------------------------------------------------------------------
void vtkPWindBladeReader::CreateZTopography(float* zValues)
{
  if (!vtkMPIController::GetGlobalController()->IsA("vtkMPIController"))
    return this->Superclass::CreateZTopography(zValues);

  // Read the x,y topography data file
  std::ostringstream fileName;
  fileName << this->RootDirectory << "/"
           << this->TopographyFile;

  int blockSize   = this->Dimension[0] * this->Dimension[1];
  float* topoData = new float[blockSize];
  char* cchar     = new char[strlen(fileName.str().c_str()) + 1];

  strcpy(cchar, fileName.str().c_str());
  MPICall(MPI_File_open(MPI_COMM_WORLD, cchar, MPI_MODE_RDONLY, MPI_INFO_NULL, &this->PInternal->FilePtr));
  delete [] cchar;

  MPI_Status status;
  char native[7] = "native";
  MPICall(MPI_File_set_view(this->PInternal->FilePtr, BYTES_PER_DATA, MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));
  MPICall(MPI_File_read_all(this->PInternal->FilePtr, topoData, blockSize, MPI_FLOAT, &status));

  this->ProcessZCoords(topoData, zValues);

  delete [] topoData;
  MPICall(MPI_File_close(&this->PInternal->FilePtr));
}

//----------------------------------------------------------------------------
// Build the turbine towers
// Parse a blade file to set the number of cells and points in blades
//----------------------------------------------------------------------------
void vtkPWindBladeReader::SetupBladeData()
{
  if (!vtkMPIController::GetGlobalController()->IsA("vtkMPIController"))
    return this->Superclass::SetupBladeData();

  // Load the tower information
  std::ostringstream fileName;
  fileName << this->RootDirectory << "/"
           << this->TurbineDirectory << "/"
           << this->TurbineTowerName;
  std::vector<char> inBuf(vtkWindBladeReader::LINE_SIZE);

  MPI_File tempFile;
  char native[7] = "native";
  char* cchar = new char[strlen(fileName.str().c_str()) + 1];
  strcpy(cchar, fileName.str().c_str());
  MPICall(MPI_File_open(MPI_COMM_WORLD, cchar, MPI_MODE_RDONLY, MPI_INFO_NULL, &tempFile));
  delete [] cchar;

  std::stringstream inStr;
  MPI_Offset i, tempSize;
  MPI_Status status;

  MPICall(MPI_File_get_size(tempFile, &tempSize));
  MPICall(MPI_File_set_view(tempFile, 0, MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));

  for(i = 0; i < tempSize; i = i + vtkWindBladeReader::LINE_SIZE)
    {
    if(i + vtkWindBladeReader::LINE_SIZE > tempSize)
      {
      MPICall(MPI_File_read_all(tempFile, &(inBuf[0]), tempSize - i, MPI_BYTE, &status));
      inStr.write(&(inBuf[0]), tempSize - i);
      }
    else
      {
      MPICall(MPI_File_read_all(tempFile, &(inBuf[0]), vtkWindBladeReader::LINE_SIZE, MPI_BYTE, &status));
      inStr.write(&(inBuf[0]), vtkWindBladeReader::LINE_SIZE);
      }
    }

  MPICall(MPI_File_close(&tempFile));

  if (!inStr)
    {
    vtkWarningMacro("Could not open " << fileName.str() << endl);
    }

  int numColumns = 0;
  this->ReadBladeHeader(fileName.str(), inStr, numColumns);

  // Calculate the number of cells in unstructured turbine blades
  std::ostringstream fileName2;
  fileName2 << this->RootDirectory << "/"
            << this->TurbineDirectory << "/"
            << this->TurbineBladeName << this->TimeStepFirst;

  cchar = new char[strlen(fileName2.str().c_str()) + 1];
  strcpy(cchar, fileName2.str().c_str());
  MPICall(MPI_File_open(MPI_COMM_WORLD, cchar, MPI_MODE_RDONLY, MPI_INFO_NULL, &tempFile));
  delete [] cchar;

  std::stringstream inStr2;

  MPICall(MPI_File_get_size(tempFile, &tempSize));
  MPICall(MPI_File_set_view(tempFile, 0, MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));

  for(i = 0; i < tempSize; i = i + vtkWindBladeReader::LINE_SIZE)
    {
    if(i + vtkWindBladeReader::LINE_SIZE > tempSize)
      {
      MPICall(MPI_File_read_all(tempFile, &(inBuf[0]), tempSize - i, MPI_BYTE, &status));
      inStr2.write(&(inBuf[0]), tempSize - i);
      }
    else
      {
      MPICall(MPI_File_read_all(tempFile, &(inBuf[0]), vtkWindBladeReader::LINE_SIZE, MPI_BYTE, &status));
      inStr2.write(&(inBuf[0]), vtkWindBladeReader::LINE_SIZE);
      }
    }

  MPICall(MPI_File_close(&tempFile));

  if (!inStr2)
    {
    vtkWarningMacro("Could not open blade file: " << fileName2.str().c_str() <<
                    " to calculate blade cells.");
    for (int j = this->TimeStepFirst + this->TimeStepDelta; j <= this->TimeStepLast;
         j += this->TimeStepDelta)
      {
      std::ostringstream fileName3;
      fileName3 << this->RootDirectory << "/"
                << this->TurbineDirectory << "/"
                << this->TurbineBladeName << j;
      //std::cout << "Trying " << fileName3.str().c_str() << "...";

      cchar = new char[strlen(fileName3.str().c_str()) + 1];
      strcpy(cchar, fileName3.str().c_str());
      MPICall(MPI_File_open(MPI_COMM_WORLD, cchar, MPI_MODE_RDONLY, MPI_INFO_NULL, &tempFile));
      delete [] cchar;

      inStr2.clear();
      inStr2.str("");

      MPICall(MPI_File_get_size(tempFile, &tempSize));
      MPICall(MPI_File_set_view(tempFile, 0, MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));

      for(i = 0; i < tempSize; i = i + vtkWindBladeReader::LINE_SIZE)
        {
        if(i + vtkWindBladeReader::LINE_SIZE > tempSize)
          {
          MPICall(MPI_File_read_all(tempFile, &(inBuf[0]), tempSize - i, MPI_BYTE, &status));
          inStr2.write(&(inBuf[0]), tempSize - i);
          }
        else
          {
          MPICall(MPI_File_read_all(tempFile, &(inBuf[0]), vtkWindBladeReader::LINE_SIZE, MPI_BYTE, &status));
          inStr2.write(&(inBuf[0]), vtkWindBladeReader::LINE_SIZE);
          }
        }
      MPICall(MPI_File_close(&tempFile));

      if(inStr2.good())
        {
        vtkWarningMacro("Success with " << fileName3.str());
        break;
        }
      else
        {
        vtkWarningMacro("Failure with " << fileName3.str());
        }
      }
    }

  this->NumberOfBladeCells = 0;
  // if we have at least 13 columns, then this is the new format with a header in the
  // turbine blade file
  if (numColumns >= 13 && inStr2)
    {
    int linesSkipped = 0;
    // each blade tower tries to split the columns such that there are
    // five items per line in header, so skip those lines
    this->NumberOfLinesToSkip = this->NumberOfBladeTowers*(int)ceil(numColumns/5.0);
    // now skip the first few lines based on header, if that applies
    while(inStr2.getline(&(inBuf[0]), vtkWindBladeReader::LINE_SIZE) &&
          linesSkipped < this->NumberOfLinesToSkip-1)
      {
      linesSkipped++;
      }
    }
  while (inStr2.getline(&(inBuf[0]), vtkWindBladeReader::LINE_SIZE))
    {
    this->NumberOfBladeCells++;
    }
  this->NumberOfBladePoints = this->NumberOfBladeCells * NUM_PART_SIDES;
  // Points and cells needed for constant towers
  this->NumberOfBladePoints += this->NumberOfBladeTowers * NUM_BASE_SIDES;
  this->NumberOfBladeCells += this->NumberOfBladeTowers;
}

//----------------------------------------------------------------------------
// Build the turbine blades
//----------------------------------------------------------------------------
void vtkPWindBladeReader::LoadBladeData(int timeStep)
{
  if (!vtkMPIController::GetGlobalController()->IsA("vtkMPIController"))
    return this->Superclass::LoadBladeData(timeStep);

  this->BPoints->Delete();
  this->BPoints = vtkPoints::New();

  // Open the file for this time step
  std::ostringstream fileName;
  fileName << this->RootDirectory << "/"
           << this->TurbineDirectory << "/"
           << this->TurbineBladeName
           << this->TimeSteps[timeStep];
  std::vector<char> inBuf(vtkWindBladeReader::LINE_SIZE);

  // only rank 0 reads this so we have to be careful
  MPI_File tempFile;
  char native[7] = "native";
  char* cchar = new char[strlen(fileName.str().c_str()) + 1];
  strcpy(cchar, fileName.str().c_str());
  // here only rank 0 opens it : MPI_COMM_SELF
  MPICall(MPI_File_open(MPI_COMM_SELF, cchar, MPI_MODE_RDONLY, MPI_INFO_NULL, &tempFile));
  delete [] cchar;

  std::stringstream inStr;
  MPI_Offset i, tempSize;
  MPI_Status status;

  MPICall(MPI_File_get_size(tempFile, &tempSize));
  MPICall(MPI_File_set_view(tempFile, 0, MPI_BYTE, MPI_BYTE, native, MPI_INFO_NULL));

  for(i = 0; i < tempSize; i = i + vtkWindBladeReader::LINE_SIZE)
    {
    if(i + vtkWindBladeReader::LINE_SIZE > tempSize)
      {
      MPICall(MPI_File_read(tempFile, &(inBuf[0]), tempSize - i, MPI_BYTE, &status));
      inStr.write(&(inBuf[0]), tempSize - i);
      }
    else
      {
      MPICall(MPI_File_read(tempFile, &(inBuf[0]), vtkWindBladeReader::LINE_SIZE, MPI_BYTE, &status));
      inStr.write(&(inBuf[0]), vtkWindBladeReader::LINE_SIZE);
      }
    }
  MPICall(MPI_File_close(&tempFile));

  this->ReadBladeData(inStr);
}
